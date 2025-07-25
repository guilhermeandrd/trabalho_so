#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <semaphore.h>

// --- Configurações ---
#define TAMANHO_BUFFER 10
#define NUM_PRODUTORES 3
#define ITENS_POR_PRODUTOR 8
#define LOTE_CONSUMO 5 // O consumidor vai esperar por 5 itens

// --- Variáveis Globais para Sincronização e Buffer ---
int buffer[TAMANHO_BUFFER];
int in = 0;  // Posição de inserção (produtor)
int out = 0; // Posição de remoção (consumidor)

pthread_mutex_t mutex_buffer;      // Protege o acesso ao buffer
sem_t sem_empty;                   // Conta espaços vazios no buffer
sem_t sem_full;                    // Conta espaços preenchidos no buffer

// Variáveis para controlar o término do consumidor
int produtores_ativos;             // Contador de produtores em execução
pthread_mutex_t mutex_produtores_ativos; // Protege o contador

/**
 * Deixe as alterações na sua branch
 * até ver que estar funcionando de maneira geral.
 * Depois, passamos para a main, que é o código principal.
 * Lembre-se de sempre atualizar seu código com a main, para 
 * você não ter que fazer função que já existem
 */
    //TODO refatorar
void * producer ( void * args ) {
    int n = *(( int *) args ) ;
    pthread_t tid = pthread_self();

    // Sinaliza que mais um produtor está ativo
    pthread_mutex_lock(&mutex_produtores_ativos);
    produtores_ativos++;
    pthread_mutex_unlock(&mutex_produtores_ativos);
    
    printf("[Produtor %ld] Iniciado. Irá produzir %d itens.\n", tid, n);

    while (n-- > 0) {
        int item = (rand() % 1000) + 1; // Produz um item (valor aleatório)

        sem_wait(&sem_empty); // Espera um espaço livre no buffer
        pthread_mutex_lock(&mutex_buffer);

        // --- Seção Crítica ---
        // acessar buffer compartilhado ( produzir )
        buffer[in] = item;
        // imprimir TID / dados

        //TODO usar pthread_self
        printf("[Produtor %ld] Inseriu R$ %d na posição %d.\n", tid, item, in);
        in = (in + 1) % TAMANHO_BUFFER;
        // --- Fim da Seção Crítica ---

        pthread_mutex_unlock(&mutex_buffer);
        // sinalizar dados
        sem_post(&sem_full); // Avisa que há um novo item

        // esperar por um tempo aleatorio
        sleep((rand() % 2) + 1);
    }
    
    // imprimir que finalizou
    printf("[Produtor %ld] Finalizou.\n", tid);

    // Sinaliza que este produtor terminou
    pthread_mutex_lock(&mutex_produtores_ativos);
    produtores_ativos--;
    pthread_mutex_unlock(&mutex_produtores_ativos);

    pthread_exit(NULL);
}

void * consumer ( void * args ) {
    pthread_t tid = pthread_self();
    printf("[Consumidor %ld] Iniciado.\n", tid);

    while (1) {
        // Verifica a condição de parada: não há produtores ativos E o buffer está vazio.
        pthread_mutex_lock(&mutex_produtores_ativos);
        pthread_mutex_lock(&mutex_buffer);
        if (produtores_ativos == 0 && in == out) {
            pthread_mutex_unlock(&mutex_buffer);
            pthread_mutex_unlock(&mutex_produtores_ativos);
            break; // Sai do loop
        }
        pthread_mutex_unlock(&mutex_buffer);
        pthread_mutex_unlock(&mutex_produtores_ativos);


        // esperar 5 dados
        printf("[Consumidor %ld] Aguardando um lote de %d itens...\n", tid, LOTE_CONSUMO);
        for (int i = 0; i < LOTE_CONSUMO; i++) {
            sem_wait(&sem_full); // Bloqueia até que um item esteja disponível
        }

        pthread_mutex_lock(&mutex_buffer);
        // --- Seção Crítica ---
        printf("[Consumidor %ld] Processando lote:\n", tid);
        for (int i = 0; i < LOTE_CONSUMO; i++) {
            // acessar buffer compartilhado ( consumir )
            int item = buffer[out];
            // imprimir TID / dados
            printf("  -> Consumiu R$ %d da posição %d.\n", item, out);
            out = (out + 1) % TAMANHO_BUFFER;
            sem_post(&sem_empty); // Libera um espaço no buffer para cada item consumido
        }
        // --- Fim da Seção Crítica ---
        pthread_mutex_unlock(&mutex_buffer);

        sleep(1); // Pequena pausa após consumir um lote
    }
    
    // imprimir que finalizou
    printf("[Consumidor %ld] Finalizou.\n", tid);
    pthread_exit(NULL);
}

int main ( void ) {
    srand(time(NULL));
    pthread_t threads_produtoras[NUM_PRODUTORES];
    pthread_t thread_consumidora;

    // Inicializa mutex e semáforos
    pthread_mutex_init(&mutex_buffer, NULL);
    pthread_mutex_init(&mutex_produtores_ativos, NULL);
    sem_init(&sem_empty, 0, TAMANHO_BUFFER);
    sem_init(&sem_full, 0, 0);

    // Argumentos para as threads produtoras
    int itens_para_produzir[NUM_PRODUTORES];
    for(int i = 0; i < NUM_PRODUTORES; i++) {
        itens_para_produzir[i] = ITENS_POR_PRODUTOR;
    }

    // criar threads produtoras
    for (int i = 0; i < NUM_PRODUTORES; i++) {
        pthread_create(&threads_produtoras[i], NULL, producer, &itens_para_produzir[i]);
    }

    // criar thread consumidora
    pthread_create(&thread_consumidora, NULL, consumer, NULL);

    // esperar threads terminarem
    for (int i = 0; i < NUM_PRODUTORES; i++) {
        pthread_join(threads_produtoras[i], NULL);
    }
    printf("\n--- Todos os produtores terminaram. ---\n");

    // NOTA: Se os produtores não gerarem um número de itens múltiplo de LOTE_CONSUMO,
    // o consumidor pode ficar preso esperando um lote que nunca será completado.
    // Esta linha "acorda" o consumidor para que ele possa verificar a condição de término.
    sem_post(&sem_full);

    pthread_join(thread_consumidora, NULL);
    printf("--- Consumidor terminou. ---\n\n");
    
    // Libera os recursos
    pthread_mutex_destroy(&mutex_buffer);
    pthread_mutex_destroy(&mutex_produtores_ativos);
    sem_destroy(&sem_empty);
    sem_destroy(&sem_full);
    
    printf("Sistema finalizado.\n");
    return 0;
}