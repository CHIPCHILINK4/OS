#include <iostream>
#include <unistd.h>
#include <cerrno>
#include <locale.h> 
#include <sys/select.h>
#include <signal.h>
#include <sys/socket.h>

using namespace std;

class signal_manager{
    static volatile sig_atomic_t was_sighup;
    sigset_t block_mask;
    sigset_t original_mask;

    static void handler(int) {
        was_sighup = 1;
    }

    signal_manager() {
        was_sighup = 0;
        sigemptyset(&block_mask);
        sigaddset(&block_mask, SIGHUP);
    }

    void Block() {
        struct sigaction action;
        memset(&action, 0, sizeof(action));
        action.sa_handler = handler;
        sigemptyset(&action.sa_mask);
        action.sa_flags = 0;
        sigaction(SIGHUP, &action, NULL);
        sigprocmask(SIG_BLOCK, &block_mask, &original_mask);
    }

    void restore() {
        sigprocmask(SIG_SETMASK, &original_mask, NULL);
    }
    bool check_Reset() {
        if (was_sighup) {
            was_sighup = 0;
            return true;
        }
        return false;
    }
    sigset_t* getMask() const {
        return &block_mask; 
    }

};
class TCP_server{
    signal_manager manager;
    int server_fd = -1;
    int client_fd = -1;


public: 
    TCPServer(int port) {
        manager.Block();

        server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd == -1) { 
            perror("socket"); 
            exit(EXIT_FAILURE); 
        }

        int opt = 1;
        setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        struct sockaddr_in address;
        memset(&address, 0, sizeof(address));
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = htonl(INADDR_ANY);
        address.sin_port = htons(port);

        if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) == -1) {
            perror("bind"); 
            close(server_fd); 
            exit(EXIT_FAILURE);
        }
        if (listen(server_fd, 16) == -1) {
            perror("listen"); 
            close(server_fd); 
            exit(EXIT_FAILURE);
        }
    }
    ~TCPServer() {
        cout<< "Сворачиваем сервер"
        if (client_fd != -1) 
            close(client_fd);
        if (server_fd != -1) 
            close(server_fd);
        manager.restore();
    }
    void run() {// kill -HUP pid
        cout << "Запуск сервера PID " << getpid() << endl;

        // pselect();
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