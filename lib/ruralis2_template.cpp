#include "ruralis2.h"

using namespace std;

ruralis2_template::ruralis2_template() {
    DP("コンストラクタ");
}
ruralis2_template::~ruralis2_template() {
    DP("デストラクタ");
}

int ruralis2_template::load(std::string& file_name) {
    int result;
    ruralis2_bytes bytes;

    if (temp_vec.size() > 0) {
        DP("読み込み済");
        return OK;
    }

    result = bytes.load(file_name);
    if (result) {
        DP("ロードエラー:" << file_name);
        return ERR_LOAD;
    }
    temp_vec.clear();
    bytes.add_byte('\0');
    string temp(bytes.body);
    size_t si = 0;
    size_t ei = 0;
    while(1) {
        ei = temp.find("{{", si);
        if (ei != string::npos) {
            temp_vec.push_back(temp.substr(si, ei - si));
            si = ei + 2;
            ei = temp.find("}}", si);
            if (ei != string::npos) {
                temp_vec.push_back(temp.substr(si, ei - si));
                si = ei + 2;
            } else {
                temp_vec.push_back("Not Founds }}");
                temp_vec.push_back(temp.substr(si));
            }
        } else {
            temp_vec.push_back(temp.substr(si));
            break;
        }
    }
    return OK;
}
int ruralis2_template::render(std::map<std::string, std::string>& map
                    , std::string& result) {
    result = "";
    for (size_t i = 0; i < temp_vec.size(); i++) {
        if ((i % 2) == 0) {
            result += temp_vec[i];
        } else {
            string param_name = temp_vec[i];
            size_t si = param_name.find("|safe");
            if (si != string::npos) {
                result += map[param_name];
            } else {
                result += ruralis2_http::esc_html(map[param_name]);
            }
        }
    }
    return OK;
}
