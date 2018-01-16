#pragma once

#include<time.h>
#include<string>
#include<locale.h>
#include<sstream>
#include<iostream>
#include"common/settings.hpp"

namespace dan
{
    //logger的日志等级,小于全局设定的日志等级就会失效
    enum class LogLevel
    {
        DEBUG,
        INFO,
        WARNING,
        ERROR,
        CRITICAL,
    };

    //日志处函数接口
    class ILogHandler
    {
        public:
            virtual void log(std::string message,LogLevel level)=0;
    };

    //默认的日志处理函数，只是打印消息
    class CerrLogHandler:public ILogHandler
    {
        public:
            void log(std::string message,LogLevel level)override
            {
                std::cerr<<message;
            }
    };

    //
    class logger
    {
        public:
            logger(std::string prefix,LogLevel level):level_(level)
            {

                stringstream_<<"("<<timestamp()<<")";
                switch(level)
                {
                    case LogLevel::INFO:
                        stringstream_<<"[\033[01m"<<prefix<<"\033[0m]";
                        break;
                    case LogLevel::WARNING:
                        stringstream_<<"[\033[32m\033[01m\033[40m"<<prefix<<"\033[0m]";
                        break;
                    case LogLevel::ERROR:
                        stringstream_<<"[\033[31m\033[01m\033[44m"<<prefix<<"\033[0m]";
                        break;
                    case LogLevel::CRITICAL:
                        stringstream_<<"[\033[33m\033[01m\033[41m"<<prefix<<"\033[0m]";
                        break;
                    case LogLevel::DEBUG:
                        stringstream_<<"[\033[37m\033[01m"<<prefix<<"\033[0m]";
                        break;
                }
            }

            ~logger()
            {
                //当logger自身的日志等级高于设定的全局日志等级时，正常运行
                if(level_>=get_current_log_level())
                {
                    stringstream_<<std::endl;//刷新缓冲区
                    get_handler_ref()->log(stringstream_.str(),level_);//析构时执行
                }
            }

            //当logger自身的日志等级低于全局日志等级时，logger失效
            template<typename T>
            logger& operator<<(T const&value)
            {
                if(level_>=get_current_log_level())
                {
                    stringstream_<<value;
                }
                return *this;
            }

            static void setLogLevel(LogLevel level)
            {
                get_log_level_ref()=level;
            }

            static void setHandler(ILogHandler*handler)
            {
                get_handler_ref()=handler;
            }

            static LogLevel get_current_log_level()
            {
                return get_log_level_ref();
            }
        private:
            static std::string timestamp()
            {
                char outstr[37];
                time_t now=time(NULL);
                tm t;
                localtime_r(&now,&t);//可重入本地时间
                //gmtime_r(&now,&t);//可重入版本
                //t.tm_hour+=8;
                size_t num=strftime(outstr,sizeof(outstr),"%Y-%m-%d %H:%M:%S",&t);
                return std::string(outstr,num);
            }

            //获得settings中当前日志等级
            static LogLevel&get_log_level_ref()
            {
                static LogLevel current_level=(LogLevel)DAN_LOG_LEVEL;
                return current_level;
            }
            //
            static ILogHandler*& get_handler_ref()
            {
                static CerrLogHandler default_handler;
                static ILogHandler* current_handler=&default_handler;
                return current_handler;
            }
        private:
            std::ostringstream stringstream_;
            LogLevel level_;

    };
}

// 日志宏:局部变量,离开作用域就析构,处理消息
#define DAN_LOG_CRITICAL \
    if(dan::logger::get_current_log_level()<= dan::LogLevel::CRITICAL) \
        dan::logger("critical",dan::LogLevel::CRITICAL)

#define DAN_LOG_ERROR \
    if(dan::logger::get_current_log_level()<= dan::LogLevel::ERROR) \
        dan::logger("error   ",dan::LogLevel::ERROR)

#define DAN_LOG_WARNING \
    if(dan::logger::get_current_log_level()<= dan::LogLevel::WARNING) \
        dan::logger("warning ",dan::LogLevel::WARNING)

#define DAN_LOG_INFO \
    if(dan::logger::get_current_log_level()<= dan::LogLevel::INFO) \
        dan::logger("info    ",dan::LogLevel::INFO)

#define DAN_LOG_DEBUG \
    if(dan::logger::get_current_log_level()<= dan::LogLevel::DEBUG) \
        dan::logger("debug   ",dan::LogLevel::DEBUG)

