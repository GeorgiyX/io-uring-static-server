#ifndef IO_URING_STATIC_SERVER_LOGGER_H
#define IO_URING_STATIC_SERVER_LOGGER_H

// boost
#include <boost/serialization/singleton.hpp>
#include <boost/log/sources/basic_logger.hpp>
#include <boost/log/sources/logger.hpp>
#include <boost/log/sources/severity_feature.hpp>
#include <boost/log/sources/severity_logger.hpp>

// stl
#include <stdexcept>

class Logger : public  boost::serialization::singleton<Logger> {
public:
    enum SeverityLevel {
        NORMAL,
        WARNING,
        ERROR,
        CRITICAL
    };
    struct ExceptionHandler {
        void operator() (const std::runtime_error &err);
    };

    static Logger &critical(const std::string &message);
    static Logger &warning(const std::string &message);
    static Logger &info(const std::string &message);

private:
    friend boost::serialization::detail::singleton_wrapper<Logger>;
    Logger();

    boost::log::sources::severity_logger_mt<SeverityLevel> _logger;

};

#endif //IO_URING_STATIC_SERVER_LOGGER_H
