/*(a) Primeiramente, faça um programa (q3 1.c) que implemente a série sequencial-
mente. Considere que a série possui 2.000.000.000 termos (ao invés de infinito).
Ao final do programa, o valor calculado e o tempo empregado deve ser mostrado.*/

#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/time.h>

#define NUM_TERMS 2000000000

void *formulaleibniz(void *args){

    //alocamos pi na heap (dinamicamente)
    //assim não ficamos dependendo da stack
    double *pi = malloc(sizeof(double));
    
    *pi = 0.0;

    for (long long int i = 0; i < NUM_TERMS; i++) {
        double termo = 1.0 / (2 * i + 1);
        if (i % 2 == 0) {
            *pi += termo;
        } else {
            *pi -= termo;
        }
    }

    //funcao exit semelhante a return
    pthread_exit(pi);
}

int main() {
    
    pthread_t t;
    void *ret_pi;

    
    struct timeval inicio, fim;
    gettimeofday(&inicio, NULL);
    
    pthread_create(&t,NULL, &formulaleibniz, NULL);
    
    //convertemos ponteiro para um tipo double

    //espera a thread t retorna o "retorno" em ret_pi
    pthread_join(t, &ret_pi);

    double pi = *(double*)ret_pi;
    
    //liberamos memória que foi alocada
    free(ret_pi);

    pi *= 4;

    gettimeofday(&fim, NULL);

    long duracao = (fim.tv_sec - inicio.tv_sec) *  1000000 + (fim.tv_usec - inicio.tv_usec);
    
    printf("Aproximação de PI com %lld termos: %.9f\n", (long long) NUM_TERMS, pi);
    printf("Em tempo de %.4ld  microsegundos\n", duracao);
    return 0;
}