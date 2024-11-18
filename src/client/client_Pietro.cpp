#include "api.h"
#include "utils.h"
#include <string>
#include <iostream>


using namespace cycles;


class BotClient {
 Connection connection;
 std::string name;
 GameState state;


 void sendMove() {
   connection.sendMove(Direction::north);
 }


 void receiveGameState() {
   state = connection.receiveGameState();
   std::cout<<"There are "<<state.players.size()<<" players"<<std::endl;
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
