#include "cache.hpp"
#include "socket.hpp"
#include <cstdlib>
#include <fstream>
//#include <exception>
#include <iostream>
#include "global.hpp"
//#include "excpt.hpp"
//extern std::ofstream logFile;
extern Cache cache; 
struct serverCacheBundle{
    struct sockaddr_in clientAddr;
    int acceptFd;
  //    Cache * s_cache;
  //    pthread_mutex_t scb_lock;
    int tId;
};
/*class Failure: public exception{
private:
  string EM;
public:
  Failure(string em) : EM(em){}
  const char * what() const noexcept{
    return EM.c_str;
  }

  }*/
string currentTime();
void * handleFunction(void * args);
