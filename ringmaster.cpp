#include "potato.h"
#include "server.h"
#include "player.h"
#include <sstream>

using namespace std;
//Check is the argument is valid
bool checkArgc(int argc, char ** argv) {
    if (argc < 4) {
        cout << "./ringmaster <port_num> <num_players> <num_hops>" << endl;
        return false;
    }
    return true;
}

//Connect the ringmaster with every player
void connectPlayers(Server * master_server, int num_players, vector<int> &fds, vector<int> &ports, vector<string> &ips) {
    int cur = 0;
    while (cur < num_players) {
        string cur_ip;
        int curFd = master_server->acceptConnect(cur_ip);
        ips[cur] = cur_ip;
        //Send #players and player_id to each player
        send(curFd, &num_players, sizeof(num_players), 0);
        send(curFd, &cur, sizeof(cur), 0);
        //Receive port number of each player
        recv(curFd, &ports[cur], sizeof(ports[cur]), MSG_WAITALL);
        cout << "Player " << cur << " is ready to play" << endl;
        // cout << "Player " << cur << " ip is " << ips[cur] << endl;
        // cout << "Player " << cur << " port is " << ports[cur] << endl;
        fds[cur] = curFd;
        cur++;
    }
}

//Circle all the players together
void circlePlayers(Server * master_server, int num_players, vector<int> &fds, vector<int> &ports, vector<string> &ips) {
    int cur = 0;
    while (cur < num_players) {
        Player next_player;
        next_player.port_num = ports[(cur + 1) % num_players];
        const char * ip_send = ips[(cur + 1) % num_players].c_str();
        strcpy(next_player.ip, ip_send);
        //Send the "next_player" to each player to connect with
        send(fds[cur], &next_player, sizeof(next_player), 0);
        // std::cout << "next ip is " << next_player.port_num << endl;
        // std::cout << "next port is " << next_player.ip << endl;
        cur++;
    }
}

//End the game, send a potato which num_hops is 0
void endGame(int num_players, vector<int> fds) {
    Potato potato(0);
    for (int i = 0; i < num_players; i++) {
        send(fds[i], &potato, sizeof(potato), 0);
    }
}

void startGame(int num_hops, int num_players, vector<int> fds) {
    //If #hops is 0, end the game
    if (num_hops == 0) {
        endGame(num_players, fds);
    }
    else {
        srand((unsigned int)time(NULL));
        int random_player = rand() % num_players;
        cout << "Ready to start the game, sending potato to player " << random_player << endl; 
        Potato potato(num_hops);
        //Send the potato to a random player
        send(fds[random_player], &potato, sizeof(potato), 0);
        fd_set readfds;
        //Clear all entries in the set
        FD_ZERO(&readfds);
        //Add the fd of each player to the set
        for (int i = 0; i < num_players; i++) {
            FD_SET(fds[i], &readfds);
        }
        select(fds[num_players - 1] + 1, &readfds, NULL, NULL, NULL);
        for (int i = 0; i < num_players; i++) {
            //If one fd is ready for reading(at the end of the game)
            if (FD_ISSET(fds[i], &readfds)) {
                //Receive the potato and end the game
                recv(fds[i], &potato, sizeof(potato), MSG_WAITALL);
                endGame(num_players, fds);
                break;
            }     
        }
        //Print the trace of the potato
        potato.printTrace(); 
    }
    //Close all the fds of every player
    for (int i = 0; i < num_players; i++) {
        close(fds[i]);
    }  
}

int main (int argc, char ** argv) {
    if (!checkArgc(argc, argv)) {
        return EXIT_FAILURE;
    }
    int num_players = atoi(argv[2]);
    if (num_players <= 1) {
        cout << "Number of players should be greater than 1." << endl;
        return EXIT_FAILURE;
    }
    int num_hops = atoi(argv[3]);
    if (num_hops < 0 || num_hops > 512) {
        cout << "Number of hops should be [0, 512]." << endl;
        return EXIT_FAILURE;
    }
    cout << "Potato Ringmaster" << endl;
    cout << "Players = " << num_players << endl;
    cout << "Hops = " << num_hops << endl;

    vector<int> fds(num_players);
    vector<int> ports(num_players);
    vector<string> ips(num_players);
    
    Server * master_server = new Server(argv[1], MASTER);
    connectPlayers(master_server, num_players, fds, ports, ips);
    circlePlayers(master_server, num_players, fds, ports, ips);
    startGame(num_hops, num_players, fds);
    delete master_server;
    return EXIT_SUCCESS;
}
