#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <semaphore.h>

#define TAMANHO_BUFFER 10
#define NUM_PRODUTORES 3
#define NUM_CONSUMIDORES 1
#define SENTINEL -1 // Valor para sinalizar o fim da produção

// --- Variáveis Globais Compartilhadas ---
int buffer[TAMANHO_BUFFER];
int in = 0;  // Índice de inserção no buffer
int out = 0; // Índice de remoção do buffer

pthread_mutex_t mutex; // Apenas um mutex é necessário para proteger o buffer
sem_t empty;           // Semáforo para contar posições vazias
sem_t full;            // Semáforo para contar posições cheias

// Função para gerar número aleatório //TODO acho que essa funcao eh desnecessaria

//TODO ajeitar impressão dos dados
int aleatorio(int min, int max) {
    return (rand() % (max - min + 1)) + min;
}

// --- Produtor ---
// Produz itens e os coloca no buffer.
void* produtora(void* arg) {
    long id = (long)arg; // Recebe o ID diretamente, sem struct
    int total_a_produzir = aleatorio(5, 10);
    printf("[Produtor %ld] Iniciado. Irá produzir %d itens.\n", id, total_a_produzir);

    for (int i = 0; i < total_a_produzir; i++) {
        int item = aleatorio(1, 100); // Gera um item (valor da venda)

        sem_wait(&empty); // Espera por um espaço livre no buffer
        pthread_mutex_lock(&mutex);

        // Seção Crítica: Adiciona o item ao buffer
        buffer[in] = item;
        printf("[Produtor %ld] Inseriu R$ %d na posição %d.\n", id, item, in);
        in = (in + 1) % TAMANHO_BUFFER;

        pthread_mutex_unlock(&mutex);
        sem_post(&full); // Sinaliza que um novo item está disponível

        sleep(aleatorio(1, 2));
    }

    // Após terminar, envia o valor SENTINELA para sinalizar ao consumidor
    sem_wait(&empty);
    pthread_mutex_lock(&mutex);
    buffer[in] = SENTINEL;
    in = (in + 1) % TAMANHO_BUFFER;
    pthread_mutex_unlock(&mutex);
    sem_post(&full);

    printf("[Produtor %ld] Finalizou.\n", id);
    pthread_exit(NULL);
}

// --- Consumidor ---
// Retira itens do buffer e os processa.
void* consumidora(void* arg) {
    int item;
    int produtores_finalizados = 0;
    long total_consumido = 0;
    int contador_itens = 0;

    printf("[Consumidor] Iniciado.\n");

    while (produtores_finalizados < NUM_PRODUTORES) {
        sem_wait(&full); // Espera por um item no buffer
        pthread_mutex_lock(&mutex);

        // Seção Crítica: Remove o item do buffer
        item = buffer[out];
        out = (out + 1) % TAMANHO_BUFFER;

        pthread_mutex_unlock(&mutex);
        sem_post(&empty); // Sinaliza que um espaço foi liberado

        // Processa o item
        if (item == SENTINEL) {
            produtores_finalizados++;
            printf("[Consumidor] Recebeu sinal de finalização de um produtor (%d/%d).\n", produtores_finalizados, NUM_PRODUTORES);
        } else {
            printf("[Consumidor] Consumiu R$ %d.\n", item);
            total_consumido += item;
            contador_itens++;
        }
    }
    
    double media = (contador_itens > 0) ? (double)total_consumido / contador_itens : 0;
    printf("\n[Consumidor] Finalizou. Total de itens consumidos: %d. Média geral: R$ %.2f\n", contador_itens, media);
    pthread_exit(NULL);
}

// --- Função Principal ---
int main(void) {
    srand(time(NULL));

    // Inicialização
    pthread_mutex_init(&mutex, NULL);
    sem_init(&empty, 0, TAMANHO_BUFFER);
    sem_init(&full, 0, 0);

    pthread_t threads_produtoras[NUM_PRODUTORES];
    pthread_t thread_consumidora;

    // Criação das Threads
    for (long i = 0; i < NUM_PRODUTORES; i++) {
        pthread_create(&threads_produtoras[i], NULL, produtora, (void*)(i + 1));
    }
    pthread_create(&thread_consumidora, NULL, consumidora, NULL);

    // Aguarda o término de todas as threads
    for (int i = 0; i < NUM_PRODUTORES; i++) {
        pthread_join(threads_produtoras[i], NULL);
    }
    printf("\n--- Todas as threads produtoras finalizaram. ---\n");

    pthread_join(thread_consumidora, NULL);
    printf("--- A thread consumidora finalizou. ---\n");

    // Liberação de recursos
    pthread_mutex_destroy(&mutex);
    sem_destroy(&empty);
    sem_destroy(&full);

    printf("\n--- Sistema Finalizado com Sucesso! ---\n");

    return 0;
}