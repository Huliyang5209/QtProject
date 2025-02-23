#ifndef THREADPOOL_H
#define THREADPOOL_H
#include<list>
#include<pthread.h>
#include <cstdio>
#include <exception>
#include"../lock/lock.h"
#include"../CGImysql/sql_connection_pool.h"
#define MAX_TASK_NUMES  10

template <typename T>
class threadpool
{
    public:
    
    threadpool(int actor_model,int max_request,sql_connection_pool *sql_pool,int m_thread);
    ~threadpool();

    //将任务添加到任务队列中
    bool add_task_to_queue(T* request);
    bool add_task_to_queue(T* request,int state);

    //单例模式，实例化取出任务运行
    static void *worker_run_task(void* arg);

    //运行任务
    void run();

    private:
    std::list<T*> task_queue;
    locker m_queuelocker;
    sem m_queuestat;
    cond m_cond;
    pthread_t *m_threads;
    int m_thread_number;
    int m_actor_model;          //模型切换
    sql_connection_pool *m_connPool;  //数据库
    int m_max_requests;         //请求队列中允许的最大请求数
};
template<typename T>
threadpool<T>::~threadpool()
{
    delete[] m_threads;
}
template <typename T>
threadpool<T>::threadpool(int actor_model,int max_request,sql_connection_pool *sql_pool,int thread_number): m_actor_model(actor_model),m_thread_number(thread_number), m_max_requests(max_request), m_threads(NULL),m_connPool(sql_pool)
{
    m_actor_model=actor_model;
    m_max_requests=max_request;
    m_connPool=sql_pool;
    m_thread_number=thread_number;
    if(max_request<=0||thread_number<=0)
    {
         throw std::exception();
    }
    m_threads=new pthread_t[m_thread_number];
    if(!m_threads)
    {
        throw std::exception();
    }
    for(int i=0;i<MAX_TASK_NUMES;i++)
    {
        if(pthread_create(m_threads+i,NULL,worker_run_task,this)!=0)
        {
            delete[] m_threads;
            throw std::exception();
        }
        if(pthread_detach(m_threads[i]))
        {
            delete[] m_threads;
            throw std::exception();
        }
    }
}
template <typename T>
bool threadpool<T>::add_task_to_queue(T* request)
{
    if(request==NULL)
    {
        return false;
    }
    m_queuelocker.lock();
    if(task_queue.size()>=m_max_requests)
    {
        m_queuelocker.unlock();
        return false;
    }
    task_queue.push_back(request);
    m_queuelocker.unlock();
    m_queuestat.post();
    return true;
}
template <typename T>
bool threadpool<T>::add_task_to_queue(T* request,int state)
{
     if(request==NULL)
    {
        return false;
    }
    m_queuelocker.lock();
    if(task_queue.size()>=m_max_requests)
    {
        m_queuelocker.unlock();
        return false;
    }
    request->m_state=state;
    task_queue.push_back(request);
    m_queuelocker.unlock();
    m_queuestat.post();
    return true;
}
template <typename T>
void *threadpool<T>::worker_run_task(void* arg)
{
     threadpool* m_thread_pool=(threadpool*)arg;
     m_thread_pool->run();
     return m_thread_pool;
}
template <typename T>
void threadpool<T>::run()
{
    while(true)
    {
        m_queuestat.wait();
        m_queuelocker.lock();
        if(task_queue.size()<=0)
        {
            m_queuelocker.unlock();
            continue;
        }
        T* m_request=task_queue.front();
        task_queue.pop_front();
        m_queuelocker.unlock();
        if(m_request==NULL)
        {
            continue;
        }
        if(m_actor_model==1)
        {
             if (0 == m_request->m_state)
            {
                if (m_request->read_one())
                {
                    m_request->improv = 1;
                    sql_connection_pool_RAII mysqlcon(&m_request->mysql, m_connPool);
                    m_request->process();
                }
                else
                {
                    m_request->improv = 1;
                    m_request->timer_flag = 1;
                }
            }
            else
            {
                if (m_request->write())
                {
                    m_request->improv = 1;
                }
                else
                {
                    m_request->improv = 1;
                    m_request->timer_flag = 1;
                }
            }
        }
        else
        {
            sql_connection_pool_RAII mysqlcon(&m_request->mysql, m_connPool);
            m_request->process();
        }
    }

};
#endif