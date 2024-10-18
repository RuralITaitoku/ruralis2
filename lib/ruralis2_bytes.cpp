#include "ruralis2.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

using namespace std;


ruralis2_bytes::ruralis2_bytes() {
    DP("コンストラクタ");
    size = 0;
    capacity = 4096;
    type = "";
    body = NULL;
}

ruralis2_bytes::~ruralis2_bytes() {
    DP("デストラクタ");
    if (body) {
        free(body);
    }
}

/*
    let content_type = if file_name.ends_with(".html") {
        "text/html"
    } else if file_name.ends_with(".css") {
        "text/css"
    } else if file_name.ends_with(".js") {
        "text/javascript"
    } else if file_name.ends_with(".jpg") || file_name.ends_with(".jpeg") {
        "image/jpeg"
    } else if file_name.ends_with(".png") {
        "image/png"
    } else if file_name.ends_with(".mp4") {
        "video/mp4"
    } else if file_name.ends_with(".ico") {
        "image/x-icon"
    } else {
        "text/plain"
    };
*/
bool ends_with(string &src, string end) {
    if (src.size() < end.size()) {
        return false;
    }
    size_t result = src.find(end, src.size() - end.size());
    if (result == string::npos) {
        return false;
    }
    return true;
}

int ruralis2_bytes::content_type(string &file_name) {
    if (ends_with(file_name, ".html")) {
        type = "text/html";
    } else if (ends_with(file_name, ".css")) {
        type = "text/css";
    } else if (ends_with(file_name, ".js")) {
        type = "text/javascript";
    } else if (ends_with(file_name, ".jpg")) {
        type = "image/jpeg";
    } else if (ends_with(file_name, ".jpeg")) {
        type = "image/jpeg";
    } else if (ends_with(file_name, ".png")) {
        type = "image/png";
    } else if (ends_with(file_name, ".mp4")) {
        type = "video/mp4";
    } else if (ends_with(file_name, ".ico")) {
        type = "image/x-icon";
    } else {
        type = "text/plain";
    }
    return OK;
}


int ruralis2_bytes::add_byte(char ch) {
    if (body == NULL) {
        body = (char*)malloc(capacity);
        if (!body) {
            DP("メモリ確保失敗");
            return ERR_MALLOC;
        }
    }
    if (size == capacity) {
        size_t new_cap = capacity * 2;
        char *new_data = (char*)realloc(body, new_cap);
        if (!new_data) {
            DP("メモリ確保失敗");
            return ERR_MALLOC;
        }
        capacity = new_cap;
        body = new_data;
    }

    body[size] = ch;
    size++;
    return OK;
}

int ruralis2_bytes::load(string &file_name) {
    FILE *fp;
    char buf[4096];
    size_t result;
    fp = fopen(file_name.c_str(), "r");
    if (fp == NULL) {
        DP("ファイルが開けません。"<< file_name);
        return ERR_OPEN;
    }
    // 1行ずつ読み込む
    while ((result = fread(buf, 1, sizeof(buf), fp)) > 0) {
        for (size_t i = 0; i < result; i++) {
            add_byte(buf[i]);
        }
    }
    fclose(fp);
    content_type(file_name);
    return OK;
}


ruralis2_bytes& ruralis2_bytes::operator=(std::string &other) {
    size = 0;
    for (size_t i = 0; i < other.size(); i++) {
        add_byte(other[i]);
    }
    return *this;
}
