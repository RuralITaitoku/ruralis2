
#ifndef RURALIS2_H
#define RURALIS2_H
#include <iostream>
#include <map>
#include <vector>
#include <sqlite3.h>
#include <functional>

#define DP(x) std::cout << __FILE__":" << __LINE__ << " " << x << std::endl
#define EP(x) std::cerr << __FILE__":" << __LINE__ << " " << x << std::endl


class ruralis2_bytes {
    int content_type(std::string &file_name);

public:
    enum ErrorCode {
        OK = 0,
        NG = 1,
        ERR_MALLOC = 2,
        ERR_OPEN = 3,
        NOT_FOUND = 4
    };
    size_t size;
    size_t capacity;
    std::string type;
    char *body;

    ruralis2_bytes();
    ~ruralis2_bytes();

    int add_byte(char ch);
    int load(std::string &file_name);
    ruralis2_bytes& operator=(std::string &other);
};

class ruralis2_template {
private:
    std::vector<std::string> temp_vec;

public:
    enum ErrorCode {
        OK = 0,
        NG = 1,
        ERR_LOAD = 2,
    };
    ruralis2_template();
    ~ruralis2_template();

    int load(std::string& file_name);
    int render(std::map<std::string, std::string>& map
                , std::string& result);



    // std::string esc_html(const std::string& str);
};

//std::string ruralis_sqltext(const std::string& sql);

class ruralis2_sql {
private:
    sqlite3 *db;
public:
    enum ErrorCode {
        OK = 0,
        ERR_OPEN = 1,
        ERR_SQL = 2,
    };
    std::string err_msg;

    ruralis2_sql();
    ~ruralis2_sql();

    int open(const char *file_name);
    std::string sqltext(const std::string& sql);
    int create_table(const std::string &table_name
                    , const std::vector<std::string> &columns);
    int insert(const std::string &table_name
                    , const std::vector<std::string> &columns
                    , const std::vector<std::string> &values);
    int insert(const std::string &table_name
                    , const std::vector<std::string> &columns
                    , const std::vector<std::vector<std::string> > &values);
    int update(const std::string &table_name
            , const std::vector<std::string> &columns
            , const std::vector<std::string> &values
            , const std::string &where);


    int exec(const std::string &sql
                    , const std::vector<std::vector<std::string> > &values);
    int exec(const std::string &sql);
    int select(const std::string &sql, 
                std::vector<std::vector<std::string> > &result_data);
    void close();
};

class ruralis2_http {
private:
    unsigned char recv_buffer[256];
    int buf_e;
    int buf_i;
    int recv_ch(char &ch);

    int server_fd;
    int client_fd;
    std::string request;
    ruralis2_bytes request_content;
    std::string response;
    std::string cookie;
    int recv_request();
    int recv_request_content(int content_length);
    int analyze_request();
    int analyze_param();
    int file_content();
    int response_not_found();
    int send_response();
    bool is_keep_alive();
    int line_render(const std::string& line, std::string& html);

public:
    enum ErrorCode {
        RECV_END = -1,
        OK = 0,
        NG = 1,
        ERR_CREATE_SOCKET = 2,
        ERR_BINDING = 3,
        ERR_ACCEPT  = 4,
        ERR_SET_SOCKET_OPT = 5,
        ERR_RECV = 6,
        ERR_FILE = 7,
        ERR_SEND = 8,
    };

    int port_no;
    std::string top_dir;
    std::map<std::string, std::string> request_map;
    std::map<std::string, std::string> param_map;
    std::string err_msg;
    // std::function<int(ruralis2_http&)> content_func;
    int (*content_func)(ruralis2_http&);
    ruralis2_bytes content;

    ruralis2_http();
    ~ruralis2_http();

    int start();
    void recv_thread();

    std::string get_request_path();
    std::string urldecode(const std::string& encoded_str);
    std::string urlencode(const std::string& str);
    static std::string esc_html(const std::string& str);

    int markdown_render(const std::string& md, std::string& html);

    int responseOK();
    int response_see_other(std::string &location);
    bool is_post();
    bool is_smartphone();
};


#endif /* RURALIS2_H*/
