#include <exception>
#include <stdexcept>
#include <string>
//using namespace std;
class Failure: public std::exception{
private:
  std::string EM;
public:
  Failure(std::string em) : EM(em){}
  const char * what() const noexcept{
    return EM.c_str();
  }
};
