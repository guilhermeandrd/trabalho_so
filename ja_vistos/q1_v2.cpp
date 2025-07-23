#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>     
#include <time.h>        
#include <semaphore.h>   

#define TAMANHO_BUFFER 8   
#define NUM_PRODUTORES 6
#define NUM_CONSUMIDORES 2 

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

typedef struct {
    int id_consumidor;
} consumidor_args_t;

int aleatorio(int min, int max) {
    return (rand() % (max - min + 1)) + min;
}

void* produtora(void* args) { 
    produtor_args_t* p_args = (produtor_args_t*)args;
    int id_caixa = p_args->id_caixa;
    int vendas_restantes = p_args->total_vendas_a_produzir;
    int iteracao = 0;

    pthread_mutex_lock(&mutex_ativos);
    num_produtores_ativos++; 
    pthread_mutex_unlock(&mutex_ativos);

    while(vendas_restantes-- > 0) { 
        sem_wait(&empty); 
        pthread_mutex_lock(&mutex_buffer); 

        int valor_venda = aleatorio(1, 1000); 
        item[idx % TAMANHO_BUFFER] = valor_venda; 
        printf("(P) TID: %d | VENDA: %d | ITERACAO: %d\n", 
               id_caixa, valor_venda, ++iteracao); 
        idx++; 

        pthread_mutex_unlock(&mutex_buffer); 
        sem_post(&full); 

        usleep(aleatorio(100000, 300000)); 
    }

    pthread_mutex_lock(&mutex_ativos);
    num_produtores_ativos--; 
    pthread_mutex_unlock(&mutex_ativos);

    free(args);
    pthread_exit(NULL);
}
void* consumidora(void* args) { 
    consumidor_args_t* c_args = (consumidor_args_t*)args;
    int id_consumidor = c_args->id_consumidor;
    int iteracao_consumo = 0;

    printf("(C) TID: %d iniciado.\n", id_consumidor);

    while (1) {
        pthread_mutex_lock(&mutex_buffer);
        int itens_disponiveis = idx - total_vendas_consumidas;
        int produtores_ativos = num_produtores_ativos;
        pthread_mutex_unlock(&mutex_buffer);

        if (produtores_ativos == 0 && itens_disponiveis == 0) {
            break;
        }

        int itens_processados = 0;
        double soma_lote = 0.0;

        pthread_mutex_lock(&mutex_buffer);
        
        itens_disponiveis = idx - total_vendas_consumidas;
        
        int itens_a_processar;
        if (itens_disponiveis >= TAMANHO_BUFFER) {
        itens_a_processar = TAMANHO_BUFFER;  
        } else {
        itens_a_processar = itens_disponiveis;  
    }
        
        if (itens_a_processar == 0 && produtores_ativos == 0) {
            pthread_mutex_unlock(&mutex_buffer);
            break;
        }

        if (itens_a_processar > 0) {
            for (int i = 0; i < itens_a_processar; i++) {
                soma_lote += item[total_vendas_consumidas % TAMANHO_BUFFER];
                total_vendas_consumidas++;
                sem_wait(&full);
            }
            iteracao_consumo++;

            double media_lote = soma_lote / itens_a_processar;
            printf("(C) TID: %d | MÉDIA: R$ %.2f | ITERAÇÃO: %d\n", id_consumidor, media_lote, iteracao_consumo);
        }
        
        pthread_mutex_unlock(&mutex_buffer);

        for (int i = 0; i < itens_a_processar; i++) {
            sem_post(&empty);
        }

        usleep(aleatorio(150000, 350000));
    }

    printf("(C) TID: %d finalizou\n", id_consumidor);
    free(args);
    pthread_exit(NULL);
}

int main(void) {
    srand(time(NULL)); 

    pthread_mutex_init(&mutex_buffer, NULL); 
    pthread_mutex_init(&mutex_ativos, NULL);
    sem_init(&empty, 0, TAMANHO_BUFFER); 
    sem_init(&full, 0, 0);               

    pthread_t produtor_threads[NUM_PRODUTORES];
    pthread_t consumidor_threads[NUM_CONSUMIDORES];

    for (int i = 0; i < NUM_PRODUTORES; ++i) {
        produtor_args_t* args = (produtor_args_t*)malloc(sizeof(produtor_args_t));
        args->id_caixa = i + 1;
        args->total_vendas_a_produzir = aleatorio(20, 30); 
        pthread_create(&produtor_threads[i], NULL, produtora, (void*)args);
    }

    for (int i = 0; i < NUM_CONSUMIDORES; ++i) {
        consumidor_args_t* args = (consumidor_args_t*)malloc(sizeof(consumidor_args_t));
        args->id_consumidor = i + 1;
        pthread_create(&consumidor_threads[i], NULL, consumidora, (void*)args);
    }

    for (int i = 0; i < NUM_PRODUTORES; ++i) {
        pthread_join(produtor_threads[i], NULL);
    }
    printf("\n--- Todas as threads produtoras finalizaram. ---\n");

    for (int i = 0; i < NUM_CONSUMIDORES; ++i) {
        sem_post(&full);
    }

    for (int i = 0; i < NUM_CONSUMIDORES; ++i) {
        pthread_join(consumidor_threads[i], NULL);
    }
    printf("\n--- Todas as threads consumidoras finalizaram. ---\n");

    pthread_mutex_destroy(&mutex_buffer); 
    pthread_mutex_destroy(&mutex_ativos);
    sem_destroy(&empty); 
    sem_destroy(&full); 

    printf("\n--- Sistema Finalizado ---\n");
    

    return 0;
}
//principai desafio foi adaptar  a logica para que funcionasse para dois consumidores visto que o algoritmo de funcionamento teve que ser repensado para que não
//  ocorressem deadlocks causados por erros no prenchimento e esvaziamento do buffer