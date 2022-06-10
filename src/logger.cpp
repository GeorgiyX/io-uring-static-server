// project
#include "logger.h"

// boost
#include <boost/log/trivial.hpp>
#include <boost/log/core.hpp>
#include <boost/log/utility/exception_handler.hpp>

Logger::Logger() : _logger() {
    boost::log::core::get()->set_exception_handler(boost::log::make_exception_handler<
            std::runtime_error,
            boost::bad_lexical_cast
            >(
            Logger::ExceptionHandler())
            );
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

void Logger::ExceptionHandler::operator()(const std::runtime_error &err) const {
    Logger::critical(err.what());
}

void Logger::ExceptionHandler::operator()(const boost::bad_lexical_cast &err) const {
    Logger::critical(err.what());
}
