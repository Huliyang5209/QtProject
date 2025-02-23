#ifndef _CONNECTION_POOL_
#define _CONNECTION_POOL_

#include <mysql/mysql.h>
#include<list>
#include <error.h>
#include <string>
#include"../lock/lock.h"
#include"../log/log.h"
//数据库池(用户----服务器---数据库)
class sql_connection_pool
{ 
    private:
    int m_MaxConn;
    list<MYSQL *>sql_connection_list;
    int had_used_conn;
    int free_use_conn;
    locker m_lock;
    sem m_sem_reserve;


    public: 
    sql_connection_pool();
    ~sql_connection_pool();

    //提前创建多个数据库连接
    void init(string url, string User, string PassWord, string DBName, int Port, int MaxConn, int close_log);
    //获取数据库连接
    MYSQL *get_sql_connection();
    //放回数据库连接
    bool return_sql_connection(MYSQL *con);
    //销毁数据库连接
    void destory_sql_pool();
    //获取剩余空闲数据库连接数量
    int get_free_use_conn_num();

    //单实例模式,实例化数据库连接池，确保只有一个连接对象，避免产生多个对象消耗过多的资源
    static sql_connection_pool *get_sql_connect_to_instance();

    public:
    string m_url;
    int m_port;
    string  m_user;
    string  m_passwd;
    string  m_dbName;

    int m_close_log;

};

//使用RAII封装sql_pool，约束其生存周期
class sql_connection_pool_RAII
{
    public:
    sql_connection_pool_RAII(MYSQL **con, sql_connection_pool *connPool);
    ~sql_connection_pool_RAII();

    private:
    MYSQL *conRAII;
    sql_connection_pool *poolRAII;
};

#endif