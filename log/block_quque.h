#include <pthread.h>
#include <iostream>
#include<sys/time.h>


#include "../lock/lock.h"
using namespace std;

template<class T>
class block_queue
{
    private:
    int m_size;
    int m_max_size;
    locker mutex;
    cond m_cond;
    int m_front;
    int m_back;
    T* arry;

    public:
    block_queue(int max_queue_size)
    {
        if(max_queue_size<0)
        {
            exit(-1);
        }

        m_max_size=max_queue_size;
        arry=new T[m_max_size];
        m_size=0;
        m_front=-1;
        m_back=-1;

    }

    ~block_queue()
    {
        mutex.lock();
        if(arry!=NULL)
            delete[] arry;
        mutex.unlock();
    }

    bool empty()
    {
        mutex.lock();
        if(cur_size>0)
        {
            mutex.unlock();
            return false;
            
        }
        return true;
        mutex.unlock();
    }

    void  clear()
    {
        mutex.lock();
        m_size=0;
        m_front=-1;
        m_back=-1;
        mutex.unlock();
    }

    bool full()
    {
        mutex.lock();
        if(m_size<=m_max_size)
        { 
            mutex.unlock();
            return false;
        }
        mutex.unlock();
        return true;
    }

    int cur_size()
    {
        int tmp=0;
        mutex.lock();

        tmp=m_size;

        mutex.unlock();
        return tmp;
    }

    int max_size()
    {
        int tmp=0;
        mutex.lock();

        tmp=m_max_size;

        mutex.unlock();
        return tmp;
    }

    bool front(T &value)
    {
        mutex.lock();
        if(cur_size<=0)
        {
            mutex.unlock();
            return false;
        }
        value=arry[m_front];
        return true;
        mutex.unlock();
    }
    
    bool back(T &value)
    {
        mutex.lock();
        if(cur_size>m_max_size)
        {
            mutex.unlock();
            return false;
        }
        value=arry[cur_size];
        return true;
        mutex.unlock();
    }
    
    //�������������޸�,���⿽������,֧�ִ��ݳ�������
    bool producter(const T &item)
    {
        mutex.lock();
        if(cur_size>=m_max_size)
        {
            m_cond.broadcast();
            mutex.unlock();
            return false;
        }
        m_back=(m_back+1)%m_max_size;
        arry[m_back]=item;
        m_size++;
        m_cond.broadcast();
        mutex.unlock();
        return true;
    }

    bool consumer(T &item)
    {
        mutex.lock();
        while(m_size<=0)
        {
            if(!m_cond.wait(mutex.get()))
            {
                mutex.unlock();
                return false;
            }
        }
        m_front=(m_front+1)%m_max_size;
        item=arry[m_front];
        m_size--;
        mutex.unlock();
        return true;
    }

    bool consumer(T &item, int ms_timeout)//��ʱ����
    {
        //��ȡ��ʱ������ԽϾ�ȷ��ϵͳʱ��
        struct timespec t = {0, 0};
        struct timeval now = {0, 0};
        gettimeofday(&now,NULL);

        mutex.lock();
        //��ʱ
        if(m_size<=0)
        {
            //��t�ṹ����ֶθ�ֵ��������һ����ʱʱ��ms_timeout֮��ľ���ʱ��
            t.tv_sec = now.tv_sec + ms_timeout / 1000;
            t.tv_nsec = (ms_timeout % 1000) * 1000;
            //ʵ���̵߳Ķ�ʱ�ȴ�����
            if (!m_cond.timewait(mutex.get(), t))
            {
                mutex.unlock();
                return false;
            }
        }
        //����
        if (m_size <= 0)
        {
            mutex.unlock();
            return false;
        }
        m_front = (m_front + 1) % m_max_size;
        item = arry[m_front];
        m_size--;
        mutex.unlock();
        return true;
    }
    
};