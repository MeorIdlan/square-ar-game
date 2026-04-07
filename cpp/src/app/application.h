#pragma once
// Application — owns all services, workers, and windows.
// Acts as the bootstrap/wiring layer (equivalent to Python's bootstrap.py + main_viewmodel.py).

#ifdef _WIN32

#include "../models/game_session_model.h"
#include "../models/contracts.h"
#include "../services/calibration_service.h"
#include "../services/camera_service.h"
#include "../services/floor_mapping_service.h"
#include "../services/game_engine_service.h"
#include "../services/player_tracker_service.h"
#include "../services/pose_tracking_service.h"
#include "../services/overlay_render_service.h"
#include "../services/debug_render_service.h"
#include "../services/detection_deduplicator.h"
#include "../threading/camera_worker.h"
#include "../threading/vision_worker.h"
#include "../threading/shared_state.h"
#include "../ui/operator_window.h"
#include "../ui/projector_window.h"
#include "../ui/debug_window.h"
#include "../utils/config.h"
#include "../utils/constants.h"

#include <windows.h>
#include <memory>
#include <chrono>

namespace sag {

class Application {
public:
    Application() = default;
    ~Application();

    bool initialize(HINSTANCE hInstance);
    int  run();      // message loop — blocks until quit
    void shutdown();

    // Called from WM_TIMER on the main thread
    void on_game_tick();

    // Called each frame from the render loop
    void render_all();

private:
    // Pipeline stages (called on main thread each tick)
    void process_camera_frame();
    void process_pose_result();
    void publish_session();

    // UI callback wiring
    ui::OperatorCallbacks make_operator_callbacks();

    // Helpers
    void calibrate();
    void reset_calibration();
    void update_projector_monitor(int index);

    HINSTANCE hinstance_ = nullptr;

    // Config
    AppConfig config_;

    // Models
    GameSessionModel session_;

    // Services
    std::unique_ptr<CameraService>         camera_service_;
    std::unique_ptr<CalibrationService>     calibration_service_;
    std::unique_ptr<PoseTrackingService>    pose_service_;
    std::unique_ptr<FloorMappingService>    floor_mapping_;
    std::unique_ptr<GameEngineService>      game_engine_;
    std::unique_ptr<PlayerTrackerService>   player_tracker_;
    std::unique_ptr<OverlayRenderService>   overlay_render_;
    std::unique_ptr<DebugRenderService>     debug_render_;

    // Threading
    LatestValue<FramePacket> latest_frame_;
    LatestValue<FramePacket> vision_frame_;   // copy for vision worker
    LatestValue<PoseResult>  latest_pose_;
    std::unique_ptr<CameraWorker> camera_worker_;
    std::unique_ptr<VisionWorker> vision_worker_;

    // UI windows (all on main thread)
    std::unique_ptr<ui::OperatorWindow>  operator_window_;
    std::unique_ptr<ui::ProjectorWindow> projector_window_;
    std::unique_ptr<ui::DebugWindow>     debug_window_;

    // State
    FramePacket                latest_frame_copy_;
    std::chrono::steady_clock::time_point last_tick_time_;
    bool                       running_ = false;

    // Game tick timer ID
    static constexpr UINT_PTR GAME_TIMER_ID = 1;
};

} // namespace sag

#endif // _WIN32
