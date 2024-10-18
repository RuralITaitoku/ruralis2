#include "ruralis2.h"
#include <sstream>
// #include <netinet/in.h>
// #include <sys/socket.h>
// #include <arpa/inet.h>
using namespace std;



std::string ruralis2_sql::sqltext(const std::string& sql) {
  std::string escape_sql ="\'";

  for (size_t i = 0; i < sql.size(); ++i) {
    if (sql[i] == '\'') {
        escape_sql += "\'\'";
    } else {
        escape_sql += sql[i];
    }
  }
  escape_sql += "\'";
  return escape_sql;
}

ruralis2_sql::ruralis2_sql() {
    DP("コンストラクタ");
    db = NULL;
}
ruralis2_sql::~ruralis2_sql() {
    DP("デストラクタ");
    if (db) {
        close();
    }
}

int ruralis2_sql::open(const char *file_name) {
    if (db == NULL) {
        int result;
        result = sqlite3_open(file_name, &db);
        if (result) {
            DP("DBオープンエラー");
            DP("file_name:" << file_name);
            return ERR_OPEN;
        }
    }
    return OK;
}

int ruralis2_sql::create_table(const string &table_name, const vector<string> &columns) {
    int result;
    string sql;

    sql += "CREATE TABLE  if not exists ";
    sql += table_name;
    sql += " (";
    for (std::size_t i = 0; i < columns.size(); i++) {
        if (i) {
            sql += ", ";
        }
        sql += columns[i];
    }
    sql += ")";
    result = exec(sql);
    return result;
}

int ruralis2_sql::update(
        const string &table_name, 
        const vector<string> &columns,
        const vector<string> &values,
        const string &where) {

    string sql;
    sqlite3_stmt *stmt;
    const char *sqlite_err_msg;
    if (db == NULL) {
        err_msg = "DB==NULL";
        DP(err_msg);
        return ERR_SQL;
    }
    sql += "update ";
    sql += table_name;
    sql += " set ";
    for (size_t i = 0; i < columns.size(); i++) {
        if (i) sql += ", ";
        sql += columns[i];
        sql += " = ?";
    }
    sql += " where ";
    sql += where;
    DP("SQL:" << sql);

    int r = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, &sqlite_err_msg);
    if (r != SQLITE_OK) {
        DP(" SQL:" << sql);
        DP("SQLエラー" << r);
        DP("SQLメッセージ" << sqlite_err_msg);
        
        err_msg = "SQLエラー ";
        err_msg += sqlite_err_msg;
        err_msg += "    ";
        err_msg += sql;
        sqlite3_free((void*)sqlite_err_msg);
        return ERR_SQL;
    }
    for (size_t i = 0; i < columns.size(); i++) {
        r = sqlite3_bind_text(stmt, i + 1, values[i].c_str(), -1, SQLITE_TRANSIENT);
        DP((i + 1) << ":" << values[i]);
        if (r != SQLITE_OK) {
            err_msg += "バインドエラー";
            err_msg += std::to_string(r);
            err_msg += " ";
            err_msg += sqlite3_errmsg(db);
            err_msg += " i=";
            err_msg += std::to_string(i);
            return ERR_SQL;
        }
    }
    r = sqlite3_step(stmt);
    if (r != SQLITE_DONE) {
        err_msg = "SQLエラー";
        err_msg += to_string(r);
        err_msg += " ";
        err_msg += sqlite3_errmsg(db);

        return ERR_SQL;
    }

    sqlite3_finalize(stmt);
    return OK;
}
int ruralis2_sql::insert(
        const string &table_name, 
        const vector<string> &columns,
        const vector<string> &values) {
    
    vector<vector<string> > vec;
    vec.push_back(values);
    return insert(table_name, columns, vec);
}

int ruralis2_sql::insert(
        const string &table_name, 
        const vector<string> &columns,
        const vector<vector<string> > &values) {
    string sql;
    sqlite3_stmt *stmt;
    const char *sqlite_err_msg;
    if (db == NULL) {
        err_msg = "DB==NULL";
        DP(err_msg);
        return ERR_SQL;
    }
    sql += "insert into ";
    sql += table_name;
    sql += " (";
    for (size_t i = 0; i < columns.size(); i++) {
        if (i) sql += ", ";
        sql += columns[i];
    }
    sql += ") values (";
    for (size_t i = 0; i < columns.size(); i++) {
        if (i) sql += ", ";
        sql += "?";
    }
    sql += ")";
    DP("SQL:" << sql);

    int r = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, &sqlite_err_msg);
    if (r != SQLITE_OK) {
        DP("SQL:" << sql);
        DP("SQLエラー" << r);
        DP("SQLメッセージ" << sqlite_err_msg);
        
        err_msg = "SQLエラー ";
        err_msg += sqlite_err_msg;
        err_msg += "    ";
        err_msg += sql;
        sqlite3_free((void*)sqlite_err_msg);
        return ERR_SQL;
    }
    for (size_t j = 0; j < values.size(); j++) {
        for (size_t i = 0; i < columns.size(); i++) {
            r = sqlite3_bind_text(stmt, i + 1, values[j][i].c_str(), -1, SQLITE_TRANSIENT);
            DP((i + 1) << ":" +values[j][i]);
            if (r != SQLITE_OK) {
                err_msg = "バインドエラー";
                err_msg += std::to_string(r);
                err_msg += " ";
                err_msg += sqlite3_errmsg(db);
                err_msg += " i=";
                err_msg += std::to_string(i);
                return ERR_SQL;
            }
        }
        r = sqlite3_step(stmt);
        if (r != SQLITE_DONE) {
            err_msg = "SQLエラー";
            err_msg += to_string(r);
            err_msg += " ";
            err_msg += sqlite3_errmsg(db);

            return ERR_SQL;
        }
    }

    sqlite3_finalize(stmt);
    return OK;
}

int ruralis2_sql::exec(
        const string &sql, 
        const vector<vector<string> > &values) {
    sqlite3_stmt *stmt;
    const char *sqlite_err_msg;
    string result;
    int r = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, &sqlite_err_msg);
    if (r != SQLITE_OK) {
        DP(" SQL:" << sql);;
        DP("SQLエラー" << result);
        DP("SQLメッセージ" << sqlite_err_msg);
        
        err_msg = "SQLエラー ";
        result += sqlite_err_msg;
        result += "    ";
        result += sql;
        sqlite3_free((void*)sqlite_err_msg);
        return ERR_SQL;
    }
    for (size_t j = 0; j < values.size(); j++) {
        for (size_t i = 0; i < values[j].size(); i++) {
            r = sqlite3_bind_text(stmt, i + 1, values[j][i].c_str(), -1, SQLITE_TRANSIENT);
            DP((i + 1) << ":" +values[j][i]);
            if (r != SQLITE_OK) {
                err_msg = "バインドエラー";
                err_msg += std::to_string(r);
                err_msg += " ";
                err_msg += sqlite3_errmsg(db);
                err_msg += " i=";
                err_msg += std::to_string(i);
                return ERR_SQL;
            }
        }
        r = sqlite3_step(stmt);
        if (r != SQLITE_DONE) {
            err_msg = "SQLエラー";
            err_msg += to_string(r);
            err_msg += " ";
            err_msg += sqlite3_errmsg(db);

            return ERR_SQL;
        }
    }

    sqlite3_finalize(stmt);
    return OK;
}

int ruralis2_sql::exec(const string &sql) {
    string result;
    DP("SQL:" << sql);
 
    char *sqlite_err_msg;
    int r = sqlite3_exec(db, sql.c_str(), NULL, NULL, &sqlite_err_msg);
    if(r != SQLITE_OK){
        err_msg = "SQLエラー";
        err_msg += to_string(r);
        err_msg += sqlite_err_msg;
        err_msg += "    ";
        err_msg += sql;
        sqlite3_free(sqlite_err_msg);
        return ERR_SQL;
    }
    return OK;
}

int ruralis2_sql::select(const string &sql
                    , vector<vector<string> > &result_data) {
    string result;
    sqlite3_stmt *stmt;
    const char *sqlite_err_msg;
    int r = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, &sqlite_err_msg);
    if(r){
        err_msg = "SQLエラー ";
        err_msg += sqlite_err_msg;
        err_msg += "\n";
        err_msg += sql;
        DP(err_msg);
        sqlite3_free((void*)sqlite_err_msg);
        return ERR_SQL;
    }
    while((r = sqlite3_step(stmt)) == SQLITE_ROW) {
        int cols = sqlite3_column_count(stmt);
        vector<string> row_data;
        for (int i = 0; i < cols; i++) {
            row_data.push_back((const char *)sqlite3_column_text(stmt, i)); 
        }
        result_data.push_back(row_data);
    }
    r = sqlite3_finalize(stmt);
    if(r){
        err_msg = "finalize error!";
        DP(err_msg);
        return ERR_SQL;
    }
    return OK;
}
void ruralis2_sql::close() {
    if (db) {
        sqlite3_close(db);
        db = NULL;
    }
}

