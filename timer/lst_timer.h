#ifndef LST_TIMER
#define LST_TIMER

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/uio.h>



#include <time.h>
#include "../log/log.h"

class util_timer;
struct client_data
{
    sockaddr_in address;
    int sockfd;
    util_timer* timer;
};

class util_timer
{
    public:
    util_timer* prev;
    util_timer* next;
    void (*func)(client_data*);
    client_data* user_data;
    time_t expire;

    public:
     util_timer():prev(NULL),next(NULL){};
};

class sort_timer_lst
{
    public:
    sort_timer_lst();
    ~sort_timer_lst();

    void add_timer(util_timer* m_timer);
    void delete_timer(util_timer* m_timer);
    void tick();
    void adjust_timer(util_timer* m_timer);
    void adjust_timer(util_timer* m_timer,int expire);

    private:
    void add_timer(util_timer* m_timer,util_timer* lst_head);
    util_timer* head;
    util_timer* tail;
};

class Utils
{
    public:
    static int *u_pipefd;
    static int u_epollfd;
    sort_timer_lst timer_list;
    int m_TIMESLOT;

    public:
    Utils(){};
    ~Utils(){};
    void init(int timeslot);
    int setnonblocking(int timefd);
    bool addfd(int epollfd,int timefd,bool one_shot,int TRIGMode);
    void set_sig(int sig, void(handler)(int), bool restart = true);
    static void sig_handler(int sig);
    void timer_handler();

    void show_error(int connfd, const char *info);
};

void func(client_data* user_data);

#endif