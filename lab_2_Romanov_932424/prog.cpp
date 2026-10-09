#include <iostream>
#include <cstring>
#include <cstdlib>
#include <clocale>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <signal.h>
#include <cerrno>

using namespace std;

class signal_manager{
    static volatile sig_atomic_t was_sighup;
    sigset_t block_mask;
    sigset_t original_mask;
    sigset_t wait_mask;
    bool is_blocked = false;

    static void handler(int) {
        was_sighup = 1;
    }

public:
    signal_manager() {
        was_sighup = 0;
        if (sigemptyset(&block_mask) == -1 || sigaddset(&block_mask, SIGHUP) == -1) {
            cerr << "Не удалось подготовить маску сигналов"<< endl;
            exit(EXIT_FAILURE);
        }
    }

    void block() {
        if (sigprocmask(SIG_BLOCK, &block_mask, &original_mask) == -1) {
            cerr << "sigprocmask" << endl;
            exit(EXIT_FAILURE);
        }
        is_blocked = true;

        struct sigaction action;
        memset(&action, 0, sizeof(action));
        action.sa_handler = handler;
        sigemptyset(&action.sa_mask);
        action.sa_flags = 0;
        sigaction(SIGHUP, &action, NULL);
        wait_mask = original_mask;
        sigprocmask(SIG_BLOCK, &block_mask, &original_mask);
    }

    void restore() {
        if (is_blocked && sigprocmask(SIG_SETMASK, &original_mask, NULL) == -1) {
            cerr << "sigprocmask" << endl;
        }
        is_blocked = false;
    }

    bool check_Reset() {
        if (was_sighup) {
            was_sighup = 0;
            return true;
        }
        return false;
    }

    const sigset_t* get_mask() const {
        return &wait_mask;
    }
};

volatile sig_atomic_t signal_manager::was_sighup = 0;

class TCP_server {
private:
    signal_manager manager;
    int server_fd = -1;
    int client_fd = -1;

public:
    TCP_server(int port) {
        manager.block();

        server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd == -1) {
            cerr << "socket" << endl;
            exit(EXIT_FAILURE);
        }


        int opt = 1;
        if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
            cerr << "setsockopt" << endl;
            close(server_fd);
            exit(EXIT_FAILURE);
        }

        struct sockaddr_in address;
        memset(&address, 0, sizeof(address));
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = htonl(INADDR_ANY);
        address.sin_port = htons(port);

        if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) == -1) {
            cerr << "bind" << endl;
            close(server_fd);
            exit(EXIT_FAILURE);
        }
        if (listen(server_fd, 16) == -1) {
            cerr << "listen" << endl;
            close(server_fd);
            exit(EXIT_FAILURE);
        }
    }

    ~TCP_server() {
        cout << "Сворачиваем сервер" << endl;
        if (client_fd != -1)
            close(client_fd);
        if (server_fd != -1) 
            close(server_fd);
        manager.restore();
    }

    void run() {// kill -HUP pid
        cout << "Запуск сервера. PID " << getpid() << endl;
        cout << "Для отправки сигнала используйте: kill -HUP " << getpid() << endl;

        while (true) {
            fd_set readfds;
            FD_ZERO(&readfds);
            FD_SET(server_fd, &readfds);
            int max_fd = server_fd;

            if (client_fd != -1) {
                FD_SET(client_fd, &readfds);
                if (client_fd > max_fd) 
                    max_fd = client_fd;
            }

            int ready = pselect(max_fd + 1, &readfds, NULL, NULL, NULL, manager.get_mask());

            if (ready == -1 && errno != EINTR) {
                cerr << "pselect" << endl;
                break;
            }

            if (manager.check_Reset()) {
                cout << "[signal_manager] Получен сигнал SIGHUP!" << endl;
            }

            if (ready > 0) {
                if (FD_ISSET(server_fd, &readfds)) {
                    int new_fd = accept(server_fd, NULL, NULL);
                    if (new_fd != -1) {
                        if (new_fd >= FD_SETSIZE) {
                            cerr << "[server] Новый дескриптор превышает FD_SETSIZE, соединение закрыто." << endl;
                            close(new_fd);
                        } else if (client_fd != -1) {
                            cout << "[server] Клиент уже подключен, закрываем новое соединение." << endl;
                            close(new_fd);
                        } else {
                            cout << "[server] Новое подключение принято." << endl;
                            client_fd = new_fd;
                        }
                    } else if (errno != EINTR) {
                        cerr << "accept";
                    }
                }

                if (client_fd != -1 && FD_ISSET(client_fd, &readfds)) {
                    char buffer[1024];
                    ssize_t received = recv(client_fd, buffer, sizeof(buffer), 0);

                    if (received > 0) {
                        cout << "[Клиент] Получено байт: " << received << endl;
                    } else if (received == 0) {
                        cout << "[Клиент] Соединение закрыто клиентом." << endl;
                        close(client_fd);
                        client_fd = -1;
                    } else if (errno != EINTR) {
                        cerr << "recv" << endl;
                        close(client_fd);
                        client_fd = -1;
                    }
                }
            }
        }
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