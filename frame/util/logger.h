#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>
#include <fstream>
#include <vector>
#include <QString>
#include <mutex>

class Logger
{
public:
    enum LogLevel
    {
        LogLevelDebug,
        LogLevelInfo,
        LogLevelError
    };

    static Logger& instance();

    void log(const QString inMessage);
    void log(const QString inMessage, const LogLevel inLogLevel);

    void log(const QStringList inMessages, const LogLevel inLogLevel);

protected:
    static Logger* pInstance;
    static const char* kLogFileName;

    std::ofstream mOutputStream;

    friend class Cleanup;
    class Cleanup
    {
    public:
        Cleanup() {}
        ~Cleanup();
    };

    void logHelper(const QString inMessage, const LogLevel inLogLevel);

private:
    Logger();
    virtual ~Logger();
    Logger(const Logger&);
    Logger& operator =(const Logger&);
    static std::mutex sMutex;
};

#endif // LOGGER_H
