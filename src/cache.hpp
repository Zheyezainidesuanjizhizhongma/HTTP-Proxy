#include <map>
#include <algorithm>
#include <mutex>
//#include <thread>
#include <pthread.h>
#include <sstream>
#include <vector>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <cstring>
#include "rsp.hpp"
#include "request.hpp"
//#include "all.hpp"
#define MAX_TEXTLEN 65536
//std::ofstream logFile("/home/ml607/http_2.21/proxy.log");
#include "global.hpp"
using namespace std;
//pthread_mutex_t cache_lock = PTHREAD_MUTEX_INITIALIZER;
class Cache {
  /*public:    pthread_t c_lock;
    class Node{
    public:
        Request input;
        Response rsp;
        //string header;//responseHeader
        //vector<char> content;//full_text
        Node * pre;
        Node * next;
        Node(): input(), rsp(),pre(NULL), next(NULL){}
        Node(Request input, Response rsp): input(input), rsp(rsp), pre(NULL), next(NULL){
            //content.insert(content.begin(), _content.begin(), _content.end());
        }
    };
    Node * head;
    Node * tail;
    size_t capacity;
    //key:first line, value: Node
    unordered_map<string, Node*> map;
   // int current_size;
   */
    size_t capacity;
    size_t cur_size;
    //key:first line, value: Node
    unordered_map<string, Response> map;
public:
  /*
    void printCache() {
        cout << "current Cache :"<< endl;
        Node * node = head->next;
        while(!node && node != tail) {
            cout << node->input.getHost() <<endl;
            cout << node->rsp.getRspFirstLine() << endl;
        }
        
	}*/
  void printCache() {
        cout << "current Cache :"<< endl;
        for(auto m : map) {
            cout << m.first << endl;
        }
    }
    //constructor
    Cache(size_t capacity): capacity(capacity){
      /*    head = new Node();
        tail = new Node();
        head->next = tail;
        tail->pre = head;*/
    }
    ~Cache() {
      /* while(head!=NULL) {
            Node* temp = head->next;
            delete head;
            head = temp;
	    }*/
    }
  /*
    void put(Request input, Response rsp) {
        //pthread_mutex_lock(&cache_lock);
        string key = input.getRequestLine();
        //if key exists
        if(inCache(input)) {
            Node * node  = map[key];
            node->rsp = rsp;
            //move to the head
            
        }
        else{// if not exist, create a new node and add it to the head;
            Node * newNode = new Node(input, rsp);
            map[input.getRequestLine()] = newNode;
            //add it to the head;
            newNode->pre = head;
            newNode->next = head->next;
            head->next = newNode;
            head->next->next->pre = newNode;
            //newNode->next->pre = newNode;
            if(map.size() > capacity) {
                //remove the last node;
                Node * last = tail->pre;
                last->pre->next = tail;
                last->next->pre = last->pre;
                map.erase(last->input.getRequestLine());
                delete last;
                
            }
        }
        //pthread_mutex_unlock(&cache_lock);
    }
    bool inCache(Request input) {
        const auto & it = map.find(input.getRequestLine());
        if(it == map.end()) return false;
        return true;
    }
    //if it is in cache, return fulltext
    Response get(Request input){
        //if exists, move the node to the front and return it
        string key = input.getRequestLine();
        Node * node  = map[key];
        //remove the node
        //再操作
        node->pre->next = node->next;
        node->next->pre = node->pre;
        //add it to the head
        node->pre = head;
        node->next = head->next;
        node->next->pre = node;
        head -> next = node;
        return node->rsp;
    }
  */
  bool inCache(Request input) {
        const auto & it = map.find(input.getRequestLine());
        if(it == map.end()) return false;
        return true;
    }
    void put(Request input, Response rsp) {
        string key = input.getRequestLine();
        //if key exists
        if(inCache(input)) {
            const auto & it = map.find(key);
            map.erase(it);
            map.insert(make_pair(key, rsp));
        }
        else{// if not exist, create a new node and add it to the head;
            map.insert(make_pair(key, rsp));
            cur_size++;
            if(cur_size > capacity) {
	      map.erase(map.begin());
	      pthread_mutex_lock(&cache_lock);
              logFile<< "(no-id): NOTE evicted " << input.getURL() << " from cache" << endl;
              pthread_mutex_unlock(&cache_lock);
            }
	}
    }
    
    //if it is in cache, return fulltext
    Response get(Request input){
        //if exists, move the node to the front and return it
        string key = input.getRequestLine();
        const auto & it = map.find(key);
	Response rsp = map[key];
	map.erase(it);
	map.insert(make_pair(key, rsp));
        //map.erase(it);
        
        return rsp;
    }
  bool storeResponse(Request input, Response rsp, int tId) {
        //200ok && x no-store && x private: needs store response
        if (rsp.getStatus() == "200" && !rsp.isPrivate() && !rsp.is_no_store()) {
            pthread_mutex_lock(&cache_lock);
            put(input, rsp);
            pthread_mutex_unlock(&cache_lock);
            //store response operation: <key, rsp> 加进cache里
            string expire = rsp.getExpire();
            if(expire.size()){
	      pthread_mutex_lock(&cache_lock);
	      logFile<< tId << ": cached, but expires at " << rsp.getExpire() << endl;
                //logfile:"cached, expires at" + "EXPIRETIME"
	      pthread_mutex_unlock(&cache_lock);
            }
	else{
              pthread_mutex_lock(&cache_lock);
	      //logfile:"cached, but required re-validation"
	      logFile << tId << ": cached, but requires re-validation"<< endl;
	      pthread_mutex_unlock(&cache_lock);
	    }
            return true;
        }
        //cannot store response
        //1. not 200 ok
	//pthread_mutex_lock(&cache_lock);
        if(rsp.getStatus() != "200"){
	  pthread_mutex_lock(&cache_lock);
	  //not cacheable + because not 200 ok
            logFile << tId << ": not cacheable, because not 200 ok" << endl;
	    pthread_mutex_unlock(&cache_lock);
	}
        else if(rsp.isPrivate()) {
	  pthread_mutex_lock(&cache_lock);
            //not cacheable + because is private
            logFile << tId << ": not cacheable, because is private" << endl;
	    pthread_mutex_unlock(&cache_lock);
	}
        else if(rsp.is_no_store()) {
	  pthread_mutex_lock(&cache_lock);
            //not cacheable + because is no-store
            logFile << tId << ": not cacheable, because is no-store" << endl;
	    pthread_mutex_unlock(&cache_lock);
	}
        return false;
    }
    //return updated response
  Response revalidation(Request input, Response rsp, int socket, int tId){
        string etag = rsp.getEtag();
	cout << "revalidation: response header:\n"<< rsp.getHeader() <<endl; 
	cout << "revalidation: etag:"<< etag << endl;
        string lastModify = rsp.getLastModify();
	cout <<	"revalidation: lastModify:"<< lastModify << endl;
        string newinput = input.getRequestLines();
        //1. if has etag, send "if-none-match"
    if(etag != "") {
            //string add = "If-None-Match: " + etag + string("\r\n");
            //replacedInput.insert(replacedInput.end() - 2, add.begin(), add.end());
      cout << "I'm here. Etag exists. Adding If-none-match header." << endl;
      string adder = string("If-None-Match: ") + etag + string("\r\n");
            cout <<"Etag: "<< etag << endl;
            newinput = newinput.insert(newinput.length()-2, adder);
            //send(socket,message.data(),message.size()+1,0);
        //  cout << "Newinput :  \n" << newinput.data() << endl;
        }
        //2. if has last-modify, send if-modified-since
        if(lastModify != "") {
	  cout << "I'm here. lastModify exists. Adding If-Modified-Since header." << endl;
            string adder = string("If-Modified-Since: ") + lastModify + string("\r\n");
            newinput = newinput.insert(newinput.length()-2, adder);
            //replacedInput = input + string("\r\n") + string("If-Modified-Since: ") + lastModify + string("\r\n");
            
        }
        cout << "Newinput :  \n" << newinput.data() <<"newinput end_______"<< endl;
        const char * newinput_ch = newinput.c_str();
        int send_flag = send(socket, newinput_ch, strlen(newinput_ch), 0);
    if(send_flag <= 0) {
      cout << "failed to send the new request." << endl;
      exit(EXIT_FAILURE);
    }
    logFile << tId << ": Requesting “" << input.getRequestLine() << "” from " << input.getHost() << endl;
        //recieve new response
    /*        vector<char> v;
    v.resize(65536);
        int recv_flag = recv(socket, &v, sizeof(v),0);
    */
    char server_text[MAX_TEXTLEN];
    memset(server_text,'\0',MAX_TEXTLEN);
    int recv_flag = recv(socket, server_text, MAX_TEXTLEN, 0);
    if(recv_flag <= 0) {
          cout << "failed to send the new request." << endl;
      exit(EXIT_FAILURE);
        }
    //cout << "Received “" << .getRequestLine() << "” from " << input.getURL() << endl;
    int sum = 0;
       std::vector<char> first_text(server_text, server_text+MAX_TEXTLEN);
       const char *crlf2 = "\r\n\r\n";
       std::vector<char>::iterator it = std::search(first_text.begin(), first_text.end(), crlf2, crlf2+strlen(crlf2));
       std::string responseHeader(first_text.begin(), it+strlen(crlf2));
       //std::cout << responseHeader << "\n";
       std::vector<char> v_responseHeader(responseHeader.begin(), responseHeader.end());
       //proxy_responseInfo.push_back(v_responseHeader);
       std::vector<char> full_text;
       //cout << "^^^^^^^^^^^^^^New response:" << endl;
       do {
            //std::cout << "recv_flag: " << recv_flag << "\n";
            sum += recv_flag;
            //server_text.resize(ret_recv);
            //send(server_sockfd, server_text, recv_flag, 0);
            //        std::cout << server_text;
            full_text.insert(full_text.end(), server_text, server_text+recv_flag);
            memset(server_text,'\0',MAX_TEXTLEN); //清零
        } while((recv_flag=recv(socket, server_text, MAX_TEXTLEN, 0))>0);

    
       cout << "$$$$$$$^^^^^^^^^^^^^^New response: \n" << full_text.data() << "^^^^^^^^^^^^^^^^$$$$$$$$$$$" <<endl;
    // Response newResponse(v);
        Response new_rsp(full_text);
	pthread_mutex_lock(&cache_lock);
        logFile << tId << ": Received “" << new_rsp.getRspFirstLine() << "” from " << input.getHost() << endl;
	pthread_mutex_unlock(&cache_lock);
	if(new_rsp.getStatus() == "304") {
      return rsp;
        }
        else{
	  storeResponse(input, new_rsp, tId);//input: Request, Response
            return new_rsp;
        }
//        //neither exists
//        else{
//            //simply resend the request
//            string orginput = input.getRequestHeader();
//            send(socket, orginput.data(), orginput.size(), 0);
//            //recieve new response
//            vector<char> v;
//            recv(socket, &v, 100000,0);
//            Response new_rsp(v);
//            //size_t p1 = input.find(' ');
//            //size_t p2 = input.find(' ', p1+1);
//            storeResponse( input, new_rsp);
//            return new_rsp;
//        }
    }
    
    
};
