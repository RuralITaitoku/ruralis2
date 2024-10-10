#ifndef RURALIS_H
#define RURALIS_H
#include <iostream>
#include <map>
#include <vector>
#include <sqlite3.h>


#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
//#define TAG(x) __FILE__ ":"  STRINGIFY(x)
#define FILE_LINE __FILE__ ":" TOSTRING(__LINE__) " "


class RuralisBytes {
private:
public:
    std::string content_type;
    std::string file_name;
    int file_size;
    char *body;
    int size;
    int capacity;
    RuralisBytes();
    ~RuralisBytes();

    void clear();
    void add(char ch);
    int load(const std::string &filename);
    void determine_content_type(char* type = NULL);
    void get_string(std::string &dst);
    bool last(std::string &l);
};



std::string ruralis_sqltext(const std::string& sql);

class RuraliSql {
private:
    sqlite3 *db;
public:
    // std::string table_name;
    // std::vector<std::string> columns;
    // std::vector<std::vector<std::string>> data;
    RuraliSql();
    ~RuraliSql();

    void open(const char *file_name);
    std::string create_table(const std::string &table_name
                    , const std::vector<std::string> &columns);

    std::string insert(const std::string &table_name
                    , const std::vector<std::string> &columns
                    , const std::vector<std::string> &values);
    std::string insert(const std::string &table_name
                    , const std::vector<std::string> &columns
                    , const std::vector<std::vector<std::string> > &values);
    std::string update(const std::string &table_name
                    , const std::vector<std::string> &columns
                    , const std::vector<std::string> &values
                    , const std::string &where);


    std::string exec(const std::string &sql
                    , const std::vector<std::vector<std::string> > &values);
    std::string exec(const std::string &sql);
    std::string select(const std::string &sql, 
                std::vector<std::vector<std::string> > &result_data);
    void close();
};


void ruralis_line_splitting(std::string &src, std::vector<std::string> &dst);
void ruralis_load_file(const char *file_name, std::string& dst);
std::size_t ruralis_skip_space(std::string &src, std::size_t pos);
void ruralis_replace_temp(std::string &temp, std::map<std::string, std::string> param_map, std::string &dst);
std::string ruralis_urldecode(const std::string& encoded_str);
std::string ruralis_urlencode(const std::string& str);
std::string ruralis_esc_html(const std::string& str);
void ruralis_random_key(std::string &dst);

class RuralisHttp {
private:
    unsigned char recv_buffer[256];
    int buf_e;
    int buf_i;
    int recv_buf();
    unsigned char send_buffer[256];
    int send_buf_i;
    int add_send_buf(char c);
    int send_buf();
    int send_content();


    void set_param_map(std::string &arg);

public:
    int port_no;
    int server_fd;
    int client_fd;
    int (*func)(RuralisHttp*);
    std::string top_dir;
    std::string request;
    std::vector<std::string> request_lines;
    std::map<std::string, std::string> request_map;
    std::map<std::string, std::string> param_map;
    std::string temp;
    std::string html;
    std::string response;
    void (*content_func)(RuralisHttp &http);
    RuralisBytes response_content;

    RuralisHttp();
    ~RuralisHttp();

    void load_temp(char *file_name);

    void recv_thread(int a_port_no, int a_client_fd);
    void start();
    void close_fd();
    void add_res_http200();
    void add_res_http200(std::string& body);

};




#endif /* RURALIS_H*/
