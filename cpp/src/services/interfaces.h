#pragma once
// Pure virtual interfaces for all services — mirrors src/services/protocols.py

#include "models/calibration_model.h"
#include "models/contracts.h"
#include "models/game_session_model.h"
#include "models/grid_model.h"

#include <optional>
#include <string>
#include <vector>

namespace sag {

class ICalibrationService {
public:
    virtual ~ICalibrationService() = default;
    virtual CalibrationModel calibrate(const cv::Mat& frame, const CalibrationMarkerLayout& layout,
                                        float playable_width, float playable_height,
                                        float playable_inset) = 0;
};

class IFloorMappingService {
public:
    virtual ~IFloorMappingService() = default;
    virtual MappedPlayerState map_detection(const std::string& player_id,
                                             const PoseFootState& pose_state,
                                             const GridModel& grid,
                                             const CalibrationModel& calibration) const = 0;
};

class IPlayerTracker {
public:
    virtual ~IPlayerTracker() = default;
    virtual void update_players(std::unordered_map<std::string, PlayerModel>& players,
                                 const std::vector<MappedPlayerState>& detections,
                                 double timestamp,
                                 float grace_period_seconds) const = 0;
};

class IGameEngine {
public:
    virtual ~IGameEngine() = default;
    virtual void start_round(GameSessionModel& session) = 0;
    virtual void tick(GameSessionModel& session, float delta_seconds) = 0;
    virtual void evaluate_round(GameSessionModel& session,
                                 const std::vector<MappedPlayerState>* mapped_players = nullptr) = 0;
    virtual void revive_player(GameSessionModel& session, const std::string& player_id) = 0;
    virtual void eliminate_player(GameSessionModel& session, const std::string& player_id) = 0;
    virtual void remove_player(GameSessionModel& session, const std::string& player_id) = 0;
    virtual void reset_session(GameSessionModel& session) = 0;
    virtual RenderState build_render_state(const GameSessionModel& session) const = 0;
};

class ICameraService {
public:
    virtual ~ICameraService() = default;
    virtual bool open(int camera_index, int width, int height, int fps) = 0;
    virtual FramePacket next_frame() = 0;
    virtual void release() = 0;
    virtual bool is_open() const = 0;
};

class IPoseTracker {
public:
    virtual ~IPoseTracker() = default;
    virtual bool initialize(const std::string& model_path, int num_poses) = 0;
    virtual PoseResult process_frame(const FramePacket& packet) = 0;
    virtual void close() = 0;
};

} // namespace sag
