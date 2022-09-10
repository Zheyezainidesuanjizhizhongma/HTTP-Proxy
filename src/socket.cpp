#include "socket.hpp"
#include <cstdlib>
#include "global.hpp"

int Socket::createSocket(){
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        
        if(sockfd == -1){
            throw Failure("can't create a socket.");
            
        }

        return sockfd;
}

int Server::connectListenFromServer() {
    //进行连接
    int server_sockfd = createSocket();
    std::cout << "server_sockfd=" << server_sockfd << "\n";

    //发送客户端连接请求，封装服务器信息
    struct sockaddr_in server_addr;
    socklen_t len_addr = sizeof(server_addr);

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(server_ipAddr); //inet_network
    server_addr.sin_port = htons(server_port);

    //设置端口复用
    int opt = 1;
    int ret_setsock = setsockopt(server_sockfd, SOL_SOCKET,SO_REUSEADDR, (const void *)&opt, sizeof(opt));
    if(ret_setsock == -1){
        throw Failure("The server can't set the socket successfully");
        
    }

    //将socket与服务器网络信息结构体绑定
    int ret_bind = bind(server_sockfd, (struct sockaddr *)&server_addr, len_addr);
    if(ret_bind == -1){
        throw Failure("The server can't bind with the socket");
    }

    //将socket设置为被动监听状态
    int ret_listen = listen(server_sockfd, 20); //设置监听上限
    if(ret_listen == -1){
        throw Failure("The server can't set the socket to listen");
    }

    std::cout << "listen success\n"; 

    return server_sockfd;
}

//接收客户端的连接
serverBundle acceptRequestFromClient(int server_sockfd){
    //阻塞等待客户端的连接accept
    struct sockaddr_in client_addr;
    socklen_t len_addr = sizeof(client_addr);

    int acceptfd = accept(server_sockfd, (struct sockaddr *)&client_addr, &len_addr);
    std::cout << "accept success\n";
    if(acceptfd == -1){
        throw Failure("The server can't get the accept from the client");
        
    }

    serverBundle sb;
    sb.clientAddr = client_addr;
    sb.acceptFd = acceptfd;

    return sb;
}

//接收客户端的连接，然后去处理连接
std::vector<std::vector<char> > acceptProcessFromServer(serverBundle svBundle, int tID){
    struct sockaddr_in client_addr = svBundle.clientAddr;
    int acceptfd = svBundle.acceptFd;
    std::vector<std::vector<char> > acceptInfo;

    //打印连接的客户端的信息
    std::cout << "ip:" << inet_ntoa(client_addr.sin_addr) << ", port:" << ntohs(client_addr.sin_port) << "\n";
    char * ip_addr = inet_ntoa(client_addr.sin_addr);
    std::vector<char> v_ip_addr(ip_addr, ip_addr+strlen(ip_addr));
    //v_ip_addr.push_back('\0');
    //添加真正服务端的IP地址
    acceptInfo.push_back(v_ip_addr); //v_ip_addr: 0

    //进行通信
    //接收数据
    char client_text[MAX_TEXTLEN];
    int ret_recv;
    memset(client_text,'\0',MAX_TEXTLEN); 
    ret_recv = recv(acceptfd, client_text, MAX_TEXTLEN, 0);

    if(ret_recv == 0){
      const char * badRequest = "HTTP/1.1 400 Bad Request\r\n\r\n";
      pthread_mutex_lock(&cache_lock);
      logFile<< tID << ": Responding HTTP/1.1 400 Bad Request" << "\n";
      pthread_mutex_unlock(&cache_lock);
      send(acceptfd, badRequest, strlen(badRequest), 0);
      throw Failure("HTTP/1.1 400 Bad Request.");
    }
       
    if(ret_recv == -1){
        throw Failure("The server can't receive data from the client");
    }

    // full_client_text.insert(full_client_text.end(), client_text, client_text+ret_recv);
    // full_client_text.push_back('\0');
    std::vector<char> full_client_text(client_text, client_text+ret_recv);
    std::cout << "from client: " << full_client_text.data() << "\n";
    acceptInfo.push_back(full_client_text); //full_client_text: 1

    return acceptInfo;
}

int Client::connectFromClient() {
    //进行连接
    int client_sockfd = createSocket();
    std::cout << "client_sockfd=" << client_sockfd << "\n";

    //发送客户端连接请求，封装服务器信息
    struct sockaddr_in server_addr;
    socklen_t len_addr = sizeof(server_addr);

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(server_ipAddr);  //inet_network
    server_addr.sin_port = htons(server_port);

    int ret_connect = connect(client_sockfd, (struct sockaddr *)&server_addr, len_addr);
    if(ret_connect == -1){
        throw Failure("The client can't connect to the server");
    }
    std::cout << "Connected!!!\n";

    return client_sockfd;   
}

void Client::sendFromClient(int client_sockfd, std::vector<char> client_text){
    //进行通信
    //发送数据
    int ret_send = send(client_sockfd, client_text.data(), client_text.size(), 0);

    std::cout << "client_text.data(): " << client_text.data() << "-----------------------"<<"\n";
    std::cout << "client_text.size(): " << client_text.size() << "-----------------------"<<"\n";


    if(ret_send == -1){
        throw Failure("The client can't send text");
    }

}

std::vector<std::vector<char> > parseHttpRequest(std::vector<char> full_client_text, int tID){
    std::vector<std::vector<char> > proxy_serverInfo;

    const char * crlf = "\r\n";
    std::vector<char>::iterator it = std::search(full_client_text.begin(), full_client_text.end(), crlf, crlf+strlen(crlf));
    std::vector<char> requestLine(full_client_text.begin(), it);
    //requestLine.push_back('\0');
    std::vector<char> requestHeader(it, full_client_text.end());
    std::cout << "the request line is: " << requestLine.data() << "\n";
    std::cout << "the request header is: " << requestHeader.data() << "\n";

    std::string requestLine_str = requestLine.data();
    std::stringstream ss(requestLine_str);
    std::string word;
    std::vector<std::string> requestInfo;
    while(ss >> word){
        requestInfo.push_back(word);
    }

    //获取method
    std::string method_str = requestInfo[0]; //要判断是GET,POST,CONNECT, GET=0, POST=1, CONNECT=2
    std::vector<char> v_method;
    //添加请求行的method
    if(method_str.compare("GET") == 0){
        v_method.push_back(0);
    }
    else if(method_str.compare("POST") == 0){
        v_method.push_back(1);
    }
    else if(method_str.compare("CONNECT") == 0){
        v_method.push_back(2);
    }
    else{
        pthread_mutex_lock(&cache_lock);
        logFile<< tID << ": Responding HTTP/1.1 400 Bad Request" << "\n";
        pthread_mutex_unlock(&cache_lock);
        throw Failure("HTTP/1.1 400 Bad Request: The request line method is not included.");
    }
    proxy_serverInfo.push_back(v_method);  //v_method: 0

    //获取protocol以及后面的内容
    std::string proto_str = requestInfo[2];

    if((method_str.compare("GET") == 0) || (method_str.compare("POST") == 0)){
        //获取hostname和filename
        std::string url_str = requestInfo[1];
        std::size_t pos_url = url_str.find("//");
        std::string url_host_file = url_str.substr(pos_url+2);
        std::cout << "url_host_file is :" << url_host_file << "\n";

        //找到hostname
        std::size_t pos_host = url_host_file.find("/");
        std::string url_host = url_host_file.substr(0, pos_host);
        //找到filename, 如果只有host没有file，则用/来表示
        std::string url_file = url_host_file.substr(pos_host);
        std::cout << "url_host = " << url_host << "\n";
        std::cout << "url_file = " << url_file << "\n";

        std::stringstream ss_requestline;
        ss_requestline << method_str << " " << url_file << " " << proto_str;
        std::string requestServerInfo = ss_requestline.str();
        std::cout << "requestServerInfo is: " << requestServerInfo << "\n";
        std::vector<char> v_requestServerInfo(requestServerInfo.begin(), requestServerInfo.end());
        v_requestServerInfo.insert(v_requestServerInfo.end(), requestHeader.begin(), requestHeader.end());
        //添加要发给真正客户端的请求行加请求体
        proxy_serverInfo.push_back(v_requestServerInfo); //v_requestServerInfo: 1

        struct hostent * ipInfo = gethostbyname(url_host.c_str());
        //std::cout << ipInfo->h_length << " " << ipInfo->h_addr_list << "\n";

        char * ip_addr = inet_ntoa(*(struct in_addr *)*ipInfo->h_addr_list);
        std::cout << "ip address:"<< ip_addr << "\n";
        std::vector<char> v_ip_addr(ip_addr, ip_addr+strlen(ip_addr));
        //v_ip_addr.push_back('\0');
        //添加真正服务端的IP地址
        proxy_serverInfo.push_back(v_ip_addr); //v_ip_addr: 2

    }
    else if(method_str.compare("CONNECT") == 0){
        std::string url_host = requestInfo[1];
        std::size_t pos_url = url_host.find(":");
        std::string host_name = url_host.substr(0, pos_url);
        std::cout << "=====host_name: " << host_name << "\n";

        struct hostent * ipInfo = gethostbyname(host_name.c_str());

        char * ip_addr = inet_ntoa(*(struct in_addr *)*ipInfo->h_addr_list);
        std::cout << "ip address:"<< ip_addr << "\n";
        std::vector<char> v_ip_addr(ip_addr, ip_addr+strlen(ip_addr));
        //v_ip_addr.push_back('\0');
        //添加真正服务端的IP地址
        proxy_serverInfo.push_back(v_ip_addr);
    }

    return proxy_serverInfo;
    
}


std::vector<std::vector<char> > sendDataByGetPost(int client_sockfd, int server_sockfd, int tID){
    std::vector<std::vector<char> > proxy_responseInfo;

    //接收数据
    char server_text[MAX_TEXTLEN];
    memset(server_text,'\0',MAX_TEXTLEN);

    // std::vector<char> server_text(MAX_TEXTLEN);
    // server_text.clear();
    int ret_recv = recv(client_sockfd, server_text, MAX_TEXTLEN, 0);
    int sum = 0;
    

    if(ret_recv == -1){
        pthread_mutex_lock(&cache_lock);
        logFile<< tID << ": Responding HTTP/1.1 502 Bad Gateway" << "\n";
        pthread_mutex_unlock(&cache_lock);
        throw Failure("HTTP/1.1 502 Bad Gateway: The client can't receive data from the server");
    }

    std::vector<char> first_text(server_text, server_text+MAX_TEXTLEN);
    const char *crlf2 = "\r\n\r\n";
    std::vector<char>::iterator it = std::search(first_text.begin(), first_text.end(), crlf2, crlf2+strlen(crlf2));
    std::string responseHeader(first_text.begin(), it+strlen(crlf2));
    std::cout << responseHeader << "\n";
    std::vector<char> v_responseHeader(responseHeader.begin(), responseHeader.end());
    proxy_responseInfo.push_back(v_responseHeader);

    std::vector<char> full_text;

    // //如果是Content-Length，就直接全文整个传输过去
    // std::transform(responseHeader.begin(), responseHeader.end(), responseHeader.begin(), ::tolower);
    // std::string contentLength("content-length: ");
    // int pos_content = responseHeader.find(contentLength);
    // if(pos_content != responseHeader.npos){
        do {
            std::cout << "ret_recv: " << ret_recv << "\n";
            sum += ret_recv;
            //server_text.resize(ret_recv);
            send(server_sockfd, server_text, ret_recv, 0); 
            //std::cout << server_text;
            full_text.insert(full_text.end(), server_text, server_text+ret_recv);
            memset(server_text,'\0',MAX_TEXTLEN); //清零
        } while((ret_recv=recv(client_sockfd, server_text, MAX_TEXTLEN, 0))>0);
    // }
   
    proxy_responseInfo.push_back(full_text);
    std::cout << "========" << sum << "\n"; 
    std::cout << ret_recv << "\n";
    std::cout << errno << "\n";
    //std::cout << full_text.data();
   
    return proxy_responseInfo;
}

void sendDataByConnect(int client_fd, int server_fd){
    //receive and send bytes back and forth between the client/server
    fd_set readset;
    int fd_list[2] = {server_fd, client_fd};
    int maxfd;
    if(client_fd < server_fd){
        maxfd = server_fd;
    }
    else{
        maxfd = client_fd;
    }

    while (1) {
        FD_ZERO(&readset);
        FD_SET(server_fd, &readset);
        FD_SET(client_fd, &readset);

        select(maxfd+1, &readset, NULL, NULL, NULL);
    
        for (int i = 0; i < 2; i++) {
            char buffer[MAX_TEXTLEN];
            if (FD_ISSET(fd_list[i], &readset)) {
                memset(buffer, 0, MAX_TEXTLEN);
                int ret_recv = recv(fd_list[i], buffer, MAX_TEXTLEN, 0);
                if(ret_recv == 0){
                    FD_CLR(fd_list[i], &readset);
                    return;
                }
        
                if(ret_recv == -1){
                    throw Failure("can't read successfully");
                }

                int ret_send = send(fd_list[1 - i], buffer, ret_recv, 0);

                if(ret_send == 0){
                    FD_CLR(fd_list[i], &readset);
                    return;
                }
        
                if(ret_send == -1){
                    throw Failure("can't send successfully");
                    
                }
            }
        }
  }

}
