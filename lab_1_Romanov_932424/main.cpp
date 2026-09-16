#include <iostream>
#include <chrono>
#include <pthread.h>
#include <unistd.h>


using namespace std;



class Data_for_needs{
    int* k = 0;

public:
    Data_for_needs(int d){
        k = new int(d);
    }
    ~Data_for_needs(){
        delete k;
    }

};

struct Monitor_for_prog
{
    Data_for_needs* dat; 
    pthread_mutex_t mtx;
    pthread_cond_t cv;
    bool flag = false;
};

//поставщик
void* producer(void* arg){

}

//потребитель
void* consumer(void* arg){

}



Monitor_for_prog monitor;

int main() {
    //первые базовые переменные
    pthread_mutex_init(&monitor.mtx, nullptr);
    pthread_cond_init(&monitor.cv, nullptr);

    monitor.flag = false;
    monitor.dat = nullptr;
    
    //потоки
    pthread_t  producer_thread, consumer_thread;
    pthread_create(&consumer_thread, nullptr, consumer, nullptr);
    pthread_create(&producer_thread, nullptr, producer, nullptr);

    //ждем завершения
    pthread_join(producer_thread,nullptr);
    pthread_join(consumer_thread,nullptr);

    // убиваем их
    pthread_mutex_destroy(&monitor.mtx);
    pthread_cond_destroy(&monitor.cv);

    cout << "\nСпасибо за работу\n\n" << endl;
    return 0;
}