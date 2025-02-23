#include <mysql/mysql.h>
#include <stdio.h>
#include <string>
#include <string.h>
#include <stdlib.h>
#include <list>
#include <pthread.h>
#include <iostream>
#include "sql_connection_pool.h"

sql_connection_pool::sql_connection_pool()
{
    free_use_conn=0;
    had_used_conn=0;
}

void sql_connection_pool::init(string url, string User, string PassWord, string DBName, int Port, int MaxConn, int close_log)
{
    m_url=url;
    m_close_log=close_log;
    m_dbName=DBName;
    m_port=Port;
    m_user=User;
    m_passwd=PassWord;

    for(int i=0;i<MaxConn;i++)
    {
        MYSQL *con = NULL;

        con=mysql_init(con);

        if(con==NULL)
        {
            LOG_ERROR("MySQL Error");
			exit(1);
        }

        con=mysql_real_connect(con,url.c_str(),User.c_str(),PassWord.c_str(),DBName.c_str(),Port,NULL,0);

        if(con==NULL)
        {
            LOG_ERROR("MySQL Error");
			exit(1);
        }
        sql_connection_list.push_back(con);
        ++free_use_conn;
        
    }

    m_sem_reserve=sem(free_use_conn);
    m_MaxConn=free_use_conn;
}

MYSQL *sql_connection_pool::get_sql_connection()
{
    MYSQL *m_con=NULL;
   
    if(sql_connection_list.size()==0)
    {
        return NULL;
    }
    //消费者-生产者模型

    m_sem_reserve.wait();//信号通知

    m_lock.lock();
    
    m_con=sql_connection_list.front();
    sql_connection_list.pop_front();

    --free_use_conn;
    ++had_used_conn;
    m_lock.unlock();
    return m_con;
}

bool sql_connection_pool::return_sql_connection(MYSQL *con)
{
    if(con==NULL)
    {
        return false;
    }

    m_lock.lock();

    sql_connection_list.push_back(con);
    ++free_use_conn;
    --had_used_conn;

    m_lock.unlock();
    m_sem_reserve.post();
}

void sql_connection_pool::destory_sql_pool()
{
    m_lock.lock();
    //先判断大小
    if(sql_connection_list.size()>0)
    {
        list<MYSQL *>::iterator it;
        for(it=sql_connection_list.begin();it!=sql_connection_list.end();++it)
        {
             MYSQL *con=*it;
             mysql_close(con);
        } 
        free_use_conn=0;
        had_used_conn=0;
        sql_connection_list.clear();
    }
   
    m_lock.unlock();
}


int sql_connection_pool::get_free_use_conn_num()
{
    //在一个类中的this使用
    return this->free_use_conn;
}


sql_connection_pool::~sql_connection_pool()
{
     destory_sql_pool();
}

sql_connection_pool *sql_connection_pool::get_sql_connect_to_instance()
{
    static sql_connection_pool m_sql_pool;
    return &m_sql_pool;
}

sql_connection_pool_RAII::sql_connection_pool_RAII(MYSQL **sql, sql_connection_pool *connpool)
{
    //将一个指针指向的内容复制到另一个指针指向的位置
    *sql=connpool->get_sql_connection();

    conRAII=*sql;

    poolRAII=connpool;

}

sql_connection_pool_RAII::~sql_connection_pool_RAII()
{
    poolRAII->return_sql_connection(conRAII);
}