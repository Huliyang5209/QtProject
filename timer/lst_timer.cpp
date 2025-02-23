#include "lst_timer.h"
#include "../http_conn/http_conn1.h"

sort_timer_lst::sort_timer_lst()
{
     head=NULL;
     tail=NULL;
}

sort_timer_lst::~sort_timer_lst()
{
    util_timer *point=head;
    for(;point!=tail;point=point->next)
    {
        delete point;
    }
}

void sort_timer_lst::add_timer(util_timer* timer)
{
     if(!timer)
     {
        return ;
     }
    if(head==NULL)
    {
        head=timer=tail;
        return ;
    }
    if(timer->expire<=head->expire)
    {
        timer->next=head;
        head->prev=timer;
        head=timer;
        return ;
    }
    add_timer(timer,head);
}

void sort_timer_lst::delete_timer(util_timer* timer)
{
    if (!timer)
    {
        return;
    }
    if ((timer == head) && (timer == tail))
    {
        delete timer;
        head = NULL;
        tail = NULL;
        return;
    }
    if (timer == head)
    {
        head = head->next;
        head->prev = NULL;
        delete timer;
        return;
    }
    if (timer == tail)
    {
        tail = tail->prev;
        tail->next = NULL;
        delete timer;
        return;
    }
    timer->prev->next = timer->next;
    timer->next->prev = timer->prev;
    delete timer;
}

void sort_timer_lst::tick()
{
    if (!head)
    {
        return;
    }
    
    time_t cur = time(NULL);
    util_timer *tmp = head;
    while (tmp)
    {
        if (cur < tmp->expire)
        {
            break;
        }
        tmp->func(tmp->user_data);
        head = tmp->next;
        if (head)
        {
            head->prev = NULL;
        }
        delete tmp;
        tmp = head;
    }
}

void sort_timer_lst::add_timer(util_timer* timer,util_timer* lst_head)
{
    util_timer *point=head;
    while(point)
    {
        if (point->next == NULL)
        {
            timer->prev = point;
            point->next = timer;
            tail = timer;
            return;
        }
        if(point->expire>timer->expire)
        {
            timer->next=point;
            timer->prev=point->prev;
            point->prev=timer;
            point->prev->next=timer;
            return ;
        }
        point=point->next;
    }
}

void sort_timer_lst::adjust_timer(util_timer* timer)
{
    if (!timer)
    {
        return;
    }
    util_timer *tmp = timer->next;
    if (!tmp || (timer->expire < tmp->expire))
    {
        return;
    }
    if (timer == head)
    {
        head = head->next;
        head->prev = NULL;
        timer->next = NULL;
        add_timer(timer, head);
    }
    else
    {
        timer->prev->next = timer->next;
        timer->next->prev = timer->prev;
        add_timer(timer, timer->next);
    }
}

void Utils::init(int timeslot)
{
    m_TIMESLOT = timeslot;
}

int Utils::setnonblocking(int timefd)
{
    int old_option=fcntl(timefd,F_GETFL);
    int new_option=old_option|O_NONBLOCK;
    fcntl(timefd,F_SETFL,new_option);
    return old_option;
}

bool Utils::addfd(int epollfd,int timefd,bool one_shot,int TRIGMode)
{
     struct epoll_event event;
     event.data.fd=timefd;

     if (1 == TRIGMode)
        event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    else
        event.events = EPOLLIN | EPOLLRDHUP;

    if (one_shot)
        event.events |= EPOLLONESHOT;
     
     epoll_ctl(epollfd,EPOLL_CTL_ADD,timefd,&event);
     setnonblocking(timefd);
}

void Utils::set_sig(int sig, void(handler)(int), bool restart)
{
      struct sigaction sa;
      memset(&sa,'\0',sizeof(sa));

      sa.sa_handler=handler;
      if(restart)
      {
        sa.sa_flags|=SA_RESTART;
      }

      sigfillset(&sa.sa_mask);

      assert(sigaction(sig,&sa,NULL)!=-1);
}
void Utils::sig_handler(int sig)
{
     int save_errno=errno;
     int msg=sig;
     send(u_pipefd[1],(char*)msg,1,0);
     errno=save_errno;
}
void Utils::timer_handler()
{
    timer_list.tick();
    alarm(m_TIMESLOT);
}

void Utils::show_error(int connfd, const char *info)
{
     send(connfd,info,strlen(info),0);
     close(connfd);
}

int *Utils::u_pipefd = 0;
int Utils::u_epollfd = 0;

class Utils;
void func(client_data* user_data)
{
    epoll_ctl(Utils::u_epollfd,EPOLL_CTL_DEL,user_data->sockfd,0);
    assert(user_data);
    close(user_data->sockfd);
    http_conn1::m_user_count--;
}