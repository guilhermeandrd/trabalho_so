#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>     
#include <time.h>        
#include <semaphore.h>   

//configuracao inicial que vai ser usada
#define TAMANHO_BUFFER 5   
#define NUM_PRODUTORES 3
#define NUM_CONSUMIDORES 1 

//buffer com os valore a serem usados
int item[TAMANHO_BUFFER]; 
int idx = 0;               

//mutex e semaforos
pthread_mutex_t mutex_buffer;

//serve para impedir o produtor de adcionar itens
//ao buffer cheio
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

        //verifica se ainda há vagos, se o contador interno > 0
        //se for, decrementa e deixa passar
        //se não, espera surgir uma vaga enquanto produtora é suspensa
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

        //delay de 1 a 5 segundos
        sleep(aleatorio(1, 5)); 
    }

    pthread_mutex_lock(&mutex_buffer);
    num_produtores_ativos--; 
    pthread_mutex_unlock(&mutex_buffer);

    printf("(P) TID: %d finalizou\n", id_caixa);

    pthread_exit(NULL);//return void
}

void* consumidora(void* args) { 
    int id_consumidor = 1; 
    int iteracao_consumo = 0;

    printf("(C) TID: %d iniciado.\n", id_consumidor);

    while(1){

        //protege buffer
        pthread_mutex_lock(&mutex_buffer);

        //roda enquanto o buffer não encher
        while ((idx - total_vendas_consumidas) < TAMANHO_BUFFER) {

            // condição para quando não tiver mais produtores ativos, mas buffer estiver cheio
            if (num_produtores_ativos == 0 && (idx - total_vendas_consumidas) > 0) {
              
                printf("(C) Produtores inativos, processando lote final de %d itens.\n", (idx - total_vendas_consumidas));
                break; 
            } else if (num_produtores_ativos == 0 && (idx - total_vendas_consumidas) == 0) {

                //condição de término
                printf("(C) Produtores inativos e buffer vazio. Finalizando.\n");

                printf("(C) TID: %d finalizou\n", id_consumidor);

                pthread_mutex_unlock(&mutex_buffer);
                pthread_exit(NULL);
            }

            printf("(C) Buffer não está cheio (%d/%d). Esperando sinal...\n", (idx - total_vendas_consumidas), TAMANHO_BUFFER);
            
            //espera o buffer encher através de sinal da produtora
            pthread_cond_wait(&nova_venda, &mutex_buffer);
        }

        printf("(C) Buffer cheio! Processando lote.\n");

        double soma_lote = 0.0;

        int itens_a_processar = idx - total_vendas_consumidas; //calcula exatamente através do idx
    
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
            //avisa que o item foi processado, logo uma vaga foi liberada
            //ou seja, incrementa o contador interno
            sem_post(&empty);
        }

    }

}

int main(void) {

    //usada para gerar numeros sempre aleatorios 
    //o que significa que toda execução terá dados diferentes
    //ou ao menos a chance disso ocorrer
    srand(time(NULL)); 

    /**
     * inicializamos o mutex do buffer
     * o semaforo empty 
     * inicia pthread_cond
     */
    pthread_mutex_init(&mutex_buffer, NULL); 
    sem_init(&empty, 0, TAMANHO_BUFFER); 
    pthread_cond_init(&nova_venda, NULL);


    pthread_t produtor_threads[NUM_PRODUTORES];
    pthread_t consumidor_thread;

    //cria as threads de produtores
    for (int i = 0; i < NUM_PRODUTORES; ++i) {
        produtor_args_t* args = (produtor_args_t*)malloc(sizeof(produtor_args_t));
        args->id_caixa = i + 1;
        
        //produz um numero de itens aleatorio (entre 20 e 30)
        args->total_vendas_a_produzir = aleatorio(20, 30); 
        pthread_create(&produtor_threads[i], NULL, produtora, (void*)args);
    }

    //cria a thread consumidora
    pthread_create(&consumidor_thread, NULL, consumidora, NULL);

    for (int i = 0; i < NUM_PRODUTORES; ++i) {
        pthread_join(produtor_threads[i], NULL);
    }

    printf("\n--- Todas as threads produtoras finalizaram. ---\n");

    //acorda nova venda para esperar consumidor terminar
    pthread_cond_signal(&nova_venda);

    pthread_join(consumidor_thread, NULL);
    printf("\n--- A thread consumidora finalizou. ---\n");

    //liberação de memória
    pthread_mutex_destroy(&mutex_buffer); 
    pthread_cond_destroy(&nova_venda);
    sem_destroy(&empty); 

    printf("\n--- Sistema Gerenciador de Caixas Finalizado com Sucesso! ---\n");

    return 0;
}