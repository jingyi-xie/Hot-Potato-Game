#include <vector>
#include <string>
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <cstring>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>

using namespace std;
class Player {    
    public:
        int status; 
        int player_id;
        int master_fd;
        int prev_fd;
        int next_fd;
        int port_num;
        char ip[100];
        Player() {
            port_num = -1;
            master_fd = -1;
            prev_fd = -1;
            next_fd = -1;
            status = -1;
            player_id = -1;
            memset(ip, 0, sizeof(ip));
        }
        //Connect player with ringmaster
        //Reference: TCP example
        void connectMaster(Server * player_server, const char *machine_name, const char* port_num, int &socket_fd, int &num_players, int &id) {
            struct addrinfo host_info;
            struct addrinfo *host_info_list;

            memset(&host_info, 0, sizeof(host_info));
            host_info.ai_family = AF_UNSPEC;
            host_info.ai_socktype = SOCK_STREAM;

            status = getaddrinfo(machine_name, port_num, &host_info, &host_info_list);
            if (status != 0) {
                cerr << "Error: cannot get address info for host" << endl;
                exit(EXIT_FAILURE);
            }
            socket_fd = socket(host_info_list->ai_family, host_info_list->ai_socktype, host_info_list->ai_protocol);
            if (socket_fd == -1) {
                cerr << "Error: cannot create socket" << endl;
                exit(EXIT_FAILURE);
            }
            status = connect(socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
            if (status == -1) {
                cerr << "Error: cannot connect to socket" << endl;
                exit(EXIT_FAILURE);
            }
            //Receive #players and the id of current player
            recv(socket_fd, &num_players, sizeof(num_players), 0);
            recv(socket_fd, &id, sizeof(id), 0);
            //Send the port of current player to ringmaster, helpful to circle the players
            int myPort = player_server->getPlayerPort();
            send(socket_fd, &myPort, sizeof(myPort), 0);
            master_fd = socket_fd;
            player_id = id;
            std::cout << "Connected as player " << id << " out of " << num_players << " total players" << endl;
            freeaddrinfo(host_info_list);
        }

        //Connect with the server of other players
        //Reference: TCP example
        void connectPlayerServer(const char *ip, const char *port, int &socket_fd) {
            struct addrinfo host_info;
            struct addrinfo *host_info_list;

            memset(&host_info, 0, sizeof(host_info));
            host_info.ai_family = AF_UNSPEC;
            host_info.ai_socktype = SOCK_STREAM;

            status = getaddrinfo(ip, port, &host_info, &host_info_list);
            if (status != 0) {
                cerr << "Error: cannot get address info for host" << endl;
                exit(EXIT_FAILURE);
            }
            socket_fd = socket(host_info_list->ai_family, host_info_list->ai_socktype, host_info_list->ai_protocol);
            if (socket_fd == -1) {
                cerr << "Error: cannot create socket" << endl;
                exit(EXIT_FAILURE);
            }
            status = connect(socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
            if (status == -1) {
                cerr << "Error: cannot connect to socket" << endl;
                exit(EXIT_FAILURE);
            }
            freeaddrinfo(host_info_list);
        }

        //Connect other players
        void connectPlayers(Server * player_server, int &ringmaster_fd) {          
            Player next_player;
            char port[100];
            //Receive from ringmaster the next player to connect
            recv(ringmaster_fd, &next_player, sizeof(next_player), MSG_WAITALL);
            sprintf(port, "%d", next_player.port_num);
            // std::cout << "next ip is " << next_player.ip << endl;
            // std::cout << "next port is " << next_player.port_num << endl;
            connectPlayerServer(next_player.ip, port, next_fd);
            //Accept the connection from other players
            struct sockaddr_storage socket_addr;
            socklen_t socket_addr_len = sizeof(socket_addr);
            prev_fd = accept(player_server->getFd(), (struct sockaddr *)&socket_addr, &socket_addr_len);
            if (prev_fd == -1) {
                cerr << "Error: cannot accept connection on socket" << endl;
                exit(EXIT_FAILURE);
            }

        }

        void playGame(int num_players) {
            //Create a new potato
            Potato potato(0);
            fd_set readfds;
            srand((unsigned int)time(NULL) + player_id);
            while(1) {
                //Clear all entries in the set
                FD_ZERO(&readfds);
                //Add the fds of ringmaster and the two neighbors to the set
                FD_SET(master_fd, &readfds);
                FD_SET(prev_fd, &readfds);
                FD_SET(next_fd, &readfds);
                select(max(prev_fd, next_fd) + 1, &readfds, NULL, NULL, NULL);
                //If master fd is ready for listenning
                if (FD_ISSET(master_fd, &readfds)) {
                    recv(master_fd, &potato, sizeof(potato), MSG_WAITALL);
                    if (potato.hops_remain == 0) {
                        return;
                    }
                    // cout << "I'm the first one" << endl;
                    // cout << "remain: " << potato.hops_remain << endl;
                }
                //If the previous fd is ready for listenning
                else if (FD_ISSET(prev_fd, &readfds)) {
                    recv(prev_fd, &potato, sizeof(potato), MSG_WAITALL);
                    if (potato.hops_remain == 0) {
                        return;
                    }
                    // cout << "remain: " << potato.hops_remain << endl;
                }
                //If the next fd is ready for listenning
                else if (FD_ISSET(next_fd, &readfds)) {
                    recv(next_fd, &potato, sizeof(potato), MSG_WAITALL);
                    if (potato.hops_remain == 0) {
                        return;
                    }
                    // cout << "remain: " << potato.hops_remain << endl;
                }
                //Add id of current player to the potato's trace
                potato.trace[potato.hops_total - potato.hops_remain] = player_id;
                potato.hops_remain--;
                //If #hops is 0, current player is "it"
                if (potato.hops_remain == 0) {
                    send(master_fd, &potato, sizeof(potato), 0);
                    cout << "I'm it" << endl;
                    continue;
                }
                //Randomly choose the next player to send the potato         
                int dest = rand() % 2;
                if (dest == 0) {
                    send(prev_fd, &potato, sizeof(potato), 0);
                    cout << "Sending potato to " << (player_id - 1 + num_players) % num_players << endl;
                }
                else {
                    send(next_fd, &potato, sizeof(potato), 0);
                    cout << "Sending potato to " << (player_id + 1) % num_players << endl;
                }
            }
        }

        void endGame() {
            close(master_fd);
            close(prev_fd);
            close(next_fd);
        }
};