#pragma once
// CameraService — Media Foundation camera capture (Windows only)

#include "services/interfaces.h"

#ifdef _WIN32
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <wrl/client.h>
#endif

#include <atomic>
#include <chrono>
#include <string>
#include <vector>

namespace sag
{

    struct CameraDeviceInfo
    {
        std::string name;
        std::string symbolic_link;
        int index = 0;
    };

    class CameraService : public ICameraService
    {
    public:
        CameraService();
        ~CameraService();

        bool open(int camera_index, int width, int height, int fps) override;
        FramePacket next_frame() override;
        void release() override;
        bool is_open() const override;

        std::vector<CameraDeviceInfo> enumerate_devices();

    private:
#ifdef _WIN32
        Microsoft::WRL::ComPtr<IMFSourceReader> reader_;
        Microsoft::WRL::ComPtr<IMFMediaSource> source_;
#endif

        std::atomic<bool> is_open_{false};
        int frame_counter_ = 0;
        int camera_index_ = 0;
        int width_ = 1280;
        int height_ = 720;
        bool use_nv12_ = false;
        std::string source_name_ = "fallback";

        cv::Mat convert_sample_to_bgr(
#ifdef _WIN32
            IMFSample *sample
#else
            void *sample
#endif
        );
    };

} // namespace sag
