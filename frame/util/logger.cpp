#include "logger.h"

#include <stdexcept>
#include <ctime>
#include <QDebug>

using namespace std;

const string kLogLevelDebug = "DEBUG";
const string kLogLevelInfo = "INFO";
const string kLogLevelError = "ERROR";

const char* Logger::kLogFileName = "/tmp/dde-top-panel.log";

Logger* Logger::pInstance = nullptr;

mutex Logger::sMutex;

Logger& Logger::instance()
{
    static Cleanup cleanup;

    lock_guard<mutex> guard(sMutex);
    if(pInstance == nullptr)
    {
        pInstance = new Logger();
    }

    return *pInstance;
}

Logger::Cleanup::~Cleanup()
{
    lock_guard<mutex> guard(Logger::sMutex);
    delete Logger::pInstance;
    Logger::pInstance = nullptr;
}

Logger::~Logger()
{
    mOutputStream.close();
}

Logger::Logger()
{
    mOutputStream.open(kLogFileName, ios_base::app);
    if(!mOutputStream.good())
    {
        throw runtime_error("Unable to initialize the Logger!");
    }
}

void Logger::log(const QString inMessage)
{
    log(inMessage, LogLevelInfo);
}

void Logger::log(const QString inMessage, const LogLevel inLogLevel)
{
    lock_guard<mutex> guard(sMutex);
    logHelper(inMessage, inLogLevel);
}

void Logger::log(const QStringList inMessages, const LogLevel inLogLevel)
{
    lock_guard<mutex> guard(sMutex);
    for(size_t i=0; i<inMessages.size(); i++)
    {
        logHelper(inMessages.at(i), inLogLevel);
    }
}

void Logger::logHelper(const QString inMessage, const LogLevel inLogLevel)
{
    time_t rawtime;
    struct tm * timeinfo;
    char buffer[80];

    time (&rawtime);
    timeinfo = localtime(&rawtime);

    strftime(buffer,sizeof(buffer),"%Y-%m-%d %H:%M:%S",timeinfo);
    QString str(buffer);
    str.append(" [").append(inLogLevel).append("] ").append(inMessage);
#ifdef QT_DEBUG
    qDebug()<<str<<endl;
#endif
    mOutputStream<<str.toStdString()<<endl;
}
