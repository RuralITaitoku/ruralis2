#include"ruralis2.h"


using namespace std;

ruralis2_http srv;



int main(int argc, char *argv[]) {
    srv.port_no = 1581;
    srv.top_dir = "/var/www/html";
    int result = srv.start();
    DP("result=" << result);
    if (result) {
        cout << srv.err_msg << endl;
        DP(srv.err_msg);
    }

}