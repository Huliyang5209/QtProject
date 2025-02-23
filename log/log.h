#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include <iostream>
#include <string>
#include <stdarg.h>
#include <pthread.h>
#include "block_quque.h"
using namespace std;
class log
{   
    public:
    static log *getInstance()//static关键字
    {
        static log instance;
        return &instance;
    }
    static void *flush_log_thread(void *args)
    {
        log::getInstance()->async_wirte_log();
    }
    void flush()
    {
        m_mutex.lock();
    //强制刷新写入流缓冲区
        fflush(m_fp);
        m_mutex.unlock();
    };
    
    void write_log(int level, const char *format, ...);
    
    bool init(const char *file_name, int close_log,int log_buf_size = 8192, int split_lines = 5000000, int max_queue_size = 0);


    private:
    log();
    virtual ~log();
    void *async_wirte_log()
    {
        string single_log;
        while(m_log_queue->consumer(single_log))
        {
            m_mutex.lock();
            fputs(single_log.c_str(), m_fp);
            m_mutex.unlock();
        }
    }
    int m_is_async;
    char dir_name[224];
    char log_name[224];
    int m_log_buf_size;
    int m_split_lines;
    int m_counts;
    FILE *m_fp;
    int m_today;
    block_queue<string>*m_log_queue;
    locker m_mutex;
    int m_close_log;
    char *m_buf;
};

#define LOG_DEBUG(format,...) if(0==m_close_log){log::getInstance()->write_log(0,format,##__VA_ARGS__);log::getInstance()->flush();}
#define LOG_INFO(format,...) if(0==m_close_log){log::getInstance()->write_log(0,format,##__VA_ARGS__);log::getInstance()->flush();}
#define LOG_WARN(format,...) if(0==m_close_log){log::getInstance()->write_log(0,format,##__VA_ARGS__);log::getInstance()->flush();}
#define LOG_ERROR(format,...) if(0==m_close_log){log::getInstance()->write_log(0,format,##__VA_ARGS__);log::getInstance()->flush();}

#endif