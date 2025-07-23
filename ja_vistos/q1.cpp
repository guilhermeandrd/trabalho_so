#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <random> // Para simular valores de transação aleatórios
#include <chrono> // Para simular tempo de trabalho (sleep)
#include <numeric> // para calcular a média
#include <atomic> // para a contagem de produtores ativos
#include <iomanip>
using namespace std;
const int tamanho_buffer= 5; //capacidade do buffer
const int num_produtores = 3; // nùmero de threads de caixa

vector<int> buffer_vendas;//buffer que aramazena valores de vendas
mutex mutex_buffer;//protege o acesso ao buffer

condition_variable cv_produtor;// pordutores vão esperar aqui se o buffer tiver cheio
condition_variable cv_consumidor; // consumidores vão esperar aqui se o buffer tiver vazio

atomic<int> num_produtores_ativos(0);//contar produtores ativos

random_device dispositivo_aleatorio;
mt19937 gerador(dispositivo_aleatorio());
uniform_int_distribution<> distrib_valor_venda(1, 1000);   // Valores de venda entre 1 e 1000 reais
uniform_int_distribution<> distrib_num_vendas(20, 30);     // Qtd. de vendas por produtor (entre 20 e 30)
uniform_int_distribution<> distrib_delay_segundos(1, 5); //  // Delay aleatório (1 a 5 segundos)

void* produtor(void* args) {
    // Desempacota o parâmetro: um array de int com {id_caixa, total_vendas_a_produzir}
    int* params = static_cast<int*>(args);
    int id_caixa = params[0];
    int total_vendas_a_produzir = params[1];
    delete[] params;

    num_produtores_ativos++; 
    
    // Obtém o ID da thread
    hash<thread::id> hasher;
    size_t tid_hash = hasher(this_thread::get_id());

    for (int i = 0; i < total_vendas_a_produzir; ++i) {
        int valor_venda = distrib_valor_venda(gerador); 
        unique_lock<mutex> lock(mutex_buffer);

        // Espera se o buffer estiver cheio
        cv_produtor.wait(lock, [] { 
            return buffer_vendas.size() < tamanho_buffer; 
        });

        buffer_vendas.push_back(valor_venda);
        
       cout << "(P) TID: " << tid_hash << " | VALOR: R$ " << valor_venda 
                  << " | ITERACAO: " << (i + 1) << endl;

        // Notifica o consumidor se o buffer ficou cheio ou se é a última venda do produtor
        // e o consumidor pode precisar processar o que sobrou.
        if (buffer_vendas.size() == tamanho_buffer || (i == total_vendas_a_produzir - 1)) {
            cv_consumidor.notify_one(); 
        }

        lock.unlock(); 

        // Espera um tempo aleatório antes de produzir a próxima venda
        this_thread::sleep_for(chrono::seconds(distrib_delay_segundos(gerador)));
    }

    num_produtores_ativos--;
    
    
    cout << "(P) TID: " << tid_hash << " finalizou" << endl;
    
    cv_consumidor.notify_one(); 

    return NULL;
}

void* consumidor(void* args) {
    // Obtém o ID real da thread (hash para facilitar a visualização)
    hash<thread::id> hasher;
    size_t tid_hash = hasher(this_thread::get_id());
    int iteracao_consumo = 0;

    while (true) {
        unique_lock<mutex> lock(mutex_buffer);

        // Espera até que o buffer esteja cheio OU todos os produtores tenham terminado E o buffer esteja vazio.
        cv_consumidor.wait(lock, [&] {
            return buffer_vendas.size() == tamanho_buffer || 
                   (buffer_vendas.empty() && num_produtores_ativos == 0);
        });

        if (buffer_vendas.empty() && num_produtores_ativos == 0) {
            lock.unlock();
            break; 
        }
        
        // Se o buffer não estiver cheio, mas ainda há produtores ativos, não consumimos.
        // A menos que seja a última rodada e os produtores já tenham finalizado.
        if (buffer_vendas.size() < tamanho_buffer && num_produtores_ativos > 0) {
            lock.unlock();
            continue; // Continua esperando por mais itens ou por produtores finalizarem
        }

        iteracao_consumo++;
        
        // Consome os itens do buffer
        long long soma_vendas = 0;
        int itens_processados_nesta_rodada = buffer_vendas.size(); 

        for (int i = 0; i < itens_processados_nesta_rodada; ++i) {
            soma_vendas += buffer_vendas[i];
        }
        buffer_vendas.clear();

        double media_aritmetica = static_cast<double>(soma_vendas) / itens_processados_nesta_rodada;
        
       cout << fixed <<setprecision(2);
        cout << "(C) TID: " << tid_hash << " | MEDIA: R$ " << media_aritmetica 
                  << " | ITERACAO: " << iteracao_consumo << endl;

       
        cv_produtor.notify_all(); 

        lock.unlock(); 

        // pausa para simular processamento do gerente
        this_thread::sleep_for(chrono::milliseconds(100)); 
    }
    
    cout << "(C) TID: " << tid_hash << " finalizou" << endl;
    return NULL; 
}

int main(void) {
    cout << "--- Sistema Gerenciador de Caixas Iniciado ---" << endl;

    vector<thread> threads_produtoras;

    // Cria as 3 threads produtoras (caixas)
    for (int i = 0; i < num_produtores; ++i) {
        // Aloca dinamicamente um array para passar múltiplos parâmetros
        int* params = new int[2];
        params[0] = i + 1; // ID do caixa
        params[1] = distrib_num_vendas(gerador); // Quantidade de vendas para este produtor

        threads_produtoras.emplace_back(produtor, static_cast<void*>(params));
    }

    thread thread_consumidora(consumidor, static_cast<void*>(NULL));

    for (thread& t : threads_produtoras) {
        if (t.joinable()) {
            t.join();
        }
    }
    cout << "\n--- Todas as threads produtoras finalizaram. ---" << endl;

    if (thread_consumidora.joinable()) {
        thread_consumidora.join();
    }
    cout << "\n--- A thread consumidora finalizou. ---" << endl;

    cout << "\n--- Sistema Gerenciador de Caixas Finalizado com Sucesso! ---" << endl;

    return 0;
}