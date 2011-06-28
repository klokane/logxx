# Trivial (policy based) C++ logger

I was looking for simple logger to my C++ projects. In last I write my own

fast start with zeroconfig ability
----------------------------------

    #include "logxx.hpp"

    int main() {
      // registrate new logger "channel" with sink file_channel
      logxx::logger::log("file").channel(new logxx::file_channel("app.log"));

      LOG(debug) << "This message goes to std::cerr" << std::endl;

      LOG_("file",debug) << "And this to file app.log" << std::endl;

      LOG_("on-fly",debug) <<  "You can create channels on fly" << std::endl;

      logxx::logger::log().filter(logxx::info); // setup minimal logging level
      LOG(debug) << "This message is now dropped because filter" << std::endl;

      // mute root logger
      logxx::logger::log().channel(new logxx::null_channel);
      LOG(error) << "Now is everything dropped" << std::endl;

      LOG_("on-fly",info) << "\"on-fly\" is still send into cerr" << std::endl;

      // unmute root logger
      logxx::logger::log().channel(new logxx::console_channel);
      LOG(info) << "Now we are in back in console" << std::endl;


    }

help macros to allow disable logging evaluation
------------------------------------------------

    #define LOG(level) if (logxx::logger::log().severity() >= logxx::level) logxx::logger::log().get(logxx::level)

original code:

    LOG(debug) << someReallyLongTimeConsumingDump() << std::endl;

expanded code:

    if (logxx::logger::log().severity() >= logxx::debug) 
        logxx::logger::log().get(logxx::debug) << someReallyLongTimeConsumingDump() << std::endl;

so evaluation of someReallyLongTimeConsumingDump() is just unly when debug severity is enabled



TODO:
----
  - syslog channel
  - filter policy with throw exception on fatal
  - more flexible formating policy (especialy channel name)
  - write doc around policies

