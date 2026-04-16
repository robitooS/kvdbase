#include <iostream>
#include <chrono>
#include <vector>
#include <numeric>
#include <algorithm>
#include <random>
#include <iomanip>
#include <string>
#include <cstdio> // For std::remove
#include "kvdbase.hpp"

using namespace std::chrono;

void print_report(const std::string& task, double duration, int ops) {
    double ops_per_sec = (duration > 0) ? static_cast<double>(ops) / duration : 0;
    std::cout << std::left << std::setw(35) << task 
              << ": " << std::fixed << std::setprecision(3) << duration << "s "
              << "(" << std::setprecision(0) << ops_per_sec << " ops/s)" << std::endl;
}

int main() {
    const int N_KEYS = 1000000; // Um milhão de chaves para o teste
    const std::string DB_NAME = "performance_test.db";
    
    // Garante um ambiente de teste limpo
    std::remove(DB_NAME.c_str()); 

    KVdbase db(DB_NAME);
    
    std::cout << "===== PERFORMANCE TEST SUITE (" << N_KEYS << " KEYS) =====" << std::endl;

    // --- Fase 1: INSERÇÃO SEQUENCIAL MASSIVA ---
    // Testa a escrita inicial e splits de nós
    auto start_insert = high_resolution_clock::now();
    for (int i = 0; i < N_KEYS; ++i) {
        db.put(i, i * 3); // Valor simples
        if (i > 0 && i % 200000 == 0) { // Progresso a cada 200k chaves
             std::cout << "... " << i << "/" << N_KEYS << " chaves inseridas." << std::endl;
        }
    }
    auto end_insert = high_resolution_clock::now();
    double duration_insert = duration<double>(end_insert - start_insert).count();
    print_report("Insercao Sequencial", duration_insert, N_KEYS);

    // --- Fase 2: BUSCA ALEATÓRIA (Teste de Leitura) ---
    // Cria um vetor embaralhado de chaves para buscar de forma aleatória
    std::vector<int> keys_to_search(N_KEYS);
    std::iota(keys_to_search.begin(), keys_to_search.end(), 0);
    std::shuffle(keys_to_search.begin(), keys_to_search.end(), std::mt19937{std::random_device{}()});

    auto start_search = high_resolution_clock::now();
    int found_count = 0;
    for (int i = 0; i < N_KEYS; ++i) { // Busca TODAS as chaves inseridas
        if (db.get(keys_to_search[i])) {
            found_count++;
        }
    }
    auto end_search = high_resolution_clock::now();
    double duration_search = duration<double>(end_search - start_search).count();
    print_report("Busca Aleatoria (Total)", duration_search, N_KEYS);
    if (found_count != N_KEYS) {
        std::cerr << "ERRO DE INTEGRIDADE: Todas as chaves deveriam ter sido encontradas apos insercao!" << std::endl;
    }

    // --- Fase 3: ATUALIZAÇÃO (Overwrite) ---
    // Modifica um subconjunto das chaves
    auto start_update = high_resolution_clock::now();
    for (int i = 0; i < N_KEYS / 2; ++i) { // Atualiza as primeiras 500k chaves
        db.put(i, i + 1000); // Novo valor
    }
    auto end_update = high_resolution_clock::now();
    double duration_update = duration<double>(end_update - start_update).count();
    print_report("Atualizacao (Metade)", duration_update, N_KEYS / 2);

    // --- Fase 4: REMOÇÃO ALEATÓRIA (Carga Pesada de IO) ---
    // Remove um subconjunto de chaves de forma aleatória
    auto start_remove = high_resolution_clock::now();
    int removed_count = 0;
    // Embaralha novamente para garantir aleatoriedade na remoção
    std::shuffle(keys_to_search.begin(), keys_to_search.end(), std::mt19937{std::random_device{}()});
    for (int i = 0; i < N_KEYS / 2; ++i) { // Remove 500k chaves
        if (db.remove(keys_to_search[i])) {
            removed_count++;
        }
        if (i > 0 && i % 100000 == 0) {
             std::cout << "... " << i << "/" << N_KEYS/2 << " chaves removidas." << std::endl;
        }
    }
    auto end_remove = high_resolution_clock::now();
    double duration_remove = duration<double>(end_remove - start_remove).count();
    print_report("Remocao Aleatoria (Metade)", duration_remove, N_KEYS / 2);

    // --- Fase 5: VERIFICAÇÃO FINAL DE INTEGRIDADE ---
    int final_found = 0;
    int final_missing_but_should_exist = 0;
    for (int i = 0; i < N_KEYS; ++i) {
        auto val = db.get(i);
        // Chaves de 0 a N_KEYS/2 - 1 foram removidas
        if (i < N_KEYS / 2) { 
            if (val) {
                std::cerr << "ERRO CRITICO: Chave " << i << " encontrada apos remocao!" << std::endl;
            }
        } else { // Chaves de N_KEYS/2 a N_KEYS-1 deveriam existir
            if (val) {
                // Verifica se o valor está correto (para os atualizados e os originais)
                if (i < N_KEYS / 2) { // Chaves atualizadas (que não foram removidas)
                     if (*val == i + 999) final_found++;
                     else final_missing_but_should_exist++;
                } else { // Chaves originais (que não foram removidas)
                     if (*val == i * 2) final_found++;
                     else final_missing_but_should_exist++;
                }
            } else {
                final_missing_but_should_exist++;
            }
        }
    }
    
    std::cout << " --- FINAL REPORT ---" << std::endl;
    std::cout << "Total de chaves restantes (inteiras): " << final_found << "/" << N_KEYS / 2 << std::endl;
    if (final_missing_but_should_exist == 0 && found_count == N_KEYS) {
        std::cout << "INTEGRIDADE: 100% OK." << std::endl;
    } else {
        std::cerr << "ALERTA DE INTEGRIDADE: Falhas encontradas!" << std::endl;
    }

    // --- CLASSIFICAÇÃO DE PERFORMANCE ---
    // Baseado na performance de inserção sequencial (escrita mais pesada)
    double insert_ops_per_sec = (duration_insert > 0) ? static_cast<double>(N_KEYS) / duration_insert : 0;
    std::cout << " --- PERFORMANCE RANKING ---" << std::endl;
    std::cout << "Insercao Sequencial Media: " << std::fixed << std::setprecision(0) << insert_ops_per_sec << " ops/s" << std::endl;
    
    if (insert_ops_per_sec > 80000)      std::cout << "RANK: [ELITE]   - Performance de NVMe/Enterprise ou otimizacao extrema." << std::endl;
    else if (insert_ops_per_sec > 30000) std::cout << "RANK: [EXCELENTE] - SSD de alta performance." << std::endl;
    else if (insert_ops_per_sec > 10000) std::cout << "RANK: [BOA]     - SSD comum." << std::endl;
    else if (insert_ops_per_sec > 2000)  std::cout << "RANK: [MEDIA]   - HDD rapido ou SSD mais antigo." << std::endl;
    else                                 std::cout << "RANK: [LENTO]   - Provavelmente HDD ou gargalo de IO." << std::endl;

    std::cout << "==============================" << std::endl;

    return 0;
}
