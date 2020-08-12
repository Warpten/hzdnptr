#include "Logger.hpp"

/* static */ Logger& Logger::Instance()
{
    static Logger instance;
    return instance;
}


Logger::Logger() { }