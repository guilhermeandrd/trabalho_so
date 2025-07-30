/*b) Após isso, faça um outro programa (q3 2.c) que implemente a série de forma
paralela, usando várias threads. A thread principal deve criar 16 threads, que vão
calcular uma parte da série, isto é, somar n/16 termos, e, ao final, somar esse
resultado em uma variável que deve conter todos os termos. A thread principal
deve esperar que as outras terminem de forma calcular o valor final da série
(multiplicar o somatório por 4), mostrar o tempo empregado por cada thread, o
tempo empregado pelo processo e a soma dos tempos das threads.*/

#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/time.h>

#define NUM_TERMS 2000000000
#define NUM_THREADS 16
#define PARTIAL_NUM_TERMS ((NUM_TERMS)/(NUM_THREADS))

//variável global para pi
double pi = 0.0;

pthread_mutex_t mutex_pi;

long duracao_total = 0;


//essa é a função que realmente calcula a formula
double partialFormula ( int first_therm ) {

    // A funcao ira processar PARTIAL_NUM_TERMS termos
    const long long int num_terms = first_therm + PARTIAL_NUM_TERMS ;

    // Aproxima o pi , de first_therm ate num_terms - 1
    double pi_approximation = 0;

    double signal = 1.0;
    
    for ( long long int k = first_therm ; k < num_terms ; k ++) {
        pi_approximation += signal /(2* k + 1) ;
        signal *= -1.0;
    }

    return pi_approximation ;

}

void * partialProcessing ( void * args ) {
    
    struct timeval inicio, fim;

    int first_therm = *(( int *) args ) ;

    // obter tempo de inicio
    gettimeofday(&inicio, NULL);
    
    double sum = partialFormula ( first_therm ) ;

    
    // obter tempo de fim
    gettimeofday(&fim, NULL);
    
    // mostrar TID e tempo empregado
    long d = (fim.tv_sec - inicio.tv_sec) *  1000000 + (fim.tv_usec - inicio.tv_usec);

    //protege acesso do buffer
    pthread_mutex_lock(&mutex_pi);
    pi += sum;
    duracao_total += d;
    pthread_mutex_unlock(&mutex_pi);

    
    printf("TID: %ld em tempo de %ld microsegundos \n",pthread_self(), d);

    pthread_exit(NULL);
}

int main ( void ) {

    struct timeval inicio, end;

    gettimeofday(&inicio, NULL);

    //vetor de threads
    pthread_t t_partials [NUM_THREADS];

    //vetor de retornos das threads
    void* rets[NUM_THREADS];

    //vetor dos valores de retornos que serão convertidos
    double pi_partial[NUM_THREADS];

    pthread_mutex_init(&mutex_pi, NULL);
    

    for ( long long int i = 0; i < NUM_THREADS ; i ++) {
        // criar threads parciais
        long long int* it = (malloc(sizeof(long long int)));
        
        *it = i*PARTIAL_NUM_TERMS;

        pthread_create(&t_partials[i], NULL, &partialProcessing, it);
    }

    // esperar threads terminarem
    for(long long int i=0; i < NUM_THREADS; i++){
        pthread_join(t_partials[i], NULL);
    }


    //ajusta valor do pi
    pi *= 4;

    gettimeofday(&end, NULL);

    long d_total = (end.tv_sec - inicio.tv_sec) * 1000000 + (end.tv_usec - inicio.tv_usec);

    // mostrar resultado e tempo emprego
    printf("Resultado final: %.9f \n", pi);


    printf("Duracao total das threads foi %ld microsegundos \n", duracao_total);

    printf("Duração total de todo o processo sem contar soma de threads foi %ld microsegundos \n", d_total);

    pthread_mutex_destroy(&mutex_pi);

    return 0;
}