#include "all.hpp"

int main(void){
    Server s("0.0.0.0", 12345);
    int serverSockfd = s.connectListenFromServer();
    int tId = 0;
    ofstream logfile("/var/log/erss/proxy.log");
    //    Cache cache(1000);
     pthread_t threadId;
     while(1){
       try{
	       tId++;
          serverBundle sv = acceptRequestFromClient(serverSockfd);
          serverCacheBundle svc;
          svc.clientAddr = sv.clientAddr;
          svc.acceptFd = sv.acceptFd;
          // svc.s_cache = &cache;
          //svc.scb_lock = PTHREAD_MUTEX_INITIALIZER;
          svc.tId = tId;
          //handleFunction(&svc);
          pthread_create(&threadId, NULL, handleFunction, &svc);
          //pthread_detach(threadId);
       }
       catch(Failure & excp){
          cerr << tId <<": "<< excp.what() << endl;
          continue;
       }

       //pthread_detach(threadId);
    }

    return EXIT_SUCCESS;
}
