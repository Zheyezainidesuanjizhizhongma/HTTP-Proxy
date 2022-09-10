//
//  rsp.hpp
//  httpddl
//
//  Created by mengqi on 2/23/22.
//

#ifndef rsp_hpp
#define rsp_hpp

#include <stdio.h>
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
#include <ctime>
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
using namespace std;

class Response{
    std::string header;
    std::vector<char> fulltext;
 public:
    Response(){}
    //Response(v[0], v[1])
    Response(const Response& rhs) {
        header = rhs.header;
        fulltext = rhs.fulltext;
    }
    Response& operator=(const Response& rhs) {
        if(&rhs != this) {
            header = rhs.header;
            fulltext = rhs.fulltext;
        }
        return *this;
    }
    Response(std::vector<char> v) {
        fulltext.insert(fulltext.begin(), v.begin(), v.end());
        string tmp;
    tmp.insert(tmp.begin(), v.begin(), v.end());
    int pos = tmp.find("\r\n\r\n");
    header = tmp.substr(0, pos);
    }
  /*  Response(std::vector<char> _header, std::vector<char> v) {
    header.insert(header.begin(), _header.begin(), _header.end());
    fulltext.insert(fulltext.begin(), v.begin(), v.end());
    }*/
  std::string getRspFirstLine() {
      int pos = header.find("\r\n");
      return header.substr(0, pos);
   }
  std::string locate(std::string s);
  std::string getHeader(){return header;}
  std::vector<char> getResponse(){return fulltext;}
  std::string getStatus();
  std::string getDate();
  time_t getTime(std::string _time);
  std::string getMaxAge();
  std::string getLastModify();
  std::string getExpire();
  std::string getCacheControl();
  std::string getEtag();
  double getCurrentAge();
  bool is_no_cache();
  bool is_no_store();
  bool isFresh();
  bool needRevalidation();
  bool isPrivate();
  long getContentLength();
  bool isChunked();

};
#endif /* rsp_hpp */
