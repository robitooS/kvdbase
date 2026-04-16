#include <iostream>
#include <chrono>
#include <vector>
#include <numeric>
#include <algorithm>
#include <random>
#include <iomanip>
#include "kvdbase.hpp"

using namespace std::chrono;

void print_report(std::string task, double duration, int ops) {
    double ops_per_sec = (duration > 0) ? ops / duration : 0;
    std::cout << std::left << std::setw(30) << task 
              << ": " << std::fixed << std::setprecision(3) << duration << "s "
              << "(" << std::setprecision(0) << ops_per_sec << " ops/s)" << std::endl;
}

int main() {
    const int N = 500000; 
    const std::string DB_NAME = "ultra_heavy.db";
    std::remove(DB_NAME.c_str());

    KVdbase db(DB_NAME);
    std::cout << "===== ULTRA HEAVY LOAD TEST (0.5 MILLION KEYS) =====" << std::endl;

    // 1. INSERÇÃO SEQUENCIAL (Escrita Pesada)
    auto s1 = high_resolution_clock::now();
    for (int i = 0; i < N; ++i) {
        db.put(i, i + 100);
        if (i % 100000 == 0 && i > 0) std::cout << "... inseriu " << i << " chaves" << std::endl;
    }
    auto e1 = high_resolution_clock::now();
    double d1 = duration<double>(e1 - s1).count();
    print_report("Insercao Sequencial", d1, N);

    // 2. BUSCA ALEATÓRIA (Teste de Cache/Disco)
    std::vector<int> keys(N);
    std::iota(keys.begin(), keys.end(), 0);
    std::shuffle(keys.begin(), keys.end(), std::mt19937{std::random_device{}()});

    auto s2 = high_resolution_clock::now();
    int found = 0;
    for (int i = 0; i < 50000; ++i) { // 50k buscas aleatórias
        if (db.get(keys[i])) found++;
    }
    auto e2 = high_resolution_clock::now();
    double d2 = duration<double>(e2 - s2).count();
    print_report("Busca Aleatoria (50k)", d2, 50000);

    // 3. ATUALIZAÇÃO (Overwrite)
    auto s3 = high_resolution_clock::now();
    for (int i = 0; i < 100000; ++i) { // Atualiza 100k chaves
        db.put(i, i + 999);
    }
    auto e3 = high_resolution_clock::now();
    double d3 = duration<double>(e3 - s3).count();
    print_report("Atualizacao (100k)", d3, 100000);

    // 4. REMOÇÃO ALEATÓRIA (Pancada Final)
    auto s4 = high_resolution_clock::now();
    for (int i = 0; i < 100000; ++i) {
        db.remove(keys[i]);
    }
    auto e4 = high_resolution_clock::now();
    double d4 = duration<double>(e4 - s4).count();
    print_report("Remocao Aleatoria (100k)", d4, 100000);

    // ANALISE DE RESULTADOS
    double avg_ops = (N / d1);
    std::cout << "\n===== VEREDITO DE PERFORMANCE =====" << std::endl;
    std::cout << "Media de Insercao: " << std::fixed << std::setprecision(0) << avg_ops << " ops/s" << std::endl;
    
    if (avg_ops > 50000)      std::cout << "RANK: [ELITE] - Performance de NVMe/Enterprise." << std::endl;
    else if (avg_ops > 10000) std::cout << "RANK: [BOA]   - SSD de alta performance." << std::endl;
    else if (avg_ops > 2000)  std::cout << "RANK: [MEDIA] - SSD comum ou HDD rapido." << std::endl;
    else                     std::cout << "RANK: [LENTO] - Provavelmente um HDD ou gargalo de IO." << std::endl;

    // Verificação de Erros
    if (found != 50000) std::cout << "ALERTA: Integridade comprometida!" << std::endl;
    else std::cout << "INTEGRIDADE: 100% OK." << std::endl;

    return 0;
}
