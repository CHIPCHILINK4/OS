#include <iostream>
#include <unistd.h>
#include <cerrno>
#include <locale.h> 
#include <sys/select.h>
#include <signal.h>

using namespace std;

class signal_manager{
    static volatile sig_atomic_t was_sighup;
    sigset_t block_mask;
    sigset_t original_mask;

    static void handler(int) {
        was_sighup = 1;
    }

};
class TCP_server{
    int server_fd = -1;
    int client_fd = -1;


public: 
    void run() {
        cout << "Запуск сервера PID " << getpid() << endl;
    }
};

int main(int argc, char* argv[]) {
    setlocale(LC_ALL, "Russian");
    if (argc != 2) {
        cerr << "ERR: передайте 1 аргументом порт" << endl;
        return 1;
    }

    TCP_server server(atoi(argv[1]));
    server.run();


    cout << "Итог, работа завершена" << endl;
    return 0;
}