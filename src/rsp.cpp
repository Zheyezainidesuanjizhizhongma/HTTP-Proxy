#include "global.hpp"
#include "rsp.hpp"
std::string Response::locate(std::string s){
  int p1 = header.find(s);
  if(p1 == -1) {return "";}
  int p2 = header.find("\r\n", p1 + 1);
    std::string res = header.substr(p1 + s.size() + 2, p2 - (p1 + s.size() + 2));
   // std::cout << "$$$$$$$" << res << "$$$$$$" << std::endl;
  return res;
}
std::string Response::getDate() {
  return locate("Date")=="" ? locate("date"): locate("Date");
  //  return locate("Date");
}

time_t Response::getTime(std::string _time) {
  tm mytime;
  int p = _time.find("GMT");
  if(p != -1) {
    _time.erase(p-1, 4);
  }
  strptime(_time.c_str(),"%a, %d %b %Y %H:%M:%S", &mytime);
  time_t ans = mktime(&mytime);
  return ans;
}

double Response::getCurrentAge() {
    std::string date = getDate();
    time_t responseDate = getTime(date);
    time_t now = time(NULL) + 18000;
  return difftime(now, responseDate);
}
std::string Response::getStatus(){
  int p1 = header.find(" ");
  int p2 = header.find(" ", p1 + 1);
    std::string ans = header.substr(p1+1, p2-p1-1);
  return ans;
}
std::string Response::getMaxAge(){
  //检测空格, 在cache control里面
    std::string cacheControl = getCacheControl();
  if(cacheControl == "") return "";
  int p1 = cacheControl.find("max-age");
  if(p1 == -1) return "";
  int p2 = cacheControl.find(" ", p1+1);
  if(p2 == -1) p2 = cacheControl.find("\r\n", p1+1);
  return cacheControl.substr(p1+8, p2-p1-8);
}
std::string Response::getLastModify() {
  return locate("Last-Modified")=="" ? locate("last-modified"):locate("Last-Modified");
  //return locate("Last-Modified");
}
std::string Response::getExpire() {
  return locate("Expires") == "" ? locate("expires"): locate("Expires");
}
std::string Response::getCacheControl() {
  return locate("Cache-Control")=="" ? locate("cache-control"):locate("Cache-Control");
  //  return locate("Cache-Control");
}
std::string Response::getEtag() {
  return locate("ETag")== "" ? locate("etag"): locate("ETag");
  //return locate("ETag");
}
bool Response::is_no_cache() {
    std::string cc = getCacheControl();
  if(cc == "") return false;
  int p = cc.find("no-cache");
  if(p == -1) return false;
  pthread_mutex_lock(&cache_lock); 
  logFile << "in cache, requires validation" << std::endl;
  pthread_mutex_unlock(&cache_lock); 
  return true;
}
bool Response::is_no_store() {
    std::string cc = getCacheControl();
  if(cc == "") return false;
  int p = cc.find("no-store");
  if(p == -1) return false;
  return true;
}
bool Response::isFresh() {
  bool fresh = true;
  //if max-age exist, compare max-age and current age
    std::string maxAge = getMaxAge();
  if(maxAge != "") {
    double currentAge = getCurrentAge();
    long max_age = atol(maxAge.c_str());
    fresh = currentAge < max_age;
    if(!fresh) {
      //放进logfile里
      pthread_mutex_lock(&cache_lock); 
      logFile << "in cache, requires validation" << std::endl;
      pthread_mutex_unlock(&cache_lock); 
	return false;
    }
  }
  //if expires exists, compare Expire time and current time
    std::string expiresline = getExpire();
  if(expiresline != "") {
    time_t expires = getTime(expiresline);
    time_t current = time(NULL) + 18000;
    fresh = current < expires;
    if(!fresh) {
      //放进logfile里,
      pthread_mutex_lock(&cache_lock); 
      logFile <<"in cache, but expired at "<<  expiresline  <<std::endl;
      pthread_mutex_unlock(&cache_lock); 
    }
	return fresh;
    
   }
//  // if Last modify exists, calculate the freshness time, and then compare freshness time and current age;
//
//    string lastModify = getLastModify();
//      if(lastModify != "") {
//        string lastModify = getLastModify();
//        time_t lastModified = getTime(lastModify);
//        string date = getDate();
//        time_t responseDate = getTime(date);
//        //freshness_lifetime =( ResponseTime - LastModified) * 10%
//        double freshness_lifetime = difftime(responseDate, lastModified) * 0.1;
//        double currentAge = getCurrentAge();
//        fresh       = currentAge < freshness_lifetime;
//        if(!fresh){
//          //放进logfile里   "in cache, requires validation"
//        }
//        return fresh;
//      }
      return fresh;
    }
    bool Response::needRevalidation(){
      cout << "need Revalidation." << endl;
      if(isFresh() && !is_no_cache()){
        if(getEtag()!="" ||getLastModify() != "") {
          cout << "in cache, etag/lastmodified revalidation "<<endl;
          pthread_mutex_lock(&cache_lock);
          logFile << "in cache, requires validation" << std::endl;
          pthread_mutex_unlock(&cache_lock);
        }
      }
      return !isFresh() || !is_no_cache() || getEtag() != "" || getLastModify() != "";
    }

    long Response::getContentLength() {
      std::string len = locate("Content-Length")=="" ? locate("content-length"):locate("Content-Length");
        std::cout << len << std::endl;
      long ans = atoi(len.c_str());
        std::cout << ans << std::endl;
      //long ans = stoi(len);
      return ans;
    }

    bool Response::isPrivate(){
        std::string cc = getCacheControl();
        if(cc == "") return false;
        int p = cc.find("private");
        if(p == -1) return false;
        return true;
    };
   
