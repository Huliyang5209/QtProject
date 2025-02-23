#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <cassert>
#include <sys/epoll.h>

#include"./http_conn/http_conn1.h"
#include"./thread/threadpool.h"

const int MAX_FD = 65536;     
const int MAX_EVENT_NUMBER = 10000;
const int TIMESLOT = 5; 
class webserver
{
    public:
    webserver();
    ~webserver();
    void trig_mode();

    void init(int port , string user, string passWord, string databaseName,
              int log_write , int opt_linger, int trigmode, int sql_num,
              int thread_num, int close_log, int actor_model);
    void sql_pool();
    void thread_pool();
    void log_write();

    void timer(int connfd, struct sockaddr_in client_address);
    void adjust_timer(util_timer* timer);
    void deal_timer(util_timer *timer, int sockfd);

    void eventListen();
    void eventLoop();

    bool dealclinetdata();
    void dealwithread(int sockfd);
    void dealwithwrite(int sockfd);
    bool dealwithsignal(bool &timeout, bool &stop_server);

    private:
    //基础
    int m_close_log;
    int m_actor_model;
    char *m_root;
    int m_port;
    int m_log_write;

    int m_pipefd[2];
    int m_epollfd;
    http_conn1 *users;

    //线程池
    int m_thread_nums;
    threadpool<http_conn1>* m_thread_pool;
    int max_request=100;
    //数据库
    sql_connection_pool *m_sql_pool;
    string m_dbname;
    string m_user;
    string m_passWord;
    int m_sql_num;
    //定时器
    client_data* users_timer;
    Utils utils;
    //epoll—event事件
    epoll_event events[MAX_EVENT_NUMBER];

    int m_listenfd;
    int m_OPT_LINGER;
    int m_TRIGMode;
    int m_LISTENTrigmode;
    int m_CONNTrigmode;
};

#endif