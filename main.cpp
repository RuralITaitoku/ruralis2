#include"ruralis2.h"
#include <algorithm>
#include "test.h"

using namespace std;

ruralis2_sql sqlite;
ruralis2_template temp_zubolite;
ruralis2_template temp_zubolite_smp;
ruralis2_template temp_zubolite_input;


int load_template() {
    int result;
    string file_name;
    file_name = "templates/zubolite.html";
    result = temp_zubolite.load(file_name);
    if (result) {
        return result;
    }
    file_name = "templates/zubolite_smp.html";
    result = temp_zubolite_smp.load(file_name);
    if (result) {
        return result;
    }
    file_name = "templates/zubolite_input.html";
    result = temp_zubolite_input.load(file_name);
    if (result) {
        return result;
    }
    DP("テンプレートファイル読み込み完了");
    return ruralis2_http::OK;
}
int select_html(const string& name, string& html) {
    int okng;

    string sql = "select html from page where name=";
    sql += sqlite.sqltext(name);
    DP(sql);
    vector<vector<string>> rows;
    okng = sqlite.select(sql, rows);
    if (okng) {
        DP(sqlite.err_msg);
        return ruralis2_http::NG;
    }
    if (rows.size() == 0) {
        return ruralis2_http::NG;
    } else {
        html = rows[0][0];
        return ruralis2_http::OK;
    }
}
int select_wtml(const string& name, string& wtml) {
    int okng;

    string sql = "select wtml from page where name=";
    sql += sqlite.sqltext(name);
    DP(sql);
    vector<vector<string>> rows;
    okng = sqlite.select(sql, rows);
    if (okng) {
        DP(sqlite.err_msg);
        return ruralis2_http::NG;
    }
    DP("size() = " << rows.size());
    if (rows.size() == 0) {
        return ruralis2_http::NG;
    } else {
        wtml = rows[0][0];
        return ruralis2_http::OK;
    }
}


int post_func(ruralis2_http& http) {
    int okng;
    string page = http.param_map["page"];
    string wtml = http.param_map["wtml"];

    vector<string> columns;
    columns.push_back("name");
    columns.push_back("wtml");    
    columns.push_back("html");    
    vector<string> values;
    values.push_back(page);
    values.push_back(wtml);
    values.push_back(wtml);

    string html_sidemenu;
    okng = select_html("SideMenu", html_sidemenu);
    if (okng) {
        DP("サイドメニューを取得できませんでした。");
    }
    string btn = http.param_map["btn"];
    if (btn == "go") {
        string url = "./";
        url += http.urlencode(page);
        http.response_see_other(url);
        return ruralis2_http::OK;
    } else if (btn == "insert") {
        string html;
        okng = http.markdown_render(wtml, html);
        if (okng) {
            DP("マークダウンレンダリングに失敗");
            return ruralis2_http::NG;
        }
        values[2] = html;
        okng = sqlite.insert("page", columns, values);
        if (okng) {
            DP(sqlite.err_msg);
            return ruralis2_http::NG;
        }
        string url = "./";
        url += http.urlencode(page);
        http.response_see_other(url);
        return ruralis2_http::OK;
    } else if (btn == "update") {
        string html;
        okng = http.markdown_render(wtml, html);
        if (okng) {
            DP("マークダウンレンダリングに失敗");
            return ruralis2_http::NG;
        }
        values[2] = html;
        string where = "name=";
        where += sqlite.sqltext(page);
        okng = sqlite.update("page", columns, values, where);
        if (okng) {
            DP(sqlite.err_msg);
            return ruralis2_http::NG;
        }
        string url = "./";
        url += http.urlencode(page);
        http.response_see_other(url);
        return ruralis2_http::OK;
    } else if (btn == "edit") {
        DP("-------------------------編集");
        string html_input;
        map<string, string> map;
        map["page"] = page;
        string wtml;
        okng = select_wtml(page, wtml);
        if (okng == ruralis2_http::OK) {
            DP("「" << page << "」があります。");
            map["btn"] = "update";
            map["wtml"] = wtml;
            map["btn_label"] = "更新";
        } else {
            DP("「" << page << "」がありません。");
            map["btn"] = "insert";
            map["btn_label"] = "登録";
        }
        okng = temp_zubolite_input.render(map, html_input);
        if (okng) {
            return ruralis2_http::NG;
        }
        string html_page;
        map["title"] = page;
        map["body|safe"] = html_input;
        map["sidemenu|safe"] = html_sidemenu;
        okng = temp_zubolite.render(map, html_page);
        http.content = html_page;
        http.responseOK();
        return ruralis2_http::OK;
    }

    return ruralis2_http::OK;
}

int get_func(ruralis2_http& http) {
    string path = http.get_request_path();
    if (http.isPost()) {
        return post_func(http);
    }
    string sql;
    int okng;

    string html_sidemenu;
    okng = select_html("SideMenu", html_sidemenu);
    if (okng) {
        DP("サイドメニューを取得できませんでした。");
    }


    if (path.find("/") == 0) {
        path = path.substr(1);
    }
    DP("パス:" << path);
    path = http.urldecode(path);
    if (path.size() == 0) {
        path = "こんにちは";
    }

    sql = "select name, wtml, html from page where name=";
    sql += sqlite.sqltext(path);
    DP(sql);
    vector<vector<string>> rows;
    okng = sqlite.select(sql, rows);
    if (okng) {
        EP(sqlite.err_msg);
        return ruralis2_http::NG;
    }
    DP("rows.size()=" << rows.size());
    if (rows.size() == 0) {
        DP("ページが見つからない場合は、入力画面を開く");
        string html_input;
        map<string, string> map;
        map["page"] = path;
        
        map["btn"] = "insert";
        map["btn_label"] = "登録";
        okng = temp_zubolite_input.render(map, html_input);
        if (okng) {
            return ruralis2_http::NG;
        }
        string html;
        map["body|safe"] = html_input;
        map["sidemenu|safe"] = html_sidemenu;
        okng = temp_zubolite.render(map, html);
        http.content = html;
        http.responseOK();
        return ruralis2_http::OK;
    } else {
 
 
        map<string, string> map;
        map["title"] = rows[0][0];
        map["page"] = rows[0][0];
        map["body|safe"] = rows[0][2];
        map["sidemenu|safe"] = html_sidemenu;
       string html;
        okng = temp_zubolite.render(map, html);
        http.content = html;
        http.responseOK();
        return ruralis2_http::OK;
    }
}


int main(int argc, char *argv[]) {
    int okng;
    string sql;
    //okng = test_all();
    //if (okng) {
    //    DP("テストエラー:" << okng);
    //    return 0;
    //}
    // テンプレートファイル準備
    okng = load_template();
    if (okng) {
        return ruralis2_http::NG;
    }
    // DB準備
    okng = sqlite.open("zubolite.db");
    if (okng) {
        EP(sqlite.err_msg);
        return ruralis2_http::NG;
    }
    sql = "create table if not exists page ("
            "    id integer primary key autoincrement,"
            "    name TEXT NOT NULL UNIQUE,"
            "    wtml TEXT NOT NULL,"
            "    html TEXT NOT NULL"
            ")";
    okng = sqlite.exec(sql);
    if (okng) {
        EP(sqlite.err_msg);
        return ruralis2_http::NG;
    }
     ruralis2_http srv;
    srv.port_no = 1581;
    srv.top_dir = "/var/www/html";
    DP("port_no:" << srv.port_no);
    srv.content_func = get_func;
    int result = srv.start();
    DP("result=" << result);
    if (result) {
        DP(srv.err_msg);
    }
}