#include <iostream>
#include <chrono>
#include <pthread.h>
#include <unistd.h>


using namespace std;


class Data_for_needs{
    int* k = nullptr;

public:
    Data_for_needs(int d){
        k = new int(d);
    }
    ~Data_for_needs(){
        delete k;
    }
    int get_val(){
        return *k;
    }
    void set_val(int d){
        *k = d;
    }

};

class Monitor_for_prog
{
    Data_for_needs* dat = nullptr; 
    pthread_mutex_t mtx;
    pthread_cond_t cv;
    bool flag = false;
    bool stop = false;
public:
    Monitor_for_prog() {
        pthread_mutex_init(&mtx, nullptr);
        pthread_cond_init(&cv, nullptr);
    }
    ~Monitor_for_prog() {
        pthread_mutex_destroy(&mtx);
        pthread_cond_destroy(&cv);
    }
    void provide(int n){
        pthread_mutex_lock(&mtx);
        cout << "P_lock"<<endl;

        sleep(1);
        while (flag) {
            pthread_mutex_unlock(&mtx);
            sleep(1); 
            pthread_mutex_lock(&mtx);
        }
        delete dat;
        dat = new Data_for_needs(n);
        flag = true;
        cout << "P_new_event"<<endl;

        cout << "P_signal"<<endl;
        pthread_cond_signal(&cv);

        cout << "P_unlock"<<endl<<endl;
        pthread_mutex_unlock(&mtx);

    }

    int consume() {
        pthread_mutex_lock(&mtx);
        cout << "c_lock"<<endl;
        while (!flag && !stop) {
            cout << "c wait, flag=0"<<endl;
            pthread_cond_wait(&cv, &mtx);
        }
        sleep(1);
        if (stop)
        {
            pthread_mutex_unlock(&mtx);
            return 0;
        }
        
        
        int result = dat->get_val();
        cout << "get val: " << result<<endl;
        delete dat; 
        dat = nullptr;
        flag = false;
        cout << "c_unlock"<<endl<<endl;
        pthread_mutex_unlock(&mtx);
        return result;
    }

    void finish() {
        pthread_mutex_lock(&mtx);
        stop = true;
        pthread_cond_broadcast(&cv); 
        pthread_mutex_unlock(&mtx);
    }

};  
static Monitor_for_prog monitor;

//поставщик
void* producer(void* arg){
    for (size_t i = 1; i <= COUNT; i++)
    {
        monitor.provide(i*10);
    }
    monitor.finish();
    return nullptr;
}

//потребитель
void* consumer(void* arg){
    for (size_t i = 0; i < COUNT; i++)
    {
        int result = monitor.consume();
        if (result)
        {
            cout << "-" << result << "-"<<endl<<endl;
        }
    }
    return nullptr;
}


int main() {
    //потоки
    pthread_t  producer_thread, consumer_thread;
    pthread_create(&producer_thread, nullptr, producer, nullptr);
    pthread_create(&consumer_thread, nullptr, consumer, nullptr);

    //ждем завершения
    pthread_join(producer_thread,nullptr);
    pthread_join(consumer_thread,nullptr);

    cout << "\nWork is done\n\n" << endl;
    return 0;
}