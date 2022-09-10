#include "all.hpp"
#include "global.hpp"
std::ofstream logFile("/home/ly161/ECE568/ERSSHW2/newhw2/proxy.log");
Cache cache(3); 
pthread_mutex_t cache_lock = PTHREAD_MUTEX_INITIALIZER;
string currentTime() {
    time_t now = time(0);
    char * dt = ctime(&now);
    tm *gmtm = gmtime(&now);
    dt = asctime(gmtm);
    return string(dt);
    //cout << string(dt) << endl;
}

void * handleFunction(void * args){
  try{
    serverCacheBundle svcBundle = *((serverCacheBundle *)args);
    serverBundle svBundle;
    svBundle.acceptFd = svcBundle.acceptFd;
    svBundle.clientAddr = svcBundle.clientAddr;
    svBundle.tId = svcBundle.tId;
    //Cache * cache = (svcBundle.s_cache);
    //pthread_mutex_t cache_lock = svcBundle.scb_lock;
    
    std::vector<std::vector<char> > accInfo = acceptProcessFromServer(svBundle, svBundle.tId);
    int accServerfd = svBundle.acceptFd;
    std::vector<char> clientIpAddr = accInfo[0];
    std::vector<char> clientText = accInfo[1];
    std::vector<std::vector<char> > requestInfo = parseHttpRequest(clientText, svBundle.tId);
    int method = requestInfo[0][0];
    std::vector<char> requestLines;
    std::vector<char> serverIpAddr;
    //    std::cout << "!!!!!!! method is: " << method << "\n";
    int port;
    if(method==0 || method==1){ //GET, POST
      
      port = 80;
      requestLines = requestInfo[1];
      serverIpAddr = requestInfo[2];
      std::cout << "serverIpAddr: " << serverIpAddr.data() << "\n";
      std::cout << "!!!!!!! method is: " << method << "\n";    
    }
    else{ //CONNECT
        port = 443;
        serverIpAddr = requestInfo[1];
        std::cout << "serverIpAddr: " << serverIpAddr.data() << "\n";
        std::cout << "!!!!!!! method is: " << method << "\n"; 
    }

    Client c(serverIpAddr.data(), port);
    int clientSockfd = c.connectFromClient(); 

    Request input(clientText);
    cout << "@@@@@@ request: " <<input.getRequestLines() << "@@@@@@@" << endl;
    pthread_mutex_lock(&cache_lock);
    logFile << svBundle.tId << ": " << input.getRequestLine() << " from " << clientIpAddr.data() << " @ " << currentTime();
    pthread_mutex_unlock(&cache_lock);
    if(method==0 || method==1){
      c.sendFromClient(clientSockfd, requestLines); 
      if(method==0){
          if((cache).inCache(input)) {
            cout << "~~~~~~~~~~~in cache~~~~~~~~~~~~" << endl;
            pthread_mutex_lock(&cache_lock);
            Response rsp = cache.get(input);
            pthread_mutex_unlock(&cache_lock);
            cout << "----------------old response header:"<< rsp.getHeader()<< "\n------------"<< endl;

            if(rsp.needRevalidation()) {
                pthread_mutex_lock(&cache_lock);
                logFile << svBundle.tId << ": ";
                pthread_mutex_unlock(&cache_lock);
                rsp = (cache).revalidation(input, rsp, clientSockfd, svBundle.tId);
            }
            else {
                //logfile
                pthread_mutex_lock(&cache_lock);
                logFile << svBundle.tId << ": in cache,valid" <<endl;
                pthread_mutex_unlock(&cache_lock);
            }

            logFile << svBundle.tId << ": Responding " << rsp.getRspFirstLine() << endl;
            send(accServerfd, rsp.getResponse().data(), rsp.getResponse().size(), 0);
          } 
          else{
            pthread_mutex_lock(&cache_lock); 
            logFile << svBundle.tId << ": not in cache." << endl;
            logFile << svBundle.tId << ": Requesting “" << input.getRequestLine() << "” from " << input.getHost() << endl;
            pthread_mutex_unlock(&cache_lock); 
            std::vector<std::vector<char> > responseInfo = sendDataByGetPost(clientSockfd, accServerfd, svBundle.tId);
            std::vector<char> responseLines = responseInfo[0];
            std::vector<char> responseBody = responseInfo[1];
            cout << "***response_fulltext: " << responseBody.data() << "**********" <<endl;
            Response newrsp(responseBody);
            pthread_mutex_lock(&cache_lock);
            logFile << svBundle.tId << ": Received “" << newrsp.getRspFirstLine() << "” from " << input.getHost() << endl;
            pthread_mutex_unlock(&cache_lock);
            (cache).storeResponse(input, newrsp, svBundle.tId);

            pthread_mutex_lock(&cache_lock);
            logFile << svBundle.tId << ": Responding " << newrsp.getRspFirstLine() << endl;
            pthread_mutex_unlock(&cache_lock);
          }
      }
      else{
        c.sendFromClient(clientSockfd, requestLines);
        std::vector<std::vector<char> > responseInfo = sendDataByGetPost(clientSockfd, accServerfd, svBundle.tId);
        std::vector<char> responseLines = responseInfo[0];
        std::vector<char> responseLinesBody = responseInfo[1];
        std::cout << "---------hello" << "\n";
        std::cout << "---------responseLinesBody: " << responseLinesBody.data() << "\n";
      } 

    }
    else{
      const char * responseLine = "HTTP/1.1 200 OK\r\n\r\n";
      pthread_mutex_lock(&cache_lock);
      logFile << svBundle.tId << ": Responding " << "HTTP/1.1 200 OK" << endl;
      pthread_mutex_unlock(&cache_lock);
      send(accServerfd, responseLine, strlen(responseLine), 0);
      sendDataByConnect(clientSockfd, accServerfd);
      //do something
    }
    
    //close(accServerfd);
    //close(clientSockfd);
  }
  catch(std::exception &e){
      std::cout << "!!!!catch" << "\n";
  }

  return NULL;
}
