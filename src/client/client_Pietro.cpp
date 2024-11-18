#include "api.h"
#include "utils.h"
#include <iostream>
#include <random>
#include <spdlog/spdlog.h>
#include <string>
#include <map>
#include <set>
#include <algorithm>

using namespace cycles;

class BotClient {
    Connection connection;
    std::string name;
    GameState state;
    Player my_player;
    std::mt19937 rng;
    std::set<Position> visited_positions;  // Tracks visited positions
    int previousDirection = -1;

    // Checks if a move in the given direction is valid
    bool is_valid_move(Direction direction) const {
        auto new_pos = my_player.position + getDirectionVector(direction);
        return state.isInsideGrid(new_pos) && state.getGridCell(new_pos) == 0;
    }

    // Scores all directions based on distance to boarders, bots, and visited cells
    std::map<Direction, int> scoreDirections() const {
        std::map<Direction, int> scores;

        for (Direction direction : {Direction::north, Direction::east, Direction::south, Direction::west}) {
            int score = 0;
            auto next_pos = my_player.position + getDirectionVector(direction);

            if (!state.isInsideGrid(next_pos) || state.getGridCell(next_pos) != 0) {
                scores[direction] = -1000;  // Penalize invalid moves
                continue;
            }

            // Avoid visited positions
            if (visited_positions.count(next_pos)) {
                score -= 10;
            }

            // Penalize proximity to other bots
            for (const auto& player : state.players) {
                if (player.name != name) {
                    int distance = manhattanDistance(next_pos, player.position);
                    if (distance <= 2) score -= 20;  // Higher penalty for closer bots
                }
            }

            // Penalize proximity to boarders
            int boarder_penalty = std::min({next_pos.x, next_pos.y,
                                            state.getGridWidth() - 1 - next_pos.x,
                                            state.getGridHeight() - 1 - next_pos.y});
            score -= (5 - boarder_penalty);  // Penalize more as we get closer to the boarder

            scores[direction] = score;
        }

        return scores;
    }

    // Chooses the best move based on scores
    Direction decideMove() {
        constexpr int max_attempts = 200;
        auto direction_scores = scoreDirections();
        int attempts = 0;

        while (attempts < max_attempts) {
            // Select the direction with the highest score
            Direction best_direction = std::max_element(
                direction_scores.begin(),
                direction_scores.end(),
                [](const auto& a, const auto& b) { return a.second < b.second; }
            )->first;

            if (is_valid_move(best_direction)) {
                return best_direction;
            }

            direction_scores.erase(best_direction);  // Remove invalid direction and retry
            attempts++;
        }

        spdlog::error("{}: Failed to find a valid move after {} attempts", name, max_attempts);
        exit(1);
    }

    // Updates the bot's visited positions
    void updateVisitedPositions() {
        visited_positions.insert(my_player.position);
    }

    // Receives the game state and updates the player's state
    void receiveGameState() {
        state = connection.receiveGameState();

        auto it = std::find_if(state.players.begin(), state.players.end(),
                               [this](const Player& player) { return player.name == name; });

        if (it != state.players.end()) {
            my_player = *it;
        }
    }

    // Sends the decided move to the server
    void sendMove() {
        spdlog::debug("{}: Sending move", name);
        auto move = decideMove();
        updateVisitedPositions();  // Mark current position as visited
        connection.sendMove(move);
    }

public:
    BotClient(const std::string& botName) : name(botName), rng(std::random_device{}()) {
        connection.connect(name);
        if (!connection.isActive()) {
            spdlog::critical("{}: Connection failed", name);
            exit(1);
        }
    }

    void run() {
        while (connection.isActive()) {
            receiveGameState();
            sendMove();
        }
    }
};

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <bot_name>" << std::endl;
        return 1;
    }

#if SPDLOG_ACTIVE_LEVEL == SPDLOG_LEVEL_TRACE
    spdlog::set_level(spdlog::level::debug);
#endif

    std::string botName = argv[1];
    BotClient bot(botName);
    bot.run();
    return 0;
}
