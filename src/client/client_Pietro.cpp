#include "api.h"
#include "utils.h"
#include <iostream>
#include <random>
#include <spdlog/spdlog.h>
#include <string>
#include <map>
#include <algorithm>

using namespace cycles;

class BotClient {
    Connection connection;
    std::string name;
    GameState state;
    Player my_player;
    std::mt19937 rng;
    int previousDirection = -1;
    int inertia = 30;

    // Checks if a move in the given direction is valid
    bool is_valid_move(Direction direction) const {
        auto new_pos = my_player.position + getDirectionVector(direction);
        return state.isInsideGrid(new_pos) && state.getGridCell(new_pos) == 0;
    }

    // Detects if another bot is near the player
    bool isBotNearby() const {
        for (const auto& player : state.players) {
            if (player.name != name) {  // Check other players
                if (manhattanDistance(my_player.position, player.position) <= 3) {
                    return true;
                }
            }
        }
        return false;
    }

    // Finds the target position on the opposite side of the map
    Position findOppositePosition() const {
        int grid_width = state.getGridWidth();
        int grid_height = state.getGridHeight();

        // Opposite position is a reflection across the center of the map
        int target_x = grid_width - 1 - my_player.position.x;
        int target_y = grid_height - 1 - my_player.position.y;

        return {target_x, target_y};
    }

    // Scores all directions based on distance to a target position
    std::map<Direction, int> scoreDirectionsTowardsTarget(const Position& target) const {
        std::map<Direction, int> scores;

        for (Direction direction : {Direction::north, Direction::east, Direction::south, Direction::west}) {
            auto new_pos = my_player.position + getDirectionVector(direction);

            if (state.isInsideGrid(new_pos) && state.getGridCell(new_pos) == 0) {
                // Use negative distance as a score (minimizing distance)
                scores[direction] = -manhattanDistance(new_pos, target);
            } else {
                // Invalid moves get a very low score
                scores[direction] = -1000;
            }
        }

        return scores;
    }

    // Chooses the best move considering nearby bots and the map's opposite side
    Direction decideMove() {
        constexpr int max_attempts = 200;
        int attempts = 0;

        // Check for nearby bots
        if (isBotNearby()) {
            spdlog::info("{}: Bot nearby detected. Moving towards the opposite side of the map.", name);
            auto target_position = findOppositePosition();
            auto direction_scores = scoreDirectionsTowardsTarget(target_position);

            // Select the direction leading closest to the target position
            Direction best_direction = std::max_element(
                direction_scores.begin(),
                direction_scores.end(),
                [](const auto& a, const auto& b) { return a.second < b.second; }
            )->first;

            return best_direction;
        }

        // Default behavior (no nearby bots)
        auto direction_scores = scoreDirections();
        while (attempts < max_attempts) {
            Direction best_direction = std::max_element(
                direction_scores.begin(),
                direction_scores.end(),
                [](const auto& a, const auto& b) { return a.second < b.second; }
            )->first;

            if (is_valid_move(best_direction)) {
                if (inertia > 0 && previousDirection == getDirectionValue(best_direction)) {
                    inertia--;
                }
                return best_direction;
            }

            direction_scores.erase(best_direction);
            attempts++;
        }

        spdlog::error("{}: Failed to find a valid move after {} attempts", name, max_attempts);
        exit(1);
    }

    // Receives the game state and updates the player's state
    void receiveGameState() {
        state = connection.receiveGameState();
        updateInertia();

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
        previousDirection = getDirectionValue(move);
        connection.sendMove(move);
    }

    // Updates inertia based on open space
    void updateInertia() {
        auto direction_scores = scoreDirections();
        int max_open_space = std::max_element(
            direction_scores.begin(),
            direction_scores.end(),
            [](const auto& a, const auto& b) { return a.second < b.second; }
        )->second;

        if (max_open_space < 2) {
            inertia = std::max(0, inertia - 1);
        } else {
            inertia = std::min(30, inertia + 1);
        }
    }

    // Scores all directions based on open space
    std::map<Direction, int> scoreDirections() const {
        std::map<Direction, int> scores;

        for (Direction direction : {Direction::north, Direction::east, Direction::south, Direction::west}) {
            int score = 0;
            auto next_pos = my_player.position;

            for (int i = 0; i < 5; ++i) {
                next_pos += getDirectionVector(direction);

                if (!state.isInsideGrid(next_pos) || state.getGridCell(next_pos) != 0) {
                    break;
                }
                score++;
            }

            scores[direction] = score;
        }

        return scores;
    }

public:
    BotClient(const std::string& botName) : name(botName), rng(std::random_device{}()) {
        std::uniform_int_distribution<int> dist(10, 30);
        inertia = dist(rng);

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
