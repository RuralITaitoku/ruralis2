#include"ruralis2.h"
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sstream>
using namespace std;



ruralis2_http::ruralis2_http() {
    DP("コンストラクタ");
    server_fd = 0;
    client_fd = 0;
    buf_i = 0;
    buf_e = 0;

    err_msg = "";
    content_func = NULL;
}

ruralis2_http::~ruralis2_http() {
    DP("デストラクタ");
    if (server_fd) {
        close(server_fd);
        server_fd = 0;
    }
    if (client_fd) {
        close(client_fd);
        client_fd = 0;
    }
}

int ruralis2_http::recv_ch(char &ch) {
    buf_i++;
    if (buf_e <= buf_i) {
        buf_i = 0;
        buf_e = recv(client_fd, recv_buffer, sizeof(recv_buffer) - 1, 0);
        if (buf_e < 0) {
            DP("client_fd=" << client_fd << ", n=" << buf_e);
            switch(errno) {
            case ECONNRESET:
                err_msg = "ECONNRESET(切断されてます。)";
                return ERR_RECV;
            case ECONNABORTED:
                err_msg = "ECONNABORTED(切断されてます。)";
                return ERR_RECV;
            case ENOTSOCK:
                err_msg = "ENOTSOCK";
                return ERR_RECV;
            case EBADF:
                err_msg = "EBADF Bad File Descriptor";
                return ERR_RECV;
            case EAGAIN:
                err_msg = "EAGAIN Resource temporarily unavailable";
                return ERR_RECV;
            }
            err_msg = "client_fd=";
            err_msg += client_fd;
            err_msg += " , errno=";
            err_msg += errno;
            return ERR_RECV;
        }
        if (buf_e == 0) {
            return RECV_END;
        }
    }
    // return (int)recv_buffer[buf_i];
    ch = recv_buffer[buf_i];
    return OK;
}


void *ruralis2_http_recv_thread(void *arg) {
    ruralis2_http *http =  (ruralis2_http*)arg;
    http->recv_thread();
    delete http;
    return NULL;
}




#define WHITESPACE " \n\r\t\f\v"
void line_splitting(string &src, vector<string> &dst) {

    istringstream iss(src);
    while(1) {
        dst.push_back("");
        size_t last = dst.size() - 1;
        string line;
        if (getline(iss, line)) {
            size_t ltrim = line.find_last_not_of(WHITESPACE);
            if (ltrim == string::npos) {
                dst[last] = line;
            } else {
                dst[last] = line.substr(0, ltrim + 1);
            }
        } else {
            break;
        }
    }
}
size_t skip_space(string &src, size_t pos) {
    size_t spos = src.find_first_not_of(WHITESPACE, pos);
    return (spos == string::npos) ? pos : spos;
}

size_t next_space(string &src, size_t pos) {
    return src.find_first_of(WHITESPACE, pos);
}

/*
size_t ruralis_skip_space(string &src, size_t pos) {
    size_t spos = src.find_first_not_of(WHITESPACE, pos);
    return (spos == string::npos) ? pos : spos;
}

size_t ruralis_next_space(string &src, size_t pos) {
    return src.find_first_of(WHITESPACE, pos);
}
*/

int ruralis2_http::analyze_request() {
    vector<string> lines;
    line_splitting(request, lines);
    request_map.clear();
    for (size_t i = 0; i < lines.size(); i++) {
        if (i == 0) {
            size_t cmd_e = next_space(lines[i], 0);
            request_map["Command"] = lines[i].substr(0, cmd_e);
            DP("コマンド:" << request_map["Command"]);

            size_t path_s = skip_space(lines[i], cmd_e);
            DP("path_s=" << path_s);
            size_t path_e = next_space(lines[i], path_s);
            DP("path_e=" << path_e);
            DP("str_size=" << lines[i].size());
            request_map["Path"] = lines[i].substr(path_s, path_e - path_s);
            DP("パス:" << request_map["Path"]);
        } else {
            size_t name_e = lines[i].find_first_of(":");
            if (name_e == string::npos) continue;
            string pname = lines[i].substr(0, name_e);
            //DP("変数名:" << pname);
            size_t value_s = skip_space(lines[i], name_e + 1);
            if (value_s == string::npos) continue;
            string v = lines[i].substr(value_s);
            // DP("値:" << v);
            request_map[pname] = v;
        }
    }
    return OK;
}

int ruralis2_http::analyze_param() {
    DP("パラメータ解析");
    string path = request_map["Path"];
    size_t n = path.find("?");
    string param;
    if (n != string::npos) {
        param = path.substr(n + 1);
    }
    if (request_content.size > 0) {
        // POSTの場合は追加
        if (param.size() == 0) {
            param.assign(request_content.body, request_content.size);
        } else {
            string str;
            str.assign(request_content.body, request_content.size);
            param += "&";
            param += str;
        }
    }
    DP(param);
    size_t si = 0;
    size_t ei = 0;
    while(1) {
        ei = param.find("=", si);
        if (ei == string::npos) {
            break;
        }
        string name = param.substr(si, ei - si);
        si = ei + 1;
        ei = param.find("&", si);
        if (ei == string::npos) {
            param_map[name] = urldecode(param.substr(si));
            DP(name << "=" << param_map[name]);
            DP("---------");
            break;
        }
        param_map[name] = urldecode(param.substr(si, ei - si));
        si = ei + 1;
        DP(name << "=" << param_map[name]);
    }
    return OK;
}

bool ends_request(string &req) {
    size_t idx;
    if (req.size() < 4) {
        return false;
    }
    idx = req.find("\r\n\r\n", req.size() - 4);
    if (idx != string::npos) {
        return true;
    } else {
        return false;
    }
}

int ruralis2_http::recv_request() {
    int result;
    request = "";
    DP("要求受信");
    while(1) {
        char ch;
        if ((result = recv_ch(ch)) != 0) {
            if (result == RECV_END) {
                DP("切断されました。");
                return RECV_END;
            } else {
                DP(err_msg);
                return ERR_RECV;
            }
        }
        request += ch;

        if (ends_request(request)) break;
    }
    DP("---------\n\n" << request);
    return OK;
}
int ruralis2_http::recv_request_content(int content_length) {
    int result;
    request_content.size = 0;
    DP("要求コンテンツ受信");
    for (int i = 0; i < content_length; i++) {
        char ch;
        if ((result = recv_ch(ch)) != 0) {
            if (result == RECV_END) {
                DP("切断されました。");
                return RECV_END;
            } else {
                DP(err_msg);
                return ERR_RECV;
            }
        }
        request_content.add_byte(ch);
    }
    return OK;
}


int ruralis2_http::file_content(){
    DP("file_contentの処理");
    auto path = request_map["Path"];
    if (path.size() <= 0) {
        DP("パスの文字列サイズ０");
        return ERR_FILE;
    } 
    if (path[0] != '/') {
        DP("パスが/始まりじゃない。path=" << path);
        return ERR_FILE;
    }
    if (path.find("/", path.size() -1) != string::npos) {
        DP("パスの終わりが/");
        return ERR_FILE;
    }
    string file_name = top_dir;
    file_name += path;
    DP("トップフォルダ:" << top_dir);
    DP("file_name:" << file_name);
    if (content.load(file_name)) {
        DP("ファイルエラー");
        return ERR_FILE;
    }
    return OK;
}
int ruralis2_http::responseOK() {
    response = "HTTP/1.1 200 OK\r\n";
    response += "content-length: ";
    response += to_string(content.size);
    response += "\r\n";
    // 
    if (content.type.size() > 0) {
        response += "content-type: ";
        response += content.type;
        response += "\r\n";
    }
    // 
    if (cookie.size() > 0) {
        response += "set-cookie: ";
        response += cookie;
        response += "\r\n";
    }
    if (is_keep_alive()) {
        response += "Connection: keep-alive\r\n";
    }
    response += "\r\n";
    // 
    return OK;
}
int ruralis2_http::response_see_other(string& location) {
    response = "HTTP/1.1 303 See Other\r\n";
    response += "Location: ";
    response += location;
    response += "\r\n\r\n";
    return OK;
}
int ruralis2_http::response_not_found() {
    string error = "<html><body>"
                    "404 Not Found"
                    "</body></html>\r\n";

    response = "HTTP/1.1 404 Not Found  \r\n";
    response += "content-length: ";
    response += to_string(error.size());
    response += "\r\n\r\n";
    response += error;
    return OK;
}
int ruralis2_http::send_response() {
    int result;
    char sbuff[256];
    const char *res_char = response.c_str();
    DP("レスポンス送信");
    for (size_t i = 0; i < response.size(); i++) {
        int ac = i % sizeof(sbuff);
        sbuff[ac] = res_char[i];
        result = OK;
        if (i == response.size() - 1) {
            result = send(client_fd, sbuff, ac + 1, 0); 
        }
        if (ac == sizeof(sbuff) - 1) {
            result = send(client_fd, sbuff, sizeof(sbuff), 0);
        }
        if (result < 0) {
            DP("送信エラー。client_fd=" << client_fd);
            return ERR_SEND;
        }
    }
    DP("コンテンツ送信");
    for (size_t i = 0; i < content.size; i++) {
        int ac = i % sizeof(sbuff);
        sbuff[ac] = content.body[i];
        result = OK;
        if (i == content.size - 1) {
            result =  send(client_fd, sbuff, ac + 1, 0);
        }
        if (ac == sizeof(sbuff) - 1) {
            result = send(client_fd, sbuff, sizeof(sbuff), 0);
        }
        if (result < 0) {
            DP("送信エラー。client_fd=" << client_fd);
            return ERR_SEND;
        }
    }
    DP("---------\n\n" << response);
    return OK;
}
bool ruralis2_http::is_keep_alive() {
    string cm = request_map["Connection"];
    DP("Connection: " << cm);
    // キープアライブ無し。
    // バグだと思うけどFirefoxの動きがおかしくなる。
    if (cm == "keep-alive!!!") {
        return true;
    } else {
        return false;
    }
}
bool ruralis2_http::is_post() {
    if (request_map["Command"] == "POST") {
        return true;
    } else {
        return false;
    }
}
bool ruralis2_http::is_smartphone() {
    string user_agent = request_map["User-Agent"];
    if (user_agent.find("iPhone") != string::npos) {
        return true;
    }
    if (user_agent.find("Android") != string::npos) {
        return true;
    }
    return false;
}

void ruralis2_http::recv_thread() {
    DP("受信スレッド開始--------" << client_fd);
    DP("top_dir=" << top_dir);
    int okng;
    while(1) {
        okng = recv_request();
        if (okng) {
            DP("受信エラー");
            break;
        }
        analyze_request();
        if (is_post()) {
            int content_length = std::stoi(request_map["Content-Length"]);
            okng = recv_request_content(content_length);
            if (okng) {
                DP("受信エラー");
                break;
            }
        }
        analyze_param();
        okng = file_content();
        if (okng == OK) {
            responseOK();
            if (send_response() == OK) {
                DP("OK送信成功");
            };
        } else {
            okng = (*content_func)(*this);
            if (okng == OK) {
                //responseOK();
                if (send_response() == OK) {
                    DP("OK送信成功");
                };
            } else {
                response_not_found();
                if (send_response() == OK) {
                    DP("404送信成功");
                };
            }
        }
        if (!is_keep_alive()) {
            break;
        } 
    }
    DP("受信スレッド終了--------" << client_fd);
}

int ruralis2_http::start() {
    struct sockaddr_in serv_addr;
    DP("ポート番号:" << port_no);
    DP("top_dir:" << top_dir);

    // ソケットを作成する
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        DP("ソケット作成失敗");
        return ERR_CREATE_SOCKET;
    }
    // サーバーソケットにオプションを設定。
    int reuseaddr = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR
                , &reuseaddr, sizeof(reuseaddr))) {
        DP("ソケットオプション設定失敗");
        return ERR_SET_SOCKET_OPT;
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
        err_msg = "バインドエラー ポート番号：";
        err_msg += std::to_string(port_no);
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
        /*
        // タイムアウト構造体
        struct timeval timeout;
        timeout.tv_sec = 6; 
        timeout.tv_usec = 0;
        // SO_RCVTIMEO オプションの設定
        DP("タイムアウトの設定");
        if (setsockopt(newfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
            DP("ソケットへのタイムアウトの設定エラー");
            return ERR_SET_SOCKET_OPT;
        }*/
        DP("受信スレッド作成");
        ruralis2_http *client;
        client = new ruralis2_http();
        client->port_no = port_no;
        client->top_dir = top_dir;
        client->server_fd = 0;
        client->client_fd = newfd;
        client->content_func = content_func;
        pthread_t thread;
        pthread_create(&thread, NULL, ruralis2_http_recv_thread, client);
        pthread_detach(thread);
    }
}

string ruralis2_http::get_request_path() {
    string path = request_map["Path"];
    size_t n = path.find("?");
    if (n == string::npos) {
        return path;
    }
    return path.substr(0, n);
}

bool is_hex_digit(char c) {
  return (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f') || (isdigit(c));
}

std::string ruralis2_http::urldecode(const std::string& encoded_str) {
  std::string decoded_str;

  for (size_t i = 0; i < encoded_str.size(); ++i) {
    if (encoded_str[i] == '%') {
      if (i + 2 < encoded_str.size() && is_hex_digit(encoded_str[i + 1]) &&
          is_hex_digit(encoded_str[i + 2])) {
        // 16進数コードの場合
        char hex_char[3] = {encoded_str[i + 1], encoded_str[i + 2], '\0'};
        unsigned int code = std::stoul(hex_char, nullptr, 16);
        decoded_str += static_cast<char>(code);
        i += 2;
      } else {
        // エラー処理
        std::cerr << "Invalid URL encoding: " << encoded_str << std::endl;
        return "";
      }
    } else if (encoded_str[i] == '+') {
        decoded_str += ' ';
    } else {
        decoded_str += encoded_str[i];

    }
  }

  return decoded_str;
}

std::string ruralis2_http::urlencode(const std::string& str) {
    std::string encoded_str;

    for (size_t i = 0; i < str.size(); i++) {
        unsigned char c = str[i];
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '*' ||
            c == ',' || c == ':' || c == '@' || c == '=' || c == '+' ||
            c == '$' || c == '(' || c == ')' || c == '[' || c == ']' ||
            c == '{' || c == '}' || c == ';' || c == '&' || c == '%' ||
            c == '<' || c == '>' || c == ',' || c == '?' || c == '/' ||
            c == '\\' || c == '^' || c == '`' || c == '|' || c == '~') {
            encoded_str += c;
        } else {
            encoded_str += "%";
            char hex_digits[3];
            snprintf(hex_digits, 3, "%02X", (int) c);
            hex_digits[2] = '\0';
            encoded_str.append(hex_digits, 2);
        }
    }
    return encoded_str;
}

string ruralis2_http::esc_html(const string& str) {
    string esc_html;
    for (size_t i = 0; i < str.size(); i++) {
        unsigned char c = str[i];
        if (c == '<') {
            esc_html += "&lt;";
        } else if (c == '>') {
            esc_html += "&gt;";
        } else if ( c == '&') {
            esc_html += "&amp;";
        } else if ( c == '"') {
            esc_html += "&quot;";
        } else {
            esc_html += c;
        }
    }
    return esc_html;
}


int ruralis2_http::line_render(const string& line, string& html) {
    ostringstream oss;
    bool img = false;
    size_t ssi = 0;
    size_t si = 0;
    size_t ei = 0;
    while(1) {
        si = line.find("[", ssi);
        if (si != string::npos) {
            img = false;
            if (si > 0) {
                if (line[si - 1] == '!') {
                    img = true;
                }
            }
            if (img) {
                oss << esc_html(line.substr(ssi, si - ssi - 1));
            } else {
                oss << esc_html(line.substr(ssi, si - ssi));
            }
            si += 1;
            ei = line.find("]", si);
            if (ei != string::npos) {
                string name = line.substr(si, ei - si);
                si = ei + 1;
                ei = line.find("(", si);
                DP("si = " << si);
                DP("ei = " << ei);
                string url;
                if (si == ei) {
                    si = ei + 1;
                    ei = line.find(")", si);
                    if (ei != string::npos) {
                        url = line.substr(si, ei - si);
                        si = ei +1;
                    }
                }
                if (url.size() == 0) {
                    oss << "<a href='./" << urlencode(name) << "'>" << name << "</a>";
                } else {
                    if (img) {
                        oss << "<img src='" << url << "' alt='" << name << "' />";
                    } else {
                        oss << "<a href='" << url << "'>" << name << "</a>";
                    }
                }
                ssi = si;
            } else {
                EP(line);
                EP("]がありません。");
                oss << line.substr(si);
                html = oss.str();
                return ruralis2_http::NG;
            }
        } else {
            // oss << line.substr(ssi);
            // ssi = si;
            break;
        }
    }
    oss << esc_html(line.substr(ssi)) << "<br />" << endl;
    html = oss.str();
    return ruralis2_http::OK;
}


int ruralis2_http::markdown_render(const std::string& md, std::string& html) {
    /*
    let mut toc_html = String::from("<div class='table_of_contents'>\n");
    toc_html.push_str(&format!("　　　　　<a href='#row{}'>{}</a><br/>\n", bm_index, &esc_html(&line)));
    html.push_str(&format!("<h4 id='row{}'>{}</h4>\n", bm_index, &esc_html(&line)));
    */
    istringstream iss(md);
    string line;
    vector<string> lines;
    ostringstream oss;
    DP("");
    while (std::getline(iss, line)) {
        lines.push_back(line);
    }
    int si = 0;
    for (auto l : lines) {
        si = l.find("###");
        if((si = l.find("###-")) == 0) {
            oss << "<h4>" << l.substr(4) << "</h4>" << endl;
        } else if((si = l.find("###")) == 0) {
            oss << "<h4>" << l.substr(3) << "</h4>" << endl;
        } else if((si = l.find("##-")) == 0) {
            oss << "<h3>" << l.substr(3) << "</h3>" << endl;
        } else if((si = l.find("##")) == 0) {
            oss << "<h3>" << l.substr(2) << "</h3>" << endl;
        } else if((si = l.find("#-")) == 0) {
            oss << "<h2>" << l.substr(2) << "</h2>" << endl;
        } else if((si = l.find("#")) == 0) {
            oss << "<h2>" << l.substr(1) << "</h2>" << endl;
        } else if((si = l.find("---")) == 0) {
            oss << "<hr/>" << endl;
        } else {
            string html;
            line_render(l, html);
            oss << html << endl;
        }
    }
    html = oss.str();
    return OK;
}
