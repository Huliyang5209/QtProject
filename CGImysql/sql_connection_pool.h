#ifndef _CONNECTION_POOL_
#define _CONNECTION_POOL_

#include <mysql/mysql.h>
#include<list>
#include <error.h>
#include <string>
#include"../lock/lock.h"
#include"../log/log.h"
//���ݿ��(�û�----������---���ݿ�)
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

    //��ǰ����������ݿ�����
    void init(string url, string User, string PassWord, string DBName, int Port, int MaxConn, int close_log);
    //��ȡ���ݿ�����
    MYSQL *get_sql_connection();
    //�Ż����ݿ�����
    bool return_sql_connection(MYSQL *con);
    //�������ݿ�����
    void destory_sql_pool();
    //��ȡʣ��������ݿ���������
    int get_free_use_conn_num();

    //��ʵ��ģʽ,ʵ�������ݿ����ӳأ�ȷ��ֻ��һ�����Ӷ��󣬱����������������Ĺ������Դ
    static sql_connection_pool *get_sql_connect_to_instance();

    public:
    string m_url;
    int m_port;
    string  m_user;
    string  m_passwd;
    string  m_dbName;

    int m_close_log;

};

//ʹ��RAII��װsql_pool��Լ������������
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