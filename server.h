#include <string>
#include <cstdlib>
#include <iostream>
#include <cstring>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#define MASTER 0
#define PLAYER 1

using namespace std;
class Server {
    private:
        int status;
        int socket_fd;
        int client_connection_fd;

        struct addrinfo host_info; //hints
        struct addrinfo *host_info_list;
    public:

        //Constructor of the Server
        Server(char * port, int CASE) {
            status = -1;
            socket_fd = -1;
            client_connection_fd = -1;
            createServer(port, CASE);
        }

        //Deconstructor of the server
        ~Server() {
            close(socket_fd);
        }

        //Set the memory
        void setMemory() {
            memset(&host_info, 0, sizeof(host_info));
            host_info.ai_family = AF_UNSPEC; 
            host_info.ai_socktype = SOCK_STREAM;
            host_info.ai_flags = AI_PASSIVE;
        }

        //Setup the socket
        //Reference: TCP example
        void socketSetUp() {
            socket_fd = socket(host_info_list->ai_family, host_info_list->ai_socktype, host_info_list->ai_protocol);
            if (socket_fd == -1) {
                cerr << "Error: cannot create socket" << endl;
                exit(EXIT_FAILURE);
            }
            int yes = 1;
            status = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
            status = bind(socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
            if (status == -1) {
                cerr << "Error: cannot bind socket" << endl;
                exit(EXIT_FAILURE);
            }
            status = listen(socket_fd, 100);
            if (status == -1) {
                cerr << "Error: cannot listen on socket" << endl;
                exit(EXIT_FAILURE);
            }
            freeaddrinfo(host_info_list); 
        }

        //Create a server: ringmaster/player server
        void createServer(char * port, int CASE) {
            setMemory();
            status = getaddrinfo(NULL, port, &host_info, &host_info_list);
            if (status != 0) {
                cerr << "Error: cannot get address info for host" << endl;
                exit(EXIT_FAILURE);
            }
            if (CASE == PLAYER) {
                ((struct sockaddr_in *)(host_info_list->ai_addr))->sin_port = 0;
                // struct sockaddr_in *s = (struct sockaddr_in *)(host_info_list->ai_addr);
                // s->sin_port = 0;
            }
            socketSetUp();
        }

        //GEt the port of player
        int getPlayerPort() {
            struct sockaddr addr;
            socklen_t size = sizeof(addr);
            int result = getsockname(socket_fd, &addr, &size);
            if (result != 0) {
                cerr << "Error: getsockname" << endl;
                exit(EXIT_FAILURE);
            }
            return ntohs(((sockaddr_in*)&addr)->sin_port);
        }

        //Accept
        //Reference: TCP example
        int acceptConnect(string &ip) {
            struct sockaddr_storage socket_addr;
            socklen_t socket_addr_len = sizeof(socket_addr);
            client_connection_fd = accept(socket_fd, (struct sockaddr *)&socket_addr, &socket_addr_len);
            if (client_connection_fd == -1) {
                cerr << "Error: cannot accept connection on socket" << endl;
                exit(EXIT_FAILURE);
            } 
            char buffer[INET_ADDRSTRLEN];
            ip = inet_ntop(socket_addr.ss_family, &(((struct sockaddr_in *)&socket_addr)->sin_addr), buffer, INET_ADDRSTRLEN);
            return client_connection_fd;         
        }

        int getFd() {
            return this->socket_fd;
        }
     
};