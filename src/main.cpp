#include "../include/kvdbase.hpp" 
#include <iostream>

int main()
{
    KVdbase kvdb;

    if (kvdb.put("higor", "lindo")) {
        
        auto result = kvdb.get("higor");

        if (result.has_value()) {
            std::cout << "Sucesso na inserção. Valor recuperado: " << *result << std::endl;
        } else {
            std::cerr << "Erro: Chave inserida mas nao encontrada no retorno." << std::endl;
        }
    }

    return 0;
}