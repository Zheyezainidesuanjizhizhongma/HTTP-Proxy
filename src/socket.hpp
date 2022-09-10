#include <iostream>
#include <string.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sstream>
#include <vector>
#include <netdb.h>
#include <algorithm>
#define MAX_TEXTLEN 65536 //一定要设置成2的倍数
#include "excpt.hpp"
//using namespace std;

struct serverBundle{
    struct sockaddr_in clientAddr;
    int acceptFd;
    int tId;
};

class Socket {
private:
    int sockfd;
public:
    int createSocket();
};

class Server: public Socket {
private:
    const char * server_ipAddr;
    unsigned int server_port;
public:
    Server(const char * ipAddr, unsigned int port): server_ipAddr(ipAddr), server_port(port) {}
    int connectListenFromServer();
    //std::vector<std::vector<char> > acceptProcessFromServer(int server_sockfd);
};


class Client: public Socket {
private:
    const char * server_ipAddr;
    unsigned int server_port;
public:
    Client(const char * ipAddr, unsigned int port): server_ipAddr(ipAddr), server_port(port) {}
    int connectFromClient(); 
    void sendFromClient(int client_sockfd, std::vector<char> client_text);
};

serverBundle acceptRequestFromClient(int server_sockfd);
std::vector<std::vector<char> > acceptProcessFromServer(serverBundle svBundle, int tID);
std::vector<std::vector<char> > parseHttpRequest(std::vector<char> client_text, int tID);
std::vector<std::vector<char> > sendDataByGetPost(int client_sockfd, int server_sockfd, int tID);
void sendDataByConnect(int clientSockfd, int accServerfd);
