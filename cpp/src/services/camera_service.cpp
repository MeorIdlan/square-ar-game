#include "services/camera_service.h"

#include <chrono>
#include <format>
#include <opencv2/imgproc.hpp>
#include "utils/logger.h"

namespace sag
{

    CameraService::CameraService() = default;
    CameraService::~CameraService() { release(); }

#ifdef _WIN32

    // ── Windows Media Foundation implementation ──────────────────────────

    bool CameraService::open(int camera_index, int width, int height, int fps)
    {
        release();

        camera_index_ = camera_index;
        width_ = width;
        height_ = height;

        Logger::info(std::format("CameraService::open — index={} {}x{} @{}fps", camera_index, width, height, fps));

        // Enumerate video capture devices
        IMFAttributes *attributes = nullptr;
        HRESULT hr = MFCreateAttributes(&attributes, 1);
        if (FAILED(hr))
        {
            Logger::error(std::format("MFCreateAttributes failed hr=0x{:08X}", static_cast<unsigned>(hr)));
            return false;
        }

        hr = attributes->SetGUID(
            MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
            MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);
        if (FAILED(hr))
        {
            Logger::error(std::format("SetGUID SOURCE_TYPE failed hr=0x{:08X}", static_cast<unsigned>(hr)));
            attributes->Release();
            return false;
        }

        IMFActivate **devices = nullptr;
        UINT32 count = 0;
        hr = MFEnumDeviceSources(attributes, &devices, &count);
        attributes->Release();
        Logger::info(std::format("MFEnumDeviceSources hr=0x{:08X} count={}", static_cast<unsigned>(hr), count));
        if (FAILED(hr) || static_cast<int>(count) <= camera_index)
        {
            Logger::error(std::format("No device at index {} (total={})", camera_index, count));
            return false;
        }

        // Activate the requested device
        hr = devices[camera_index]->ActivateObject(
            IID_PPV_ARGS(source_.ReleaseAndGetAddressOf()));
        Logger::info(std::format("ActivateObject hr=0x{:08X}", static_cast<unsigned>(hr)));

        // Get device name
        WCHAR *friendlyName = nullptr;
        UINT32 nameLen = 0;
        if (SUCCEEDED(devices[camera_index]->GetAllocatedString(
                MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME, &friendlyName, &nameLen)))
        {
            int size = WideCharToMultiByte(CP_UTF8, 0, friendlyName, nameLen, nullptr, 0, nullptr, nullptr);
            source_name_.resize(size);
            WideCharToMultiByte(CP_UTF8, 0, friendlyName, nameLen, source_name_.data(), size, nullptr, nullptr);
            CoTaskMemFree(friendlyName);
            Logger::info(std::format("Device friendly name: {}", source_name_));
        }

        for (UINT32 i = 0; i < count; ++i)
            devices[i]->Release();
        CoTaskMemFree(devices);

        if (FAILED(hr))
        {
            Logger::error(std::format("ActivateObject failed hr=0x{:08X}", static_cast<unsigned>(hr)));
            return false;
        }

        // Create source reader
        hr = MFCreateSourceReaderFromMediaSource(
            source_.Get(), nullptr, reader_.ReleaseAndGetAddressOf());
        Logger::info(std::format("MFCreateSourceReaderFromMediaSource hr=0x{:08X}", static_cast<unsigned>(hr)));
        if (FAILED(hr))
        {
            Logger::error(std::format("MFCreateSourceReaderFromMediaSource failed hr=0x{:08X}", static_cast<unsigned>(hr)));
            return false;
        }

        // Configure output format: request NV12 or RGB32 at target resolution
        Microsoft::WRL::ComPtr<IMFMediaType> mediaType;
        hr = MFCreateMediaType(mediaType.GetAddressOf());
        if (FAILED(hr))
            return false;

        mediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
        mediaType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);
        MFSetAttributeSize(mediaType.Get(), MF_MT_FRAME_SIZE, width, height);
        MFSetAttributeRatio(mediaType.Get(), MF_MT_FRAME_RATE, fps, 1);

        hr = reader_->SetCurrentMediaType(
            static_cast<DWORD>(MF_SOURCE_READER_FIRST_VIDEO_STREAM),
            nullptr, mediaType.Get());
        Logger::info(std::format("SetCurrentMediaType RGB32 hr=0x{:08X}", static_cast<unsigned>(hr)));
        if (FAILED(hr))
        {
            // Fall back to NV12 and convert ourselves
            Logger::warn("RGB32 not accepted, falling back to NV12");
            mediaType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_NV12);
            hr = reader_->SetCurrentMediaType(
                static_cast<DWORD>(MF_SOURCE_READER_FIRST_VIDEO_STREAM),
                nullptr, mediaType.Get());
            Logger::info(std::format("SetCurrentMediaType NV12 hr=0x{:08X}", static_cast<unsigned>(hr)));
            if (SUCCEEDED(hr))
                use_nv12_ = true;
        }
        else
        {
            use_nv12_ = false;
        }

        if (FAILED(hr))
        {
            Logger::error(std::format("SetCurrentMediaType failed for all formats hr=0x{:08X}", static_cast<unsigned>(hr)));
            return false;
        }

        Logger::info(std::format("Pixel format: {}", use_nv12_ ? "NV12" : "RGB32/BGRA"));

        is_open_ = true;
        frame_counter_ = 0;
        return true;
    }

    FramePacket CameraService::next_frame()
    {
        FramePacket packet;
        packet.frame_id = ++frame_counter_;
        packet.camera_index = camera_index_;
        packet.source_name = source_name_;

        auto now = std::chrono::steady_clock::now();
        packet.timestamp = std::chrono::duration<double>(now.time_since_epoch()).count();

        if (!is_open_ || !reader_)
        {
            packet.error_message = "Camera not open";
            return packet;
        }

        DWORD stream_index = 0, flags = 0;
        LONGLONG timestamp = 0;
        Microsoft::WRL::ComPtr<IMFSample> sample;

        HRESULT hr = reader_->ReadSample(
            static_cast<DWORD>(MF_SOURCE_READER_FIRST_VIDEO_STREAM),
            0, &stream_index, &flags, &timestamp, sample.GetAddressOf());

        if (FAILED(hr) || !sample)
        {
            // MF_SOURCE_READERF_STREAMTICK (0x100): a gap in the media stream —
            // no sample was produced this tick but the stream is still live.
            // Silently skip; the next ReadSample call will return a real frame.
            if (SUCCEEDED(hr) && (flags & MF_SOURCE_READERF_STREAMTICK))
                return packet;

            packet.error_message = std::format("Failed to read sample hr=0x{:08X} flags=0x{:08X}",
                                               static_cast<unsigned>(hr), flags);
            Logger::warn(std::format("ReadSample failed: {}", packet.error_message));
            return packet;
        }

        packet.frame = convert_sample_to_bgr(sample.Get());
        if (!packet.frame.empty())
        {
            packet.is_live = true;
            if (frame_counter_ <= 3)
                Logger::info(std::format("First frame received: {}x{}", packet.frame.cols, packet.frame.rows));
        }
        else
        {
            packet.error_message = "Failed to convert sample to BGR";
            Logger::warn("convert_sample_to_bgr returned empty mat");
        }

        return packet;
    }

    cv::Mat CameraService::convert_sample_to_bgr(IMFSample *sample)
    {
        if (!sample)
            return {};

        Microsoft::WRL::ComPtr<IMFMediaBuffer> buffer;
        HRESULT hr = sample->ConvertToContiguousBuffer(buffer.GetAddressOf());
        if (FAILED(hr))
        {
            Logger::warn(std::format("ConvertToContiguousBuffer failed hr=0x{:08X}", static_cast<unsigned>(hr)));
            return {};
        }

        BYTE *data = nullptr;
        DWORD maxLen = 0, curLen = 0;
        hr = buffer->Lock(&data, &maxLen, &curLen);
        if (FAILED(hr))
        {
            Logger::warn(std::format("IMFMediaBuffer::Lock failed hr=0x{:08X}", static_cast<unsigned>(hr)));
            return {};
        }

        cv::Mat bgr;

        if (use_nv12_)
        {
            // NV12: Y plane (width*height bytes) + interleaved UV plane (width*height/2 bytes)
            // Expected buffer size = width * height * 3 / 2
            DWORD expected = static_cast<DWORD>(width_ * height_ * 3 / 2);
            if (curLen < expected)
            {
                Logger::warn(std::format("NV12 buffer too small: got {} expected {}", curLen, expected));
                buffer->Unlock();
                return {};
            }
            cv::Mat yuv(height_ * 3 / 2, width_, CV_8UC1, data);
            cv::cvtColor(yuv, bgr, cv::COLOR_YUV2BGR_NV12);
        }
        else
        {
            // RGB32 / BGRA: 4 bytes per pixel
            DWORD expected = static_cast<DWORD>(width_ * height_ * 4);
            if (curLen < expected)
            {
                Logger::warn(std::format("BGRA buffer too small: got {} expected {}", curLen, expected));
                buffer->Unlock();
                return {};
            }
            cv::Mat bgra(height_, width_, CV_8UC4, data);
            cv::cvtColor(bgra, bgr, cv::COLOR_BGRA2BGR);
        }

        buffer->Unlock();
        return bgr.clone();
    }

    void CameraService::release()
    {
        is_open_ = false;
        reader_.Reset();
        if (source_)
        {
            source_->Shutdown();
            source_.Reset();
        }
    }

    bool CameraService::is_open() const { return is_open_.load(); }

    std::vector<CameraDeviceInfo> CameraService::enumerate_devices()
    {
        std::vector<CameraDeviceInfo> result;

        IMFAttributes *attributes = nullptr;
        HRESULT hr = MFCreateAttributes(&attributes, 1);
        if (FAILED(hr))
            return result;

        hr = attributes->SetGUID(
            MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
            MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);
        if (FAILED(hr))
        {
            attributes->Release();
            return result;
        }

        IMFActivate **devices = nullptr;
        UINT32 count = 0;
        hr = MFEnumDeviceSources(attributes, &devices, &count);
        attributes->Release();
        if (FAILED(hr))
            return result;

        for (UINT32 i = 0; i < count; ++i)
        {
            CameraDeviceInfo info;
            info.index = static_cast<int>(i);

            WCHAR *name = nullptr;
            UINT32 nameLen = 0;
            if (SUCCEEDED(devices[i]->GetAllocatedString(
                    MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME, &name, &nameLen)))
            {
                int size = WideCharToMultiByte(CP_UTF8, 0, name, nameLen, nullptr, 0, nullptr, nullptr);
                info.name.resize(size);
                WideCharToMultiByte(CP_UTF8, 0, name, nameLen, info.name.data(), size, nullptr, nullptr);
                CoTaskMemFree(name);
            }

            WCHAR *link = nullptr;
            UINT32 linkLen = 0;
            if (SUCCEEDED(devices[i]->GetAllocatedString(
                    MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK, &link, &linkLen)))
            {
                int size = WideCharToMultiByte(CP_UTF8, 0, link, linkLen, nullptr, 0, nullptr, nullptr);
                info.symbolic_link.resize(size);
                WideCharToMultiByte(CP_UTF8, 0, link, linkLen, info.symbolic_link.data(), size, nullptr, nullptr);
                CoTaskMemFree(link);
            }

            result.push_back(std::move(info));
            devices[i]->Release();
        }
        CoTaskMemFree(devices);
        return result;
    }

#else

    // ── Non-Windows stub (allows compilation on Linux for CI/testing) ────

    bool CameraService::open(int camera_index, int width, int height, int /*fps*/)
    {
        camera_index_ = camera_index;
        width_ = width;
        height_ = height;
        source_name_ = "Camera " + std::to_string(camera_index);
        is_open_ = false;
        return false;
    }

    FramePacket CameraService::next_frame()
    {
        FramePacket packet;
        packet.frame_id = ++frame_counter_;
        packet.error_message = "Media Foundation not available on this platform";
        return packet;
    }

    cv::Mat CameraService::convert_sample_to_bgr(void *) { return {}; }
    void CameraService::release() { is_open_ = false; }
    bool CameraService::is_open() const { return false; }
    std::vector<CameraDeviceInfo> CameraService::enumerate_devices() { return {}; }

#endif // _WIN32

} // namespace sag
