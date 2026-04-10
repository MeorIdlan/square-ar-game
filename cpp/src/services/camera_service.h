#pragma once
// CameraService — Media Foundation camera capture (Windows only)

#include "services/interfaces.h"
#include "utils/config.h"

#ifdef _WIN32
#include <mfapi.h>
#include <mferror.h>
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

    // Deduplicate and sort camera profiles (highest resolution + fps first).
    // Removes entries with zero width, height, or fps.
    std::vector<CameraProfile> normalize_camera_formats(std::vector<CameraProfile> formats);

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

        // Formats enumerated during the last successful open(); empty if camera is not open.
        const std::vector<CameraProfile> &cached_formats() const { return cached_formats_; }

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
        std::vector<CameraProfile> cached_formats_;

        cv::Mat convert_sample_to_bgr(
#ifdef _WIN32
            IMFSample *sample
#else
            void *sample
#endif
        );
    };

} // namespace sag
