#ifndef ROTFILE_CHANNEL_H_XVR88EAX

#define ROTFILE_CHANNEL_H_XVR88EAX

#include <unistd.h>
#include "../logxx.hpp"

namespace logxx {

class rotfile_channel : public basic_channel {
public:
  rotfile_channel(const char* filename) 
    : file_(filename, std::ios_base::app | std::ios_base::out ) 
    , filename_(filename) {}

  std::ostream& stream() { 
    if(access(filename_.c_str(), F_OK)) {
      file_.close();
      file_.open(filename_.c_str(), std::ios_base::app | std::ios_base::out);
    }
    return file_; 
  }

  std::ostream& flush() { 
    return file_.flush(); 
  }

private:
  std::ofstream file_;
  std::string filename_;
};

};




#endif /* end of include guard: ROTFILE_CHANNEL_H_XVR88EAX */
