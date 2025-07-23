#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <semaphore.h>


/**
 * Deixe as alterações na sua branch
 * até ver que estar funcionando de maneira geral.
 * Depois, passamos para a main, que é o código principal.
 * Lembre-se de sempre atualizar seu código com a main, para 
 * você não ter que fazer função que já existem
 */

//ESTRUTURA BÁSICA RETIRADA DO ARQUIVO DO PROJETO
void * producer ( void * args ) {
    int n = *(( int *) args ) ;

    while (n-- > 0) {
        // acessar buffer compartilhado ( produzir )
        // sinalizar dados
        // imprimir TID / dados
        // esperar por um tempo aleatorio
    }
    // imprimir que finalizou
}

void * consumer ( void * args ) {
    while ( /* tem produtoras ainda ? */) {
        // esperar 5 dados
        // acessar buffer compartilhado ( consumir )
        // imprimir TID / dados
    }
    // imprimir que finalizou
}

int main ( void ) {
    // criar threads produtoras
    // criar thread consumidora
    // esperar threads terminarem
}
