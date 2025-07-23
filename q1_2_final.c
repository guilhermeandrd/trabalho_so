#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <semaphore.h>

// --- Configurações ---
#define TAMANHO_BUFFER 8
#define NUM_PRODUTORES 6
#define NUM_CONSUMIDORES 2
#define ITENS_POR_PRODUTOR 10
#define SENTINEL -1 // Valor especial para sinalizar o fim da produção

// --- Variáveis Globais Simplificadas ---
int buffer[TAMANHO_BUFFER];
int in = 0;  // Índice onde o produtor insere
int out = 0; // Índice onde o consumidor remove

pthread_mutex_t mutex; // Apenas um mutex é necessário
sem_t empty;           // Semáforo para posições vazias
sem_t full;            // Semáforo para posições cheias

int aleatorio(int min, int max) {
    return (rand() % (max - min + 1)) + min;
}

// --- Produtor ---
// A lógica do produtor permanece similar, mas usando o índice 'in'.
void* produtora(void* args) {
    long id = (long)args;

    for (int i = 0; i < ITENS_POR_PRODUTOR; i++) {
        int valor_venda = aleatorio(1, 1000);

        sem_wait(&empty); // Espera por um espaço vazio
        pthread_mutex_lock(&mutex);

        buffer[in] = valor_venda;
        printf("[Produtor %ld] Inseriu R$ %d na posição %d\n", id, valor_venda, in);
        in = (in + 1) % TAMANHO_BUFFER;

        pthread_mutex_unlock(&mutex);
        sem_post(&full); // Sinaliza que um novo item está disponível

        usleep(aleatorio(50000, 100000));
    }

    printf("[Produtor %ld] Finalizou a produção.\n", id);
    pthread_exit(NULL);
}

// --- Consumidor ---
// Lógica drasticamente simplificada para ser mais robusta e permitir concorrência.
void* consumidora(void* args) {
    long id = (long)args;
    printf("[Consumidor %ld] Iniciado.\n", id);

    while (1) {
        sem_wait(&full); // Espera por um item (qualquer item)

        // Trava o mutex apenas o tempo suficiente para remover um item do buffer
        pthread_mutex_lock(&mutex);
        int item = buffer[out];
        out = (out + 1) % TAMANHO_BUFFER;
        pthread_mutex_unlock(&mutex);

        // Se o item for um sentinela, o trabalho acabou.
        if (item == SENTINEL) {
            // Importante: Devolve o sentinela ao buffer para que outros consumidores
            // também possam ler e terminar.
            sem_post(&full);
            break; // Sai do loop e encerra a thread.
        }

        // Se for um item normal, libera um espaço 'empty' e o processa.
        sem_post(&empty);
        printf("[Consumidor %ld] Consumiu R$ %d.\n", id, item);

        usleep(aleatorio(150000, 350000)); //TODO verificar depois
    }

    printf("[Consumidor %ld] Finalizou.\n", id);
    pthread_exit(NULL);
}

int main(void) {
    srand(time(NULL));

    // Inicializa mutex e semáforos
    pthread_mutex_init(&mutex, NULL);
    sem_init(&empty, 0, TAMANHO_BUFFER);
    sem_init(&full, 0, 0);

    pthread_t produtor_threads[NUM_PRODUTORES];
    pthread_t consumidor_threads[NUM_CONSUMIDORES];

    // Cria as threads produtoras
    for (long i = 0; i < NUM_PRODUTORES; ++i) {
        pthread_create(&produtor_threads[i], NULL, produtora, (void*)(i + 1));
    }

    // Cria as threads consumidoras
    for (long i = 0; i < NUM_CONSUMIDORES; ++i) {
        pthread_create(&consumidor_threads[i], NULL, consumidora, (void*)(i + 1));
    }

    // Espera todas as produtoras terminarem seu trabalho principal
    for (int i = 0; i < NUM_PRODUTORES; ++i) {
        pthread_join(produtor_threads[i], NULL);
    }
    printf("\n--- Todas as threads produtoras finalizaram sua produção. ---\n");

    // Após todos os produtores terminarem, inserimos os sentinelas no buffer.
    // Um sentinela para cada consumidor, para garantir que todos recebam o sinal.
    for (int i = 0; i < NUM_CONSUMIDORES; i++) {
        sem_wait(&empty);
        pthread_mutex_lock(&mutex);
        buffer[in] = SENTINEL;
        in = (in + 1) % TAMANHO_BUFFER;
        pthread_mutex_unlock(&mutex);
        sem_post(&full);
    }

    // Espera as consumidoras terminarem
    for (int i = 0; i < NUM_CONSUMIDORES; ++i) {
        pthread_join(consumidor_threads[i], NULL);
    }
    printf("\n--- Todas as threads consumidoras finalizaram. ---\n");

    // Libera os recursos
    pthread_mutex_destroy(&mutex);
    sem_destroy(&empty);
    sem_destroy(&full);

    printf("\n--- Sistema Finalizado ---\n");
    return 0;
}