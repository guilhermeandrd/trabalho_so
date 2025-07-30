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

struct ret_partial
{
    long duracao_partial;

    double pi_partial;
};


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

    int first_therm = *(( int *) args ) ;

    // obter tempo de inicio
    clock_t inicio = clock();

    double sum = partialFormula ( first_therm ) ;

    //TODO acredito que falte essa parte do buffer
    // acessar buffer compartilhado

    // obter tempo de fim
    clock_t fim = clock();

    // mostrar TID e tempo empregado

    double d = (double) fim-inicio;
    d = d/CLOCKS_PER_SEC;

    printf("TID: %ld em tempo de %.4f segundos \n",pthread_self(), d);


    double *ret_pi_partial = malloc(sizeof(double));

    *ret_pi_partial = sum;

    pthread_exit(ret_pi_partial);
}

int main ( void ) {

    //vetor de threads
    pthread_t t_partials [NUM_THREADS];

    //vetor de retornos das threads
    void* ret_pi[NUM_THREADS];

    //vetor dos valores de retornos que serão convertidos
    double pi_partial[NUM_THREADS];

    struct ret_partial rets [NUM_THREADS];


    // obter tempo de inicio
    
    struct timeval start, end;
    gettimeofday(&start, NULL);

    for ( long long int i = 0; i < NUM_THREADS ; i ++) {
        // criar threads parciais
        long long int* it = (malloc(sizeof(long long int)));
        
        *it = i*PARTIAL_NUM_TERMS;

        pthread_create(&t_partials[i], NULL, &partialProcessing, it);
    }

    //cria variavel pi para armazenar soma de pi parciais
    double pi = 0.0;

    // esperar threads terminarem
    for(long long int i=0; i < NUM_THREADS; i++){
        pthread_join(t_partials[i], &ret_pi[i]);

        //"transforma" o valor que void estava apontando em double
        //armazena em pi_partial
        double pi_partial = *(double*) ret_pi[i];

        //soma em pi que é o que realmente importa
        //no sentido de ser o resultado final
        pi += pi_partial;
    }

    // obter tempo de fim
    gettimeofday(&end, NULL);

    //ajusta valor do pi
    pi *= 4;

    // mostrar resultado e tempo emprego
    printf("Resultado final: %.10f \n", pi);

    long duracao = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec);

    printf("Duracao %ld microsegundos", duracao);

    return 0;
}