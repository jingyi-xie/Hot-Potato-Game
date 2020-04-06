#include "potato.h"
#include "server.h"
#include "player.h"

using namespace std;
bool checkArgc(int argc, char ** argv) {
    if (argc < 3) {
        cout << "./player <machine_name> <port_num>" << endl;
        return false;
    }
    return true;
}

int main (int argc, char ** argv) {
    if (!checkArgc(argc, argv)) {
        return EXIT_FAILURE;
    }
    const char *machine_name = argv[1];
    const char *port_num = argv[2];
    int ringmaster_fd;
    int id;
    int num_players;
    Server * player_server = new Server((char*)"", PLAYER);
    Player * current_player = new Player();
    current_player->connectMaster(player_server, machine_name, port_num, ringmaster_fd, num_players, id);
    current_player->connectPlayers(player_server, ringmaster_fd);
    current_player->playGame(num_players);
    current_player->endGame();
    delete player_server;
    delete current_player;
    return EXIT_SUCCESS;
}