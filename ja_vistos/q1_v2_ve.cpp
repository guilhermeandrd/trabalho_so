#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>     
#include <time.h>        
#include <semaphore.h>   

#define TAMANHO_BUFFER 5   
#define NUM_PRODUTORES 6
#define NUM_CONSUMIDORES 2

//TODO ACRESCENTAR MAIS UM CONSUMIDOR AQUI
int item[TAMANHO_BUFFER]; 
int idx = 0;               

pthread_mutex_t mutex_buffer;   
sem_t empty;                    
sem_t full;                     

pthread_mutex_t mutex_ativos;   
int num_produtores_ativos = 0; 

int total_vendas_consumidas = 0; 

typedef struct {
    int id_caixa;
    int total_vendas_a_produzir;
} produtor_args_t;

int aleatorio(int min, int max) {
    return (rand() % (max - min + 1)) + min;
}

void* produtora(void* args) { 
    produtor_args_t* p_args = (produtor_args_t*)args;
    int id_caixa = p_args->id_caixa;
    int vendas_restantes = p_args->total_vendas_a_produzir;

    pthread_mutex_lock(&mutex_ativos);
    num_produtores_ativos++; 
    pthread_mutex_unlock(&mutex_ativos);

    printf("(P) TID: %d iniciado. Irá produzir %d itens.\n", id_caixa, vendas_restantes);

    while(vendas_restantes-- > 0) { 
        sem_wait(&empty); 
        pthread_mutex_lock(&mutex_buffer); 

        int valor_venda = aleatorio(1, 1000); 
        item[idx % TAMANHO_BUFFER] = valor_venda; 
        printf("(P) TID: %d | VALOR: R$ %d | Pos: %d | Restantes: %d\n", 
               id_caixa, valor_venda, idx % TAMANHO_BUFFER, vendas_restantes); 
        idx++; 

        pthread_mutex_unlock(&mutex_buffer); 
        sem_post(&full); 

        sleep(aleatorio(1, 3)); 
    }

    pthread_mutex_lock(&mutex_ativos);
    num_produtores_ativos--; 
    pthread_mutex_unlock(&mutex_ativos);

    printf("(P) TID: %d finalizou\n", id_caixa);

    pthread_exit(NULL);
}

void* consumidora(void* args) { 
    int id_consumidor = 1; 
    int iteracao_consumo = 0;

    printf("(C) TID: %d iniciado.\n", id_consumidor);

    while (1) {
        pthread_mutex_lock(&mutex_buffer);
        if (num_produtores_ativos == 0 && (idx - total_vendas_consumidas) == 0) { 
            pthread_mutex_unlock(&mutex_buffer);
            sem_post(&full); 
            break; 
        }
        pthread_mutex_unlock(&mutex_buffer);

        for (int k = 0; k < TAMANHO_BUFFER; k++) {
            sem_wait(&full); 
        }

        pthread_mutex_lock(&mutex_buffer);

        int itens_buffer = idx - total_vendas_consumidas; 

        if (num_produtores_ativos == 0 && itens_buffer < TAMANHO_BUFFER) { 
            for (int k = 0; k < TAMANHO_BUFFER; k++) {
                sem_post(&full); 
            }
            pthread_mutex_unlock(&mutex_buffer);
            break; 
        }

        double soma_lote = 0.0; 
        
        for (int k = 0; k < TAMANHO_BUFFER; k++) {
            soma_lote += item[total_vendas_consumidas % TAMANHO_BUFFER]; 
            total_vendas_consumidas++; 
        }
        iteracao_consumo++;

        double media_lote = soma_lote / TAMANHO_BUFFER; 
        printf("(C) TID: %d | MEDIA: R$ %.2f | ITERACAO: %d\n",
               id_consumidor, media_lote, iteracao_consumo); 

        pthread_mutex_unlock(&mutex_buffer);

        for (int k = 0; k < TAMANHO_BUFFER; k++) {
            sem_post(&empty); 
        }

        usleep(100000); 
    }

    printf("(C) TID: %d finalizou\n", id_consumidor);
    pthread_exit(NULL);
}

int main(void) {
    srand(time(NULL)); 

    pthread_mutex_init(&mutex_buffer, NULL); 
    pthread_mutex_init(&mutex_ativos, NULL);
    sem_init(&empty, 0, TAMANHO_BUFFER); 
    sem_init(&full, 0, 0);               

    pthread_t produtor_threads[NUM_PRODUTORES];
    pthread_t consumidor_thread[NUM_CONSUMIDORES]; //TODO criar vetor de consumiores

    for (int i = 0; i < NUM_PRODUTORES; ++i) {
        produtor_args_t* args = (produtor_args_t*)malloc(sizeof(produtor_args_t));
        args->id_caixa = i + 1;
        args->total_vendas_a_produzir = aleatorio(20, 30); 
        pthread_create(&produtor_threads[i], NULL, produtora, (void*)args);
    }

    for (int i = 0; i < NUM_CONSUMIDORES; ++i) {
        produtor_args_t* args = (consumidor_ar*)malloc(sizeof(produtor_args_t));
        args->id_caixa = i + 1;
        args->total_vendas_a_produzir = aleatorio(20, 30); 
        pthread_create(&produtor_threads[i], NULL, produtora, (void*)args);
    }



    for (int i = 0; i < NUM_PRODUTORES; ++i) {
        pthread_join(produtor_threads[i], NULL);
    }
    printf("\n--- Todas as threads produtoras finalizaram. ---\n");

    for (int i = 0; i < TAMANHO_BUFFER + NUM_PRODUTORES + 5; ++i) { 
        sem_post(&full); 
    }

    pthread_join(consumidor_thread, NULL);
    printf("\n--- A thread consumidora finalizou. ---\n");

    pthread_mutex_destroy(&mutex_buffer); 
    pthread_mutex_destroy(&mutex_ativos);
    sem_destroy(&empty); 
    sem_destroy(&full); 

    printf("\n--- Sistema Gerenciador de Caixas Finalizado com Sucesso! ---\n");

    return 0;
}