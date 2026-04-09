#ifdef _WIN32

#include "application.h"
#include "../utils/constants.h"
#include "../utils/logger.h"
#include <format>
#include <algorithm>
#include <filesystem>

namespace sag
{

    Application::~Application()
    {
        shutdown();
    }

    bool Application::initialize(HINSTANCE hInstance)
    {
        hinstance_ = hInstance;

        // Init logger — logs/ directory next to the executable
        {
            wchar_t buf[MAX_PATH]{};
            GetModuleFileNameW(nullptr, buf, MAX_PATH);
            std::filesystem::path exe_dir = std::filesystem::path(buf).parent_path();
            Logger::init(exe_dir);
        }
        Logger::info("Application initializing");

        // Load configuration
        config_ = load_config();
        Logger::info(std::format("Config loaded — camera index {}, grid {}x{}",
                                 config_.camera.camera_index, config_.grid.rows, config_.grid.columns));

        // Initialize session model from config
        session_.grid.rows = config_.grid.rows;
        session_.grid.columns = config_.grid.columns;
        session_.grid.playable_width = config_.grid.playable_width;
        session_.grid.playable_height = config_.grid.playable_height;
        session_.grid.reset_states();

        // Create services
        camera_service_ = std::make_unique<CameraService>();
        calibration_service_ = std::make_unique<CalibrationService>();
        pose_service_ = std::make_unique<PoseTrackingService>();
        floor_mapping_ = std::make_unique<FloorMappingService>();
        game_engine_ = std::make_unique<GameEngineService>();
        player_tracker_ = std::make_unique<PlayerTrackerService>();
        overlay_render_ = std::make_unique<OverlayRenderService>();
        debug_render_ = std::make_unique<DebugRenderService>();

        // Initialize camera
        bool camera_ok = camera_service_->open(config_.camera.camera_index,
                                               config_.camera.frame_width,
                                               config_.camera.frame_height,
                                               config_.camera.target_fps);
        if (camera_ok)
            Logger::info(std::format("Camera {} opened successfully ({}x{} @ {} fps)",
                                     config_.camera.camera_index,
                                     config_.camera.frame_width,
                                     config_.camera.frame_height,
                                     config_.camera.target_fps));
        else
            Logger::error(std::format("Camera {} failed to open ({}x{} @ {} fps) — running in fallback mode",
                                      config_.camera.camera_index,
                                      config_.camera.frame_width,
                                      config_.camera.frame_height,
                                      config_.camera.target_fps));

        // Initialize pose tracking
        if (!config_.pose.model_asset_path.empty())
        {
            Logger::info(std::format("Loading pose model: {}", config_.pose.model_asset_path));
            std::filesystem::path model_path(config_.pose.model_asset_path);
            pose_service_->initialize(model_path.string(), config_.pose.num_poses);
            Logger::info(std::format("Pose tracking initialized (max {} poses)", config_.pose.num_poses));
        }
        else
        {
            Logger::warn("No pose model path configured — pose tracking disabled");
        }

        // Initialize calibration service with ArUco dictionary
        calibration_service_->set_dictionary(config_.aruco_dictionary);

        // Create operator window (ImGui)
        operator_window_ = std::make_unique<ui::OperatorWindow>();
        if (!operator_window_->create(hInstance, config_, make_operator_callbacks()))
            return false;

        // Create projector window
        projector_window_ = std::make_unique<ui::ProjectorWindow>();
        if (!projector_window_->create_on_monitor(config_.display.projector_screen_index, hInstance))
            return false;
        projector_window_->show();

        // Create debug window
        debug_window_ = std::make_unique<ui::DebugWindow>();
        if (!debug_window_->create_default(hInstance))
            return false;
        debug_window_->show();

        // Start worker threads
        int target_fps = std::max(1, config_.camera.target_fps);
        camera_worker_ = std::make_unique<CameraWorker>(*camera_service_, latest_frame_, target_fps);
        vision_worker_ = std::make_unique<VisionWorker>(*pose_service_, vision_frame_, latest_pose_);

        camera_worker_->start();
        vision_worker_->start();
        Logger::info("Worker threads started");

        // Initialize session state
        session_.app_state = AppState::CameraReady;
        session_.status_message = "Ready to calibrate";
        session_.camera_status_message = "Waiting for camera frames";
        session_.pose_status_message = "Pose tracking idle";
        session_.display_status_message = std::format(
            "Projector assigned to display {}", config_.display.projector_screen_index);

        last_tick_time_ = std::chrono::steady_clock::now();
        running_ = true;

        Logger::info("Application initialized successfully");
        return true;
    }

    int Application::run()
    {
        // Set up game tick timer on the operator window
        SetTimer(operator_window_->hwnd(), GAME_TIMER_ID,
                 constants::GAME_TICK_INTERVAL_MS, nullptr);

        MSG msg{};
        while (running_)
        {
            while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
            {
                if (msg.message == WM_QUIT)
                {
                    running_ = false;
                    break;
                }

                if (msg.hwnd == operator_window_->hwnd() && msg.message == WM_TIMER)
                {
                    on_game_tick();
                    render_all();
                    continue;
                }

                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }

            if (!running_)
                break;

            // Check if operator window wants to close
            if (operator_window_->wants_close())
            {
                running_ = false;
                break;
            }

            // Yield to avoid busy-wait when no messages
            WaitMessage();
        }

        KillTimer(operator_window_->hwnd(), GAME_TIMER_ID);
        return 0;
    }

    void Application::shutdown()
    {
        Logger::info("Application shutting down");
        running_ = false;

        // Stop workers
        if (camera_worker_)
            camera_worker_->stop();
        if (vision_worker_)
            vision_worker_->stop();

        // Release hardware
        if (camera_service_)
            camera_service_->release();
        if (pose_service_)
            pose_service_->close();

        // Save config
        save_config(config_);
        Logger::info("Config saved");

        // Destroy windows
        operator_window_.reset();
        projector_window_.reset();
        debug_window_.reset();
    }

    void Application::on_game_tick()
    {
        // Process incoming data from worker threads
        process_camera_frame();
        process_pose_result();

        // Advance game engine
        auto now = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float>(now - last_tick_time_).count();
        last_tick_time_ = now;

        game_engine_->tick(session_, dt);

        publish_session();
    }

    void Application::render_all()
    {
        // Build render state
        RenderState render_state = game_engine_->build_render_state(session_);

        // Render operator window (ImGui)
        operator_window_->begin_frame();
        operator_window_->render(session_);
        operator_window_->end_frame();

        // Render projector window
        if (projector_window_ && projector_window_->is_visible())
        {
            projector_window_->render_frame(
                *overlay_render_, render_state,
                &latest_frame_copy_, &session_.calibration);
        }

        // Render debug window
        if (debug_window_ && debug_window_->is_visible())
        {
            debug_window_->render_frame(*debug_render_, render_state);
        }
    }

    void Application::process_camera_frame()
    {
        auto frame = latest_frame_.take();
        if (!frame)
            return;

        latest_frame_copy_ = *frame;

        // Also feed to the vision worker
        vision_frame_.set(*frame);

        // Update camera status
        if (frame->is_live)
        {
            int h = frame->frame.empty() ? 0 : frame->frame.rows;
            int w = frame->frame.empty() ? 0 : frame->frame.cols;
            std::string fps_text;
            if (frame->actual_fps > 0)
            {
                fps_text = std::format(" @ {:.1f} FPS", frame->actual_fps);
            }
            session_.camera_status_message = std::format(
                "Camera {} live at {}x{}{}", frame->camera_index, w, h, fps_text);
        }
        else
        {
            std::string detail = frame->error_message.empty()
                                     ? "Live camera feed unavailable."
                                     : frame->error_message;
            session_.camera_status_message = std::format("Fallback frame in use. {}", detail);
        }
    }

    void Application::process_pose_result()
    {
        auto pose = latest_pose_.take();
        if (!pose)
            return;

        // Map detections to floor coordinates
        std::vector<MappedPlayerState> mapped;
        int idx = 1;
        for (const auto &detection : pose->detections)
        {
            mapped.push_back(floor_mapping_->map_detection(
                "detection-" + std::to_string(idx++),
                detection, session_.grid, session_.calibration));
        }

        // Deduplicate
        mapped = deduplicate_detections(mapped, constants::DETECTION_DEDUP_DISTANCE);

        // Update player tracking
        player_tracker_->update_players(
            session_.players, mapped, pose->timestamp,
            session_.round_state.timings.missed_detection_grace_seconds);

        // Update status
        int mapped_count = 0, in_bounds_count = 0;
        for (const auto &m : mapped)
        {
            if (m.standing_point)
                ++mapped_count;
            if (m.in_bounds)
                ++in_bounds_count;
        }

        int active_count = 0;
        for (const auto &[_, p] : session_.players)
        {
            if (p.tracking_state == PlayerTrackingState::Active)
                ++active_count;
        }

        session_.pose_status_message = std::format(
            "{} | mapped {} | in bounds {}", pose->status_text, mapped_count, in_bounds_count);
        session_.status_message = std::format("Tracking {} active player(s)", active_count);
    }

    void Application::publish_session()
    {
        // Nothing to do — the session model is updated in place and read by render_all()
    }

    void Application::calibrate()
    {
        Logger::info("Calibration requested");
        session_.app_state = AppState::Calibrating;
        session_.status_message = "Attempting ArUco calibration from latest camera frame";

        if (!latest_frame_copy_.frame.empty())
        {
            session_.calibration = calibration_service_->calibrate(
                latest_frame_copy_.frame,
                session_.calibration.marker_layout,
                config_.grid.playable_width,
                config_.grid.playable_height,
                0.0f); // playable_inset
        }

        session_.app_state = session_.calibration.is_valid()
                                 ? AppState::Calibrated
                                 : AppState::CameraReady;
        session_.status_message = session_.calibration.validation_message;

        if (session_.calibration.is_valid())
        {
            Logger::info(std::format("Calibration succeeded: {}", session_.calibration.validation_message));
        }
        else
        {
            Logger::warn(std::format("Calibration failed: {}", session_.calibration.validation_message));
        }
    }

    void Application::reset_calibration()
    {
        Logger::info("Calibration reset");
        session_.calibration = CalibrationModel{};
        session_.app_state = AppState::CameraReady;
        session_.status_message = "Calibration reset";
    }

    void Application::update_projector_monitor(int index)
    {
        config_.display.projector_screen_index = index;
        if (projector_window_)
        {
            projector_window_->set_target_monitor(index);
        }
        session_.display_status_message = std::format("Projector assigned to display {}", index);
    }

    ui::OperatorCallbacks Application::make_operator_callbacks()
    {
        ui::OperatorCallbacks cb;

        cb.calibrate = [this]()
        { calibrate(); };
        cb.reset_calibration = [this]()
        { reset_calibration(); };

        cb.start_round = [this]()
        {
            Logger::info("Round started");
            game_engine_->start_round(session_);
        };
        cb.lock_round = [this]()
        {
            Logger::info("Round locked");
            game_engine_->lock_round(session_);
        };
        cb.force_next_round = [this]()
        {
            Logger::info("Round forced to next");
            game_engine_->force_next_round(session_);
        };
        cb.reset_session = [this]()
        {
            Logger::info("Session reset");
            game_engine_->reset_session(session_);
        };
        cb.save_settings = [this]()
        {
            save_config(config_);
        };

        cb.reconnect_camera = [this]()
        {
            camera_service_->release();
            camera_service_->open(config_.camera.camera_index,
                                  config_.camera.frame_width,
                                  config_.camera.frame_height,
                                  config_.camera.target_fps);
            session_.camera_status_message = "Camera reconnected";
        };

        cb.set_camera_index = [this](int idx)
        {
            config_.camera.camera_index = idx;
            camera_service_->release();
            camera_service_->open(idx, config_.camera.frame_width,
                                  config_.camera.frame_height,
                                  config_.camera.target_fps);
            session_.camera_status_message = std::format("Camera switched to {}", idx);
        };

        cb.set_projector_monitor = [this](int idx)
        {
            update_projector_monitor(idx);
        };

        cb.set_aruco_dictionary = [this](const std::string &name)
        {
            config_.aruco_dictionary = name;
            calibration_service_->set_dictionary(name);
        };

        cb.set_pose_model = [this](const std::string &path)
        {
            config_.pose.model_asset_path = path;
            pose_service_->close();
            pose_service_->initialize(path, config_.pose.num_poses);
        };

        cb.set_grid_rows = [this](int rows)
        {
            config_.grid.rows = rows;
            session_.grid.rows = rows;
            session_.grid.reset_states();
        };

        cb.set_grid_columns = [this](int cols)
        {
            config_.grid.columns = cols;
            session_.grid.columns = cols;
            session_.grid.reset_states();
        };

        cb.set_timing = [this](const std::string &field, float value)
        {
            auto &t = config_.timings;
            auto &rt = session_.round_state.timings;
            if (field == "flashing_duration_seconds")
            {
                t.flashing_duration_seconds = value;
                rt.flashing_duration_seconds = value;
            }
            else if (field == "flash_interval_seconds")
            {
                t.flash_interval_seconds = value;
                rt.flash_interval_seconds = value;
            }
            else if (field == "reaction_window_seconds")
            {
                t.reaction_window_seconds = value;
                rt.reaction_window_seconds = value;
            }
            else if (field == "lock_delay_seconds")
            {
                t.lock_delay_seconds = value;
                rt.lock_delay_seconds = value;
            }
            else if (field == "elimination_display_seconds")
            {
                t.elimination_display_seconds = value;
                rt.elimination_display_seconds = value;
            }
            else if (field == "inter_round_delay_seconds")
            {
                t.inter_round_delay_seconds = value;
                rt.inter_round_delay_seconds = value;
            }
            else if (field == "missed_detection_grace_seconds")
            {
                t.missed_detection_grace_seconds = value;
                rt.missed_detection_grace_seconds = value;
            }
        };

        cb.revive_player = [this](const std::string &pid)
        {
            game_engine_->revive_player(session_, pid);
        };
        cb.eliminate_player = [this](const std::string &pid)
        {
            game_engine_->eliminate_player(session_, pid);
        };
        cb.remove_player = [this](const std::string &pid)
        {
            game_engine_->remove_player(session_, pid);
        };

        return cb;
    }

} // namespace sag

#endif // _WIN32
