#include "api.h"
#include "utils.h"
#include <string>
#include <iostream>
#include <cmath>
#include <climits>

using namespace cycles;

class BotClient {
  Connection connection;
  std::string name;
  GameState state;

  void sendMove() {
    sf::Vector2i currentPosition = state.players[0].position; // Your bot's position
    sf::Vector2i targetPosition;
    int minDistance = INT_MAX;

    // Find the nearest player
    for (const auto &player : state.players) {
      if (player.name == name) continue; // Skip your own bot

      int distance = std::abs(player.position.x - currentPosition.x) +
                     std::abs(player.position.y - currentPosition.y); // Manhattan distance

      if (distance < minDistance) {
        minDistance = distance;
        targetPosition = player.position;
      }
    }

    // Determine the best direction to move towards the target
    sf::Vector2i delta = targetPosition - currentPosition;

    if (std::abs(delta.x) > std::abs(delta.y)) {
      // Prioritize horizontal movement
      if (delta.x > 0 && state.isCellEmpty(currentPosition + getDirectionVector(Direction::east))) {
        connection.sendMove(Direction::east);
        return;
      } else if (delta.x < 0 && state.isCellEmpty(currentPosition + getDirectionVector(Direction::west))) {
        connection.sendMove(Direction::west);
        return;
      }
    }

    // Fallback to vertical movement
    if (delta.y > 0 && state.isCellEmpty(currentPosition + getDirectionVector(Direction::south))) {
      connection.sendMove(Direction::south);
    } else if (delta.y < 0 && state.isCellEmpty(currentPosition + getDirectionVector(Direction::north))) {
      connection.sendMove(Direction::north);
    } else {
      // Default action if all else fails
      connection.sendMove(Direction::north);
    }
  }

  void receiveGameState() {
    state = connection.receiveGameState();
    std::cout << "There are " << state.players.size() << " players" << std::endl;
  }

public:
  BotClient(const std::string &botName) : name(botName) {
    connection.connect(name);
    if (!connection.isActive()) {
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

int main(int argc, char *argv[]) {
  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " <bot_name>" << std::endl;
    return 1;
  }
  std::string botName = argv[1];
  BotClient bot(botName);
  bot.run();
  return 0;
}
