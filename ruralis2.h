
#ifndef RURALIS2_H
#define RURALIS2_H
#include <iostream>
#include <map>
#include <vector>
#include <sqlite3.h>


#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define FL __FILE__ ":" TOSTRING(__LINE__) " "
#define DP(x) cout << FL << x << endl


class ruralis2_http {
private:
    unsigned char recv_buffer[256];
    int buf_e;
    int buf_i;
    int recv_ch(char &ch);

    int server_fd;
    int client_fd;


public:

    enum ErrorCode {
        READ_END = -1,
        SUCCESS = 0,
        ERR_CREATE_SOCKET = 1,
        ERR_BINDING = 2,
        ERR_ACCEPT  = 3,
        ERR_SET_TIMEOUT = 4.
        ERR_READ = 5
    };



    int port_no;
    std::string top_dir;

    std::string err_msg;
    ruralis2_http();
    ~ruralis2_http();


    int recv_ch(char &ch);
    int start();
    void recv_thread();

};


#endif /* RURALIS2_H*/
