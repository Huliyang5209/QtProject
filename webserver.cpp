#include "webserver.h"
webserver::webserver()
{
    users=new http_conn1[MAX_FD];
    char server_path[200];
    getcwd(server_path, 200);
    char root[6] = "/root";
    m_root = (char *)malloc(strlen(server_path) + strlen(root) + 1);
    strcpy(m_root, server_path);
    strcat(m_root, root);
    users_timer=new client_data[MAX_FD];
}
webserver::~webserver()
{
    close(m_epollfd);
    close(m_listenfd);
    close(m_pipefd[1]);
    close(m_pipefd[0]);
    delete[] users;
    delete[] users_timer;
    delete m_thread_pool;
}

void webserver::trig_mode()
{
    //LT + LT
    if (0 == m_TRIGMode)
    {
        m_LISTENTrigmode = 0;
        m_CONNTrigmode = 0;
    }
    //LT + ET
    else if (1 == m_TRIGMode)
    {
        m_LISTENTrigmode = 0;
        m_CONNTrigmode = 1;
    }
    //ET + LT
    else if (2 == m_TRIGMode)
    {
        m_LISTENTrigmode = 1;
        m_CONNTrigmode = 0;
    }
    //ET + ET
    else if (3 == m_TRIGMode)
    {
        m_LISTENTrigmode = 1;
        m_CONNTrigmode = 1;
    }
}

void webserver::init(int port, string user, string passWord, string databaseName, int log_write, 
                     int opt_linger, int trigmode, int sql_num, int thread_num, int close_log, int actor_model)
{
   m_port=port;
   m_user=user;
   m_passWord=passWord;
   m_dbname=databaseName;
   m_log_write=log_write;
   m_thread_nums=thread_num;
   m_sql_num=sql_num;
   m_close_log=close_log;
   m_OPT_LINGER=opt_linger;
   m_TRIGMode=trigmode;
   m_actor_model = actor_model;
}
void webserver::thread_pool()
{
    //int actor_model,int max_request,sql_connection_pool *sql_pool,int thread_number
    m_thread_pool=new threadpool<http_conn1>(m_actor_model,max_request, m_sql_pool, m_thread_nums);
}
void webserver::sql_pool()
{
    m_sql_pool=sql_connection_pool::get_sql_connect_to_instance();
    m_sql_pool->init("localhost", m_user, m_passWord, m_dbname, 3306, m_sql_num, m_close_log);
    users->initmysql_result(m_sql_pool);
}
void webserver::log_write()
{
    if (0 == m_close_log)
    {
        //初始化日志
        if (1 == m_log_write)
            log::getInstance()->init("./ServerLog", m_close_log, 2000, 800000, 800);
        else
            log::getInstance()->init("./ServerLog", m_close_log, 2000, 800000, 0);
    }
}
void webserver::timer(int connfd, struct sockaddr_in client_address)
{
    users[connfd].init(connfd, client_address, m_root, m_CONNTrigmode, m_close_log, m_user, m_passWord, m_dbname);
    
    users_timer[connfd].address=client_address;
    users_timer[connfd].sockfd=connfd;
    util_timer *timer=new util_timer;

    time_t cur=time(NULL);
    timer->expire=cur+3*TIMESLOT;
    timer->user_data=&users_timer[connfd];
    timer->func=func;
    users_timer[connfd].timer = timer;

    utils.timer_list.add_timer(timer);
}

void webserver::adjust_timer(util_timer* timer)
{
     if(timer==NULL)
     {
        return ;
     }
     time_t cur=time(NULL);
     timer->expire=cur+3*TIMESLOT;
     utils.timer_list.add_timer(timer);
     LOG_INFO("%s", "adjust timer once");
}

void webserver::deal_timer(util_timer *timer, int sockfd)
{
    timer->func(&users_timer[sockfd]);
    if(timer)
    {
        utils.timer_list.delete_timer(timer);
    }
    LOG_INFO("close fd %d", users_timer[sockfd].sockfd);
}

void webserver::eventListen()
{
    m_listenfd=socket(PF_INET, SOCK_STREAM, 0);
    assert(m_listenfd >= 0);
    
    //关闭服务器-客户端连接
    if(0==m_OPT_LINGER)
    {
        struct linger tmp={0,1};
        setsockopt(m_listenfd,SOL_SOCKET,SO_LINGER,&tmp,sizeof(tmp));
    }else if(1==m_OPT_LINGER)
    {
        struct linger tmp={1,1};
        setsockopt(m_listenfd,SOL_SOCKET,SO_LINGER,&tmp,sizeof(tmp));
    }

    int ret=0;
    struct sockaddr_in address;
    bzero(&address,sizeof(address));
    address.sin_family=AF_INET;
    address.sin_addr.s_addr=htonl(INADDR_ANY);
    address.sin_port=htons(m_port);
    

    int flag=1;
    setsockopt(m_listenfd,SOL_SOCKET,SO_REUSEADDR,&flag,sizeof(flag));
    ret = bind(m_listenfd, (struct sockaddr *)&address, sizeof(address));
    assert(ret >= 0);
    ret=listen(m_listenfd,5);
    assert(ret >= 0);

    utils.init(TIMESLOT);

    epoll_event events[MAX_EVENT_NUMBER];
    m_epollfd=epoll_create(5);
    assert(m_epollfd!=-1);

    utils.addfd(m_epollfd,m_listenfd,false, m_LISTENTrigmode);
    http_conn1::m_epollfd = m_epollfd;

    ret=socketpair(PF_UNIX, SOCK_STREAM, 0, m_pipefd);
    assert(ret != -1);
    utils.setnonblocking(m_pipefd[1]);
    utils.addfd(m_epollfd,m_pipefd[0],false, m_LISTENTrigmode);
    utils.set_sig(SIGPIPE, SIG_IGN);

    utils.set_sig(SIGALRM,utils.sig_handler,false);
    utils.set_sig(SIGTERM,utils.sig_handler,false);

    alarm(TIMESLOT);
    Utils::u_pipefd = m_pipefd;
    Utils::u_epollfd = m_epollfd;
}

bool webserver::dealclinetdata()
{
    struct sockaddr_in client_address;
    socklen_t client_addrlength=sizeof(client_address);
    if(m_LISTENTrigmode==0)
    {
        int connfd=accept(m_listenfd,(struct sockaddr*)&client_address,&client_addrlength);
        if(connfd<0)
        {
             LOG_ERROR("%s:errno is:%d", "accept error", errno);
             return false;
        }
        if(http_conn1::m_user_count>=MAX_FD)
        {
            utils.show_error(connfd, "Internal server busy");
            LOG_ERROR("%s", "Internal server busy");
            return false;
        }
        timer(connfd,client_address);
    }
    else
    {
        while(1)
        {
        int connfd=accept(m_listenfd,(struct sockaddr*)&client_address,&client_addrlength);
        if(connfd<0)
        {
             LOG_ERROR("%s:errno is:%d", "accept error", errno);
             break;
        }
        if(http_conn1::m_user_count>=MAX_FD)
        {
            utils.show_error(connfd, "Internal server busy");
            LOG_ERROR("%s", "Internal server busy");
            break;
        }
        timer(connfd,client_address);
        }
         return false;
    }
    return true;
}

void webserver::dealwithread(int sockfd)
{
     util_timer* timer=users_timer[sockfd].timer;

     if(1==m_actor_model)
     {
        if(timer)
        {
           adjust_timer(timer);
        }

        m_thread_pool->add_task_to_queue(users+sockfd,0);

        while(true)
        {
           if(users[sockfd].improv==1)
           {
              if(users[sockfd].timer_flag==1)
              {
                deal_timer(timer,sockfd);
                users[sockfd].timer_flag=0;
              }
              users[sockfd].improv=0;
           }
           break;
        }
     }else
     {
        if(users[sockfd].read_one())
        {
            LOG_INFO("deal with the client(%s)", inet_ntoa(users[sockfd].get_address()->sin_addr));
            m_thread_pool->add_task_to_queue(users+sockfd);
            if(timer)
            {
                adjust_timer(timer);
            }
        }else
        {
            deal_timer(timer,sockfd);
        }
     }
}

void webserver::dealwithwrite(int sockfd)
{
    util_timer* timer=users_timer[sockfd].timer;
    if(1==m_actor_model)
    {
        if(timer)
        {
            adjust_timer(timer);
        }
        m_thread_pool->add_task_to_queue(users+sockfd,1);
        while(true)
        {
            if(users[sockfd].improv==1)
            {
                if(users[sockfd].timer_flag==1)
                {
                    deal_timer(timer,sockfd);
                    users[sockfd].timer_flag=0;
                }
                users[sockfd].improv=0;
                break;
            }
        }
    }else
    {
        if(users[sockfd].write())
        {
            LOG_INFO("send data to the client(%s)", inet_ntoa(users[sockfd].get_address()->sin_addr));
            if (timer)
            {
                adjust_timer(timer);
            }
        }
        else
        {
            deal_timer(timer,sockfd);
        }
    }
}

bool webserver::dealwithsignal(bool &timeout, bool &stop_server)
{
     int ret=0;
     char signals[1024];
     int sig;

     ret=recv(m_pipefd[0],signals,sizeof(signals),0);
     if(ret==0)
     {
        return false;
     }else if(ret==-1)
     {
        return false;
     }else
     {
        for(int i=0;i<ret;i++)
        {
           switch(signals[i])
           {
              case SIGALRM:
              {
                timeout=true;
                break;
              }
              case SIGTERM:
              {
                stop_server = true;
                break;
              }    
           }
        }
     }
     return true;
}

void webserver::eventLoop()
{
    bool timerout=false;
    bool stop_server=false;

    while(!stop_server)
    {
        int number=epoll_wait(m_epollfd,events,MAX_EVENT_NUMBER,-1);

        if(number<=0&&errno!=EINTR)
        {
            LOG_ERROR("%s", "epoll failure");
            break;
        }

        for(int i=0;i<number;i++)
        {
            int sockfd=events[i].data.fd;

            if(sockfd==m_listenfd)
            {
                bool flag = dealclinetdata();
                if (false == flag)
                    continue;
            }else if(events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
            {
                util_timer *timer = users_timer[sockfd].timer;
                deal_timer(timer, sockfd);
            }else if((sockfd == m_pipefd[0]) && (events[i].events & EPOLLIN))
            {
                bool flag = dealwithsignal(timerout, stop_server);
                if (false == flag)
                    LOG_ERROR("%s", "dealclientdata failure");
            }else if(events[i].events & EPOLLIN)
            {
                dealwithread(sockfd);
            }
            else if (events[i].events & EPOLLOUT)
            {
                dealwithwrite(sockfd);
            }
        }
        if(timerout)
        {
            utils.timer_handler();

            LOG_INFO("%s", "timer tick");

            timerout = false;
        }

    }
}