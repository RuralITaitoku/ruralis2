#include"ruralis2.h"
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

using namespace std;




ruralis2_http::ruralis2_http() {
    cout << FL "コンストラクタ" << endl;
    server_fd = 0;
    client_fd = 0;
}

ruralis2_http::~ruralis2_http() {
    cout << FL "デストラクタ" << endl;
}


int ruralis2_http::recv_ch(char &ch) {



    buf_i++;
    if (buf_e <= buf_i) {
        buf_i = 0;
        buf_e = recv(client_fd, recv_buffer, sizeof(recv_buffer) - 1, 0);
        if (buf_e < 0) {
            cout << FILE_LINE << client_fd 
                << " n=" << buf_e << ", errno=" << errno << endl;
            switch(errno) {
            case ECONNRESET:
                cout << FILE_LINE << client_fd <<  "ECONNRESET(切断されてます。)"
                 << endl;               
                return -2;
            case ECONNABORTED:
               cout << FILE_LINE << client_fd <<  "ECONNABORTED(切断されてます。)"
                 << endl;               
                return -2;
            case ENOTSOCK:
               cout << FILE_LINE << client_fd <<  "ENOTSOCK"
                 << endl;               
                return -2;
            case EBADF:
               cout << FILE_LINE << client_fd <<  "EBADF Bad File Descriptor"
                 << endl;               
                return -2;
            case EAGAIN:
               cout << FILE_LINE << client_fd <<  "EAGAIN Resource temporarily unavailable"
                 << endl;               
                return -2;
            }

            char msg[1024];
            snprintf(msg, sizeof(msg), "%s %d read error!!", FILE_LINE, client_fd);
            throw runtime_error(msg);
        }
        if (buf_e == 0) {
            cout << FILE_LINE << client_fd <<  "受信終了" << endl;
            return -1;
        }
    }
    // return (int)recv_buffer[buf_i];

    return 0;
}


void *ruralis2_http_recv_thread(void *arg) {
    ruralis2_http *http =  (ruralis2_http*)arg;
    http->recv_thread();
    delete http;
    return NULL;
}


void ruralis2_http::recv_thread() {
    DP("受信スレッド開始--------");
    DP("server_fd=" << server_fd);
    DP("client_fd=" << client_fd);
    DP("top_dir=" << top_dir);

}

int ruralis2_http::start() {
    struct sockaddr_in serv_addr;
    // ソケットを作成する
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        DP("ソケット作成失敗");
        return ERR_CREATE_SOCKET;
    }
    // ソケットにアドレスを割り当てる
    memset((char *)&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port_no);
    int bind_result = bind(server_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    if (bind_result < 0) {
        DP("バインドエラー" << port_no);
        close(server_fd);
        return ERR_BINDING;
    }
    // クライアントからの接続を待つ
    listen(server_fd, 16);
    while(1) {
        DP("接続待ち server_fd=" << server_fd);
        int newfd = accept(server_fd, NULL, NULL);
        if (newfd < 0) {
            DP("acceptエラー sever_fd=" << server_fd);
            return ERR_ACCEPT;
        }
        // タイムアウト構造体
        struct timeval timeout;
        timeout.tv_sec = 6; 
        timeout.tv_usec = 0;
        // SO_RCVTIMEO オプションの設定
        DP("タイムアウトの設定");
        if (setsockopt(newfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
            DP("ソケットへのタイムアウトの設定エラー");
            return ERR_SET_TIMEOUT;
        }
        DP("受信スレッド作成");
        ruralis2_http *client;
        client = new ruralis2_http();
        client->port_no = port_no;
        client->top_dir = top_dir;
        client->server_fd = server_fd;
        client->client_fd = newfd;
        pthread_t thread;
        pthread_create(&thread, NULL, ruralis2_http_recv_thread, client);
        pthread_detach(thread);
    }
}
