#include"ruralis2.h"
#include"test.h"

using namespace std;

int test_ruralis2_sql() {
    DP("SQLテスト");
    string sql;
    ruralis2_sql sqlite;
    int okng;

    sqlite.open("test.db");
    DP("sql execテスト");
    sql = "create table if not exists page ("
            "    id integer primary key autoincrement,"
            "    name TEXT NOT NULL UNIQUE,"
            "    wtml TEXT NOT NULL,"
            "    html TEXT NOT NULL"
            ")";
    okng = sqlite.exec(sql);
    if (okng) {
        EP(sqlite.err_msg);
        DP("sql exec失敗");
        return 1;
    }
 
    return 0;
}

int test_ruralis2_http() {
    ruralis2_http http;

    DP("マークダウンのテスト");
    string md = "[てすと](http://aaaa/)";
    DP("MD:" << md);
    string html;
    http.markdown_render(md, html);
    DP("完了：" << html);
    return 0;
}

int test_all() {
    int sum = 0;

    // sum += test_ruralis2_sql();
    sum += test_ruralis2_http();
    return sum;
}