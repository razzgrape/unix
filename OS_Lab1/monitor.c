#include <pthread.h>
#include <stdio.h>
#include <unistd.h> 

pthread_mutex_t mutex_var;   
pthread_cond_t cond_var;    
int event_ready = 0;         
int shared_data = 0;         

void* producer_thread(void* arg) {
    int i = 1;
    while (1) { 
        sleep(1); 

        pthread_mutex_lock(&mutex_var); 

        shared_data = i; 
        event_ready = 1;
        printf("-> **[Поставщик]** Событие #%d. (Данные: %d)\n", i, shared_data);

        pthread_cond_signal(&cond_var); 
        
        pthread_mutex_unlock(&mutex_var);
        i++; 
    }
    return NULL;
}

void* consumer_thread(void* arg) {
    int i = 1;
    while (1) { 
        pthread_mutex_lock(&mutex_var); 

        while (event_ready == 0) {
            pthread_cond_wait(&cond_var, &mutex_var);
        }
        
        int received_data = shared_data;
        printf("<- **[Потребитель]** Событие #%d получено. (Прочитанные данные: %d)\n", i, received_data);
        
        event_ready = 0;

        pthread_mutex_unlock(&mutex_var);
        i++; 
    }
    return NULL;
}

int main() {
    pthread_t producer_tid, consumer_tid;
    
    pthread_mutex_init(&mutex_var, NULL);
    pthread_cond_init(&cond_var, NULL);

    pthread_create(&producer_tid, NULL, producer_thread, NULL);
    pthread_create(&consumer_tid, NULL, consumer_thread, NULL);

    pthread_join(producer_tid, NULL);
    pthread_join(consumer_tid, NULL);

    pthread_mutex_destroy(&mutex_var);
    pthread_cond_destroy(&cond_var);

    return 0;
}