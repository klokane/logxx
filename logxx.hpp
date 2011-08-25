#ifndef LOGXX_5JKQSPQI
// vi:cin:et:sw=2 ts=2
#define LOGXX_5JKQSPQI

//  Copyright Jiri Kratochvil (aka KLoK) 2010
//  Distributed under the Boost Software License, Version 1.0.
//  (see at http://www.boost.org/LICENSE_1_0.txt)


#include <iostream>
#include <fstream>
#include <unistd.h>

#include <string>
#include <map>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/smart_ptr/scoped_ptr.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/format.hpp>

#ifndef LOGXX_DEFAULT_LOGGER
#define LOGXX_DEFAULT_LOGGER logxx::logger
#endif

#ifndef LOGXX_DEFAULT_LEVEL
#define LOGXX_DEFAULT_LEVEL logxx::debug
#endif

// IDEA: load dynamicaly LEVEL from ENVIRONMENT


namespace logxx {

class basic_channel {
public:
  virtual std::ostream& stream() = 0;
  virtual std::ostream& flush() = 0;
};

class console_channel : public basic_channel {
public:
  std::ostream& stream() { return std::cerr; }
  std::ostream& flush() { return std::cerr.flush(); }
};

class file_channel : public basic_channel {
public:
  file_channel(const char* filename) : file_(filename, std::ios_base::app | std::ios_base::out ) {}
  std::ostream& stream() { return file_; }
  std::ostream& flush() { return file_.flush(); }

private:
  std::ofstream file_;
};

struct nullstream : std::ostream { nullstream():std::ostream(0){} };

class null_channel : public basic_channel {
public:
  std::ostream& stream() { return null_; }
  std::ostream& flush() { return null_.flush(); }
private:
  static nullstream null_;
};

//
// TODO: 
// - tee_channel, 
// - syslog channel, 
// - anything other??
//

typedef enum {
  fatal,
  error,
  warning,
  info,
  debug,
  trace
} severity;


/**
 * basic logger
 *
 * policy based logger - idea:
 *
 * logger - there is root logger (without name), all messages are sent there
 * you can create new one "on fly" just by send name into logger::log() static method
 * if logger does not exists, new one is created
 *
 * all loggers inherit basic settings from "root logger" when created
 *
 *
 * filter_policy - decide what will happens with message
 * format_policy - add some envelope to message (e.g. time, severity, etc)
 * channel_policy - messages which goes through filter are sent into
 *
 * basic flow: 
 * message is sent to logger ->
 * logger will sent message to filter ->
 * filter will use formater to add message envelope 
 * and in next step will decide what to do with message (usualy send into channel)
 *
 */

template <int default_level, class format_policy, class filter_policy, class no_channel_policy = console_channel>
class basic_logger {
public:
  typedef basic_logger<default_level, format_policy, filter_policy, no_channel_policy> self_t;
public:


  /**
   * Heart of logger. 
   * return instance of logger from logger repository
   * if logger doesn't exists create new one. 
   *
   * logger instances are holds by boost::shared_ptr so you need no care about free instance
   */
  static self_t& log(const std::string& name = "") {
    // IDEA: move creating strategy into another policy
    boost::shared_ptr<self_t> r = loggers_[name];
    if(!r) {
      r.reset(new self_t);
      if(!name.empty()) { // as default - copy channel properties root logger
        boost::shared_ptr<self_t> root = loggers_[""];
        if(!root) root.reset(new self_t); // if not root loger in repository create new one with default settings
        r->channel_ = root->channel_;
        r->filter_.severity_ = root->filter_.severity_;
      }
      loggers_[name] = r;
    }
    return *r;
  }

  void channel(basic_channel* channel) { if(channel_) channel_->flush() ; channel_.reset(channel); }
  basic_channel& channel() { if(!channel_) channel_.reset(new no_channel_policy); return *channel_; }

  /**
   * return instance of filter to send log on
   */
  std::ostream& get(int severity) {
    return filter_.filter(severity,channel(),format_);
  }

  /**
   * shortcut of ::get() method
   */
  std::ostream& operator()(int severity) {
    return filter_.filter(severity,channel(),format_);
  }

  virtual ~basic_logger() {
    if (channel_) channel_->flush();
  }

  /**
   * setup filter level for given channel
   *
   * usage:
   * logxx::logger::log().filter(logxx::debug)
   */
  inline void filter(int severity) { filter_.severity_ = severity; }

  /**
   * return current severity of filter
   */  
  inline int severity() const { return filter_.severity_; }

  /**
   * hex memory dump from @param addr and length @param len
   *
   * dump() is by default sent to debug level
   */
  void dump(void* addr, int len) {
    if(filter_.severity_ < logxx::debug) return;
    unsigned char* a = reinterpret_cast<unsigned char*>(addr);
    get(logxx::debug) << boost::format("DUMP [%p(%d)]:") % addr % len << std::endl;
    for(int i = 0 ; i < len ; ++i ) {
      channel_->stream() << boost::format("%02x%c")
        % (unsigned int)(a[i])
        % ((i+1)%16?' ':'\n');
    }
    channel_->stream() << std::endl;
  }

protected:
  boost::shared_ptr<basic_channel> channel_;
  format_policy format_;
  filter_policy filter_;
  static std::map<std::string, boost::shared_ptr<self_t> > loggers_;

  basic_logger(int severity = default_level) {
    filter_.severity_ = severity; 
  }
};

// init static member :)
template <int default_level, class format_policy, class filter_policy, class no_channel_policy>
std::map<std::string, boost::shared_ptr<basic_logger<default_level, format_policy,filter_policy,no_channel_policy> >  > 
  basic_logger<default_level, format_policy,filter_policy,no_channel_policy>::loggers_;


/**
 * message formater
 */
struct basic_format {
  virtual std::ostream& format(int severity, basic_channel& out) = 0;
};

// IDEA: filter should receive ref to logger to allow identify name logger, 
// instead of directly writing to channel just only receive message, create envelope and return it??
struct std_format : basic_format {
  std::ostream& format(int severity, basic_channel& out) {
    return (out.stream()
        << boost::posix_time::second_clock::universal_time()
        << " [" << getpid() << "]"
        << " <" << severity << ">"
        << ": "
  );
}

};

/**
 * log filter - 
 *   it takes care on filtering sent messages
 *   std_filter will send everything with higher level than severity into null channel
 *
 *   it is meant for extending eg. 
 *   - abort program when severity is lower than "critical"
 *   - sent critical events to different channel eg. syslog
 *   - assertion for debug version
 *
 * some of ideas will be added into logxx lib in future
 */
struct std_filter {
  std::ostream& filter(int severity, basic_channel& s, basic_format& f) {
    static nullstream null;
    return severity <= severity_ 
      ? f.format(severity,s)
      : null;
  }

  int severity_;
};

/**
 * predefined logger with most basic policy to allow trivial log
 */
typedef class basic_logger<LOGXX_DEFAULT_LEVEL , std_format, std_filter> logger;

}

#define LOG_CHANNEL(ch) LOGXX_DEFAULT_LOGGER::log().channel(ch)
#define LOG_CHANNEL_(sink, ch) LOGXX_DEFAULT_LOGGER::log(sink).channel(ch)

#define LOG(level) if (LOGXX_DEFAULT_LOGGER::log().severity() >= logxx::level) LOGXX_DEFAULT_LOGGER::log().get(logxx::level)
#define LOG_(sink, level) if (LOGXX_DEFAULT_LOGGER::log(sink).severity() >= logxx::level) LOGXX_DEFAULT_LOGGER::log(sink).get(logxx::level)

#define DUMP(addr, len) if (LOGXX_DEFAULT_LOGGER::log().severity() >= logxx::debug) LOGXX_DEFAULT_LOGGER::log().dump((void*)addr,len)


#define LOG_IF(condition, level) if ((condition) && LOGXX_DEFAULT_LOGGER::log().severity() >= logxx::level) LOGXX_DEFAULT_LOGGER::log().get(logxx::level)
#define LOG_IF_(condition, sink, level) if ((condition) && LOGXX_DEFAULT_LOGGER::log(sink).severity() >= logxx::level) LOGXX_DEFAULT_LOGGER::log(sink).get(logxx::level)

#endif /* end of include guard: LOGXX_5JKQSPQI */
