// project
#include "logger.h"

// boost
#include <boost/log/trivial.hpp>

Logger::Logger() : _logger() {
}

Logger &Logger::warning(const std::string &message) {
    BOOST_LOG_SEV(Logger::get_mutable_instance()._logger, SeverityLevel::WARNING) << message;
    return Logger::get_mutable_instance();
}

Logger &Logger::info(const std::string &message) {
    BOOST_LOG_SEV(Logger::get_mutable_instance()._logger, SeverityLevel::NORMAL) << message;
    return Logger::get_mutable_instance();
}

Logger &Logger::critical(const std::string &message) {
    BOOST_LOG_SEV(Logger::get_mutable_instance()._logger, SeverityLevel::CRITICAL) << message;
    return Logger::get_mutable_instance();
}

void Logger::ExceptionHandler::operator()(const std::runtime_error &err) {
    Logger::warning(err.what());
}
