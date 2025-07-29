#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>     
#include <time.h>        
#include <semaphore.h>   

//configuracao inicial que vai ser usada
#define TAMANHO_BUFFER 5   
//TODO mudar pra 6
#define NUM_PRODUTORES 6
//TODO alterar lógica para lidar com 2 consumidores
#define NUM_CONSUMIDORES 1 

//buffer com os valore a serem usados
int item[TAMANHO_BUFFER]; 
int idx = 0;               

//mutex e semaforos
pthread_mutex_t mutex_buffer;

//
sem_t empty;          

//usada para eficiente esperar
pthread_cond_t nova_venda;


pthread_mutex_t mutex_ativos;   

//flags para verificacao e gerenciamento
int num_produtores_ativos = 0; 

int total_vendas_consumidas = 0; 

// estrutura que contem o id da caixa na qual esta ocorrendo o processo
// e o total de vendas que serão produzidas
typedef struct {
    int id_caixa;
    int total_vendas_a_produzir;
} produtor_args_t;

//TODO não sei se isso é útil realmente
typedef struct{
    int id_caixa;
    int total_itens_a_consumir;
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


        //verificamos se buffer encheu
        if ((idx - total_vendas_consumidas) == TAMANHO_BUFFER) {
            printf("(P) TID: %d, Buffer ENCHEU. Sinalizando consumidor...\n", id_caixa);

            //usamos pthread_cond_signal para sinalizar que buffer encheu
            //importante para consumidor
            pthread_cond_signal(&nova_venda); 
        }

        // A liberação do mutex vem depois da checagem e do possível sinal
        pthread_mutex_unlock(&mutex_buffer);

        sleep(aleatorio(1, 3)); 
    }

    pthread_mutex_lock(&mutex_ativos);
    num_produtores_ativos--; 
    pthread_mutex_unlock(&mutex_ativos);

    printf("(P) TID: %d finalizou\n", id_caixa);

    pthread_exit(NULL);
}

//TODO deixar parecida com a produtora
void* consumidora(void* args) { 

    int id_consumidor = 1; 
    int iteracao_consumo = 0;

    printf("(C) TID: %d iniciado.\n", id_consumidor);

    /**
     * estratégia : 
     * buffer cheio, mas 
     */

    while(1){

            // Dentro do laço while(1) da sua consumidora

        pthread_mutex_lock(&mutex_buffer); // Tranca a porta

        // Enquanto o buffer NÃO estiver cheio...
        while ((idx - total_vendas_consumidas) < TAMANHO_BUFFER) {

            // Adicionar uma condição de término aqui é importante. E se os
            // produtores terminarem e deixarem o buffer com apenas 3 itens?
            // O consumidor ficaria esperando para sempre.
            if (num_produtores_ativos == 0 && (idx - total_vendas_consumidas) > 0) {
                // Se não há mais produtores e há itens no buffer, saia do laço
                // para processar o lote final, mesmo que incompleto.
                printf("(C) Produtores inativos, processando lote final de %d itens.\n", (idx - total_vendas_consumidas));
                break; 
            } else if (num_produtores_ativos == 0 && (idx - total_vendas_consumidas) == 0) {
                // Se não há mais produtores e o buffer está vazio, termine.
                printf("(C) Produtores inativos e buffer vazio. Finalizando.\n");

                printf("(C) TID: %d finalizou\n", id_consumidor);

                pthread_mutex_unlock(&mutex_buffer);
                pthread_exit(NULL);
            }

            printf("(C) Buffer não está cheio (%d/%d). Esperando sinal...\n", (idx - total_vendas_consumidas), TAMANHO_BUFFER);
            // Dorme e espera o sinal de "buffer cheio".
            pthread_cond_wait(&nova_venda, &mutex_buffer);
        }
        printf("(C) Buffer cheio! Processando lote.\n");
        double soma_lote = 0.0;
        int itens_a_processar = idx - total_vendas_consumidas; // Pode ser 5 ou menos no lote final
    
        // O seu laço 'for' para consumir o lote estava correto, mas vamos adaptá-lo
        // para consumir o número exato de itens.
        for (int k = 0; k < itens_a_processar; k++) {
            soma_lote += item[total_vendas_consumidas % TAMANHO_BUFFER];
            total_vendas_consumidas++;
        }
        iteracao_consumo++;
    
        double media_lote = soma_lote / itens_a_processar; 
        printf("(C) TID: %d | MEDIA DO LOTE: R$ %.2f | ITERACAO: %d\n",
            id_consumidor, media_lote, iteracao_consumo); 
    
        pthread_mutex_unlock(&mutex_buffer); // Destranca a porta
    
        // Avisa que os espaços foram liberados
        for (int k = 0; k < itens_a_processar; k++) {
            sem_post(&empty); // Sinaliza 'itens_a_processar' vezes
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
     * o mutex dos produtores ativos
     * o semaforo da 
     */
    pthread_mutex_init(&mutex_buffer, NULL); 
    pthread_mutex_init(&mutex_ativos, NULL);

    //inicia pthread_cond
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
        produtor_args_t* args = (produtor_args_t*)malloc(sizeof(produtor_args_t));
        args->id_caixa = i + 1;
        
        pthread_create(&consumidor_thread[i], NULL, consumidora, (void*)args);
    }


    for (int i = 0; i < NUM_PRODUTORES; ++i) {
        pthread_join(produtor_threads[i], NULL);
    }

    printf("\n--- Todas as threads produtoras finalizaram. ---\n");

    pthread_cond_signal(&nova_venda);

    for (int i = 0; i < NUM_CONSUMIDORES; ++i) {
        pthread_join(consumidor_thread[i], NULL);
    }

    printf("\n--- Todas as thread consumidoras finalizaram. ---\n");

    //liberação de memória
    pthread_mutex_destroy(&mutex_buffer); 
    pthread_mutex_destroy(&mutex_ativos);
    pthread_cond_destroy(&nova_venda);
    sem_destroy(&empty); 

    printf("\n--- Sistema Gerenciador de Caixas Finalizado com Sucesso! ---\n");

    return 0;
}