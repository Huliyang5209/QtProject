#ifndef HTTPCONNECTION_H
#define HTTPCONNECTION_H
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
#include <map>


#include"../lock/lock.h"
#include"../CGImysql/sql_connection_pool.h"
#include"../log/log.h"
#include"../timer/lst_timer.h"
class http_conn1
{
    public:

    static const int FILENAME_LEN = 200;
    static const int READ_BUFFER_SIZE = 2048;
    static const int WRITE_BUFFER_SIZE = 1024;

    enum HTTP_CODE
    {
        NO_REQUEST,
        GET_REQUEST,
        BAD_REQUEST,
        NO_RESOURCE,
        FORBIDDEN_REQUEST,
        FILE_REQUEST,
        INTERNAL_ERROR,
        CLOSED_CONNECTION
    };

    enum METHOD
    {
        GET = 0,
        POST,
        HEAD,
        PUT,
        DELETE,
        TRACE,
        OPTIONS,
        CONNECT,
        PATH
    };

    enum CHECK_STATE
    {
        CHECK_STATE_REQUESTLINE = 0,
        CHECK_STATE_HEADER,
        CHECK_STATE_CONTENT
    };

    enum LINE_STATUS
    {
        LINE_OK = 0,
        LINE_BAD,
        LINE_OPEN
    };

    public:
    http_conn1(){};
    ~http_conn1(){};

    public:

    void init(int sockfd,const sockaddr_in &addr,char*,int,int,string user,string passwd,string sql_dbname);

    void process();
    bool read_one();
    bool write();
    void close_conn(bool real_close = true);

    sockaddr_in *get_address()
    {
        return &m_address;
    }

    void initmysql_result(sql_connection_pool* sql_pool);
    int timer_flag;
    int improv;
    
    private:

    void init();
    char *get_line() { return m_read_buf + m_start_line; };

    LINE_STATUS parse_line();
    HTTP_CODE parse_request_line(char *text);
    HTTP_CODE parse_headers(char *text);
    HTTP_CODE parse_contents(char *text);

    HTTP_CODE  process_read();
    HTTP_CODE do_request();
    void unmap();

    bool add_response(const char* format,...);
    bool add_content(const char *content);
    bool add_status_line(int status, const char *title);
    bool add_headers(int content_length);
    bool add_content_type();
    bool add_content_length(int content_length);
    bool add_linger();
    bool add_blank_line();
    bool process_write(HTTP_CODE ret);
    
    public:
    MYSQL *mysql;
    int m_state;
    static int m_epollfd;
    static int m_user_count;

    private:

    int m_sockfd;
    int m_start_line;
    map<string, string> m_users;

    struct stat m_file_stat;
    struct iovec m_iv[2];
    int m_iv_count;
    char *m_file_address;
    int bytes_to_send;
    int bytes_have_send;
    
    int m_close_log;

    char *doc_root;
    char m_real_file[FILENAME_LEN];

    int cgi;
    char *m_string;

    METHOD m_method;
    char* m_url;
    char* m_version;
    char* host;
    bool linger;
    int m_content_length;

    CHECK_STATE m_check_state;

    long m_read_idx;
    long m_write_idx;
    long m_checked_idx;
    char m_read_buf[READ_BUFFER_SIZE];
    char m_write_buf[READ_BUFFER_SIZE];

    int m_TRIGMode;

    sockaddr_in m_address;
    char sql_user[225];
    char sql_passwd[225];
    char sql_dbname[225];
};

#endif