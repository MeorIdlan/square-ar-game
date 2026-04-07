#pragma once
// GameEngineService — round state machine, mirrors src/services/game_engine_service.py

#include "models/contracts.h"
#include "models/game_session_model.h"
#include "services/round_evaluator.h"

#include <optional>
#include <random>
#include <string>
#include <vector>

namespace sag {

class GameEngineService {
public:
    explicit GameEngineService(std::optional<unsigned> seed = std::nullopt);

    void start_round(GameSessionModel& session);
    void lock_round(GameSessionModel& session);
    void begin_check(GameSessionModel& session);
    void begin_preparing_next_round(GameSessionModel& session);
    void tick(GameSessionModel& session, float delta_seconds);

    void evaluate_round(
        GameSessionModel& session,
        const std::vector<MappedPlayerState>* mapped_players = nullptr);

    std::vector<MappedPlayerState> current_mapped_players(const GameSessionModel& session) const;

    void revive_player(GameSessionModel& session, const std::string& player_id);
    void eliminate_player(GameSessionModel& session, const std::string& player_id);
    void remove_player(GameSessionModel& session, const std::string& player_id);
    void reset_session(GameSessionModel& session);
    void force_next_round(GameSessionModel& session);

    void randomize_flashing_pattern(GameSessionModel& session);

    RenderState build_render_state(const GameSessionModel& session) const;

private:
    void refresh_round_outcome(GameSessionModel& session);

    static void ensure_membership(std::vector<std::string>& values, const std::string& id);
    static void remove_membership(std::vector<std::string>& values, const std::string& id);

    std::mt19937 rng_;
    RoundEvaluator evaluator_;
};

} // namespace sag
