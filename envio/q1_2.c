#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>     
#include <time.h>        
#include <semaphore.h>   

//configuracao inicial que vai ser usada
#define TAMANHO_BUFFER 5

#define NUM_PRODUTORES 6
#define NUM_CONSUMIDORES 2

//buffer com os valore a serem usados
int item[TAMANHO_BUFFER]; 
int idx = 0;               

//mutex e semaforos
pthread_mutex_t mutex_buffer;


sem_t empty;          

//usada para eficiente esperar
pthread_cond_t nova_venda;


//flags para verificacao e gerenciamento
int num_produtores_ativos = 0; 

int total_vendas_consumidas = 0; 

// estrutura que contem o id da caixa na qual esta ocorrendo o processo
// e o total de vendas que serão produzidas
typedef struct {
    int id_caixa;
    int total_vendas_a_produzir;
} produtor_args_t;

typedef struct{
    int id_caixa;
    int iteracao_consumo;
} consumidor_args_t;

//funcao que gera um numero aleatorio dado um numero maximo e minimo
int aleatorio(int min, int max) {
    return (rand() % (max - min + 1)) + min;
}

void* produtora(void* args) { 
    produtor_args_t* p_args = (produtor_args_t*)args;
    int id_caixa = p_args->id_caixa;
    int vendas_restantes = p_args->total_vendas_a_produzir;

    //protecao do mutex
    pthread_mutex_lock(&mutex_buffer);
    num_produtores_ativos++; 
    pthread_mutex_unlock(&mutex_buffer);

    printf("(P) TID: %d iniciado. Irá produzir %d itens.\n", id_caixa, vendas_restantes);

    while(vendas_restantes-- > 0) { 
        sem_wait(&empty); 
        pthread_mutex_lock(&mutex_buffer); 

        int valor_venda = aleatorio(1, 1000); 
        item[idx % TAMANHO_BUFFER] = valor_venda; 
        printf("(P) TID: %d | VALOR: R$ %d | Pos: %d | Restantes: %d\n", 
               id_caixa, valor_venda, idx % TAMANHO_BUFFER, vendas_restantes); 
        idx++;


        //verificamos se buffer encheu
        if ((idx - total_vendas_consumidas) == TAMANHO_BUFFER) {
            printf("(P) TID: %d, Buffer ENCHEU. Sinalizando consumidor...\n", id_caixa);

            //usamos pthread_cond_signal para sinalizar que buffer encheu
            //importante para consumidor
            pthread_cond_signal(&nova_venda); 
        }

        pthread_mutex_unlock(&mutex_buffer);

        sleep(aleatorio(1, 3)); 
    }

    pthread_mutex_lock(&mutex_buffer);
    num_produtores_ativos--; 
    pthread_mutex_unlock(&mutex_buffer);

    
    printf("(P) TID: %d finalizou\n", id_caixa);

    pthread_exit(NULL);
}

void* consumidora(void* args) { 

    consumidor_args_t* c_args = (consumidor_args_t*)args;
    int id_consumidor = c_args->id_caixa; 

    int iteracao_consumo = c_args->iteracao_consumo;

    printf("(C) TID: %d iniciado.\n", id_consumidor);

    while(1){


        pthread_mutex_lock(&mutex_buffer); 

        while ((idx - total_vendas_consumidas) < TAMANHO_BUFFER) {

           
            if (num_produtores_ativos == 0 && (idx - total_vendas_consumidas) > 0) {
             
                printf("(C) Produtores inativos, processando lote final de %d itens.\n", (idx - total_vendas_consumidas));
                break; 
            } else if (num_produtores_ativos == 0 && (idx - total_vendas_consumidas) == 0) {

                printf("(C) Produtores inativos e buffer vazio. Finalizando.\n");

                //ultima mensagem de saída
                printf("(C) TID: %d | TOTAL DE ITERACAO: %d\n",
            id_consumidor, iteracao_consumo);
                printf("(C) TID: %d finalizou\n", id_consumidor);

                pthread_mutex_unlock(&mutex_buffer);
                pthread_exit(NULL);
            }

            printf("(C) Buffer não está cheio (%d/%d). Esperando sinal...\n", (idx - total_vendas_consumidas), TAMANHO_BUFFER);

            pthread_cond_wait(&nova_venda, &mutex_buffer);
        }
        printf("(C) Buffer cheio! Processando lote.\n");
        double soma_lote = 0.0;
        int itens_a_processar = idx - total_vendas_consumidas; 
    
        for (int k = 0; k < itens_a_processar; k++) {
            soma_lote += item[total_vendas_consumidas % TAMANHO_BUFFER];
            total_vendas_consumidas++;
        }
        iteracao_consumo++;
    
        double media_lote = soma_lote / itens_a_processar; 
        printf("(C) TID: %d | MEDIA DO LOTE: R$ %.2f | ITERACAO: %d\n",
            id_consumidor, media_lote, iteracao_consumo); 
    
        pthread_mutex_unlock(&mutex_buffer); 
    
        for (int k = 0; k < itens_a_processar; k++) {
            sem_post(&empty);
        }
    }

}

//TODO lembrar de liberar memória com free (as da struct que eu aloquei)
int main(void) {

    //usada para gerar numeros sempre aleatorios 
    //o que significa que toda execução terá dados diferentes
    //ou ao menos a chance disso ocorrer
    srand(time(NULL)); 

    /**
     * inicializamos o mutex do buffer
     * o semaforo da 
     */
    pthread_mutex_init(&mutex_buffer, NULL); 
    pthread_cond_init(&nova_venda, NULL);
    sem_init(&empty, 0, TAMANHO_BUFFER); 

    pthread_t produtor_threads[NUM_PRODUTORES];
    pthread_t consumidor_thread[NUM_CONSUMIDORES];

    for (int i = 0; i < NUM_PRODUTORES; ++i) {
        produtor_args_t* args = (produtor_args_t*)malloc(sizeof(produtor_args_t));
        args->id_caixa = i + 1;
        
        //produz um numero de itens aleatorio (entre 20 e 30)
        args->total_vendas_a_produzir = aleatorio(20, 30); 
        pthread_create(&produtor_threads[i], NULL, produtora, (void*)args);
    }

    for (int i = 0; i < NUM_CONSUMIDORES; ++i) {
        consumidor_args_t* args = (consumidor_args_t*)malloc(sizeof(consumidor_args_t));
        args->id_caixa = i + 1;
        args->iteracao_consumo = 0;
        
        pthread_create(&consumidor_thread[i], NULL, consumidora, (void*)args);
    }


    for (int i = 0; i < NUM_PRODUTORES; ++i) {
        pthread_join(produtor_threads[i], NULL);
    }

    printf("\n--- Todas as threads produtoras finalizaram. ---\n");

    
    pthread_cond_broadcast(&nova_venda);
    
    for (int i = 0; i < NUM_CONSUMIDORES; ++i) {
        pthread_join(consumidor_thread[i], NULL);
    }

    printf("\n--- Todas as thread consumidoras finalizaram. ---\n");

    //liberação de memória
    pthread_mutex_destroy(&mutex_buffer); 
    pthread_cond_destroy(&nova_venda);
    sem_destroy(&empty); 

    printf("\n--- Sistema Gerenciador de Caixas Finalizado com Sucesso! ---\n");

    return 0;
}