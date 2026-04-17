#include <iostream>
#include <chrono>
#include <vector>
#include <numeric>
#include <algorithm>
#include <random>
#include <iomanip>
#include <string>
#include <cstdio>
#include "kvdbase.hpp"
#include <set>

using namespace std::chrono;

void print_report(const std::string& task, double duration, int ops) {
    double ops_per_sec = (duration > 0) ? static_cast<double>(ops) / duration : 0;
    std::cout << std::left << std::setw(35) << task 
              << ": " << std::fixed << std::setprecision(3) << duration << "s "
              << "(" << std::setprecision(0) << ops_per_sec << " ops/s)" << std::endl;
}

int main() {
    const int N_KEYS = 1000000;
    const std::string DB_NAME = "performance_test.db";
    std::remove(DB_NAME.c_str()); 

    KVdbase db(DB_NAME);

    std::cout << "===== FORENSIC PERFORMANCE SUITE (" << N_KEYS << " KEYS) =====" << std::endl;

    auto start_insert = high_resolution_clock::now();
    for (int i = 0; i < N_KEYS; ++i) {
        db.put(i, i * 3);
        if (i > 0 && i % (N_KEYS / 10) == 0) {
             std::cout << "... " << i << "/" << N_KEYS << " inserts." << std::endl;
        }
    }
    auto end_insert = high_resolution_clock::now();
    print_report("Fase 1: Insercao Sequencial", duration<double>(end_insert - start_insert).count(), N_KEYS);

    std::vector<int> keys_to_search(N_KEYS);
    std::iota(keys_to_search.begin(), keys_to_search.end(), 0);
    std::shuffle(keys_to_search.begin(), keys_to_search.end(), std::mt19937{std::random_device{}()});

    auto start_search = high_resolution_clock::now();
    int found_count = 0;
    for (int i = 0; i < N_KEYS; ++i) {
        if (db.get(keys_to_search[i])) found_count++;
    }
    auto end_search = high_resolution_clock::now();
    print_report("Fase 2: Busca Aleatoria", duration<double>(end_search - start_search).count(), N_KEYS);
    
    if (found_count != N_KEYS) {
        std::cerr << "[CRITICAL] Indice quebrado na Fase 1. Faltam: " << (N_KEYS - found_count) << " chaves." << std::endl;
    }

    auto start_update = high_resolution_clock::now();
    for (int i = 0; i < N_KEYS / 2; ++i) {
        db.put(i, i + 1000);
    }
    auto end_update = high_resolution_clock::now();
    print_report("Fase 3: Atualizacao Massiva", duration<double>(end_update - start_update).count(), N_KEYS / 2);

    std::set<int> actual_removed_keys;
    auto start_remove = high_resolution_clock::now();
    int removed_success = 0;
    
    std::shuffle(keys_to_search.begin(), keys_to_search.end(), std::mt19937{std::random_device{}()});
    for (int i = 0; i < N_KEYS / 2; ++i) {
        int target_key = keys_to_search[i];
        actual_removed_keys.insert(target_key);
        if (db.remove(target_key)) {
            removed_success++;
        }
    }
    auto end_remove = high_resolution_clock::now();
    print_report("Fase 4: Remocao Aleatoria", duration<double>(end_remove - start_remove).count(), N_KEYS / 2);
    std::cout << "-> db.remove() retornou true para: " << removed_success << "/" << (N_KEYS/2) << std::endl;

    // Métricas Forenses
    int correctly_retained = 0;
    int correctly_removed = 0;
    int error_ghost_keys = 0;        // Chave deveria estar removida, mas ainda existe
    int error_missing_keys = 0;      // Chave deveria existir, mas sumiu
    int error_wrong_value_updated = 0; // Chave existe, sofreu update (Fase 3), mas o valor esta errado
    int error_wrong_value_original = 0;// Chave existe, intacta (Fase 1), mas o valor esta errado

    for (int i = 0; i < N_KEYS; ++i) {
        auto val = db.get(i);
        bool should_be_removed = actual_removed_keys.count(i);

        if (should_be_removed) {
            if (!val) correctly_removed++;
            else error_ghost_keys++;
        } else {
            if (!val) {
                error_missing_keys++;
            } else {
                if (i < N_KEYS / 2) {
                    if (*val == i + 1000) correctly_retained++;
                    else error_wrong_value_updated++;
                } else {
                    if (*val == i * 3) correctly_retained++;
                    else error_wrong_value_original++;
                }
            }
        }
    }

    std::cout << "\n--- RELATORIO FORENSE DE INTEGRIDADE ---" << std::endl;
    std::cout << "[SUCESSO]" << std::endl;
    std::cout << "Retidas Corretamente   : " << correctly_retained << "/" << (N_KEYS - actual_removed_keys.size()) << std::endl;
    std::cout << "Removidas Corretamente : " << correctly_removed << "/" << actual_removed_keys.size() << std::endl;
    
    std::cout << "\n[FALHAS DIRECIONADAS]" << std::endl;
    std::cout << "Ghost Keys (Ressuscitadas)         : " << error_ghost_keys << " (Falha em delete/tombstone)" << std::endl;
    std::cout << "Missing Keys (Sumiço Silencioso)   : " << error_missing_keys << " (Falha de split/pointer loss)" << std::endl;
    std::cout << "Valor Incorreto (Falha no Update)  : " << error_wrong_value_updated << " (Dirty page eviction falhou)" << std::endl;
    std::cout << "Valor Incorreto (Corrupcao Fase 1) : " << error_wrong_value_original << " (Sobrescrita de memoria incorreta)" << std::endl;

    int total_errors = error_ghost_keys + error_missing_keys + error_wrong_value_updated + error_wrong_value_original;
    
    if (total_errors == 0) {
        std::cout << "\nSTATUS: MOTOR ESTAVEL. Zero corrupcao de indice." << std::endl;
    } else {
        std::cerr << "\nSTATUS: COLAPSO ESTRUTURAL. " << total_errors << " anomalias detectadas." << std::endl;
    }

    return 0;
}