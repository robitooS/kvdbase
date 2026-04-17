
# kvdbase

O kvdbase é um banco de dados chave-valor desenvolvido do zero em C++. Projetamos este sistema para salvar dados diretamente no disco físico de forma eficiente, sem a necessidade de carregar o banco inteiro na memória RAM.

Para garantir a velocidade de leitura e gravação, construímos uma arquitetura baseada em Árvore B+ (B+ Tree) com gerenciamento próprio de páginas de disco.

## Como Funciona

O projeto é estruturado em três componentes principais:

1. **Gerenciador de Disco (Pager):** O banco não lê ou escreve dados de forma aleatória. Ele divide o arquivo físico em blocos de 4 KB (Páginas). Quando você faz uma consulta, o Pager carrega para a memória apenas o bloco exato onde o dado está.
2. **Árvore B+ (B+ Tree):** É o índice do banco. Em vez de varrer o arquivo do início ao fim procurando uma chave, a árvore guia o algoritmo de busca diretamente para a página correta no disco.
3. **Controle de Concorrência:** O projeto está preparado para suporte a múltiplas threads (via flags de compilação), garantindo uma base sólida para futuras expansões de concorrência.

## Como Iniciar

**Pré-requisitos:**

* Compilador com suporte a C++17 (GCC 7+ ou Clang).
* GNU Make.

**Compilação e Execução:**

**Linux / macOS / WSL (Windows Subsystem for Linux):**

```bash
# Compilar o projeto
make

# Executar o benchmark de performance e integridade
./bin/kvdbase
```

**Windows (MinGW / G++ Manual):**

Caso não possua o `make` instalado, você pode compilar manualmente via terminal:

```powershell
# Criar pastas manualmente se necessário (bin e build)
g++ -std=c++17 -Iinclude src/btree.cpp src/pager.cpp src/kvdbase.cpp src/main.cpp -o bin/kvdbase.exe -pthread
./bin/kvdbase.exe
```

## Como usar no seu Projeto

Se você quiser usar o `kvdbase` como motor de armazenamento no seu próprio código C++, siga estes passos:

1. Copie as pastas `include/` e `src/` (exceto o `main.cpp`) para o seu diretório.
2. No seu código, inclua o cabeçalho: `#include "include/kvdbase.hpp"`.
3. Compile o seu código junto com os arquivos `src/kvdbase.cpp`, `src/btree.cpp` e `src/pager.cpp`.

**Exemplo de Compilação de um projeto próprio:**

```bash
g++ -std=c++17 -Ipath/to/include seu_codigo.cpp src/kvdbase.cpp src/btree.cpp src/pager.cpp -o seu_programa -pthread
```

## Exemplo de Uso (API)

**C++**

```cpp
#include "kvdbase.hpp"
#include <iostream>

int main() {
    // Inicializa o banco apontando para o arquivo no disco
    KVdbase db("dados.db");

    // Insere uma chave e um valor (atualmente suporta chaves e valores inteiros)
    db.put(1001, 500); 

    // Recupera o valor
    auto val = db.get(1001);  
  
    if (val.has_value()) {
        std::cout << "Encontrado: " << *val << std::endl;
    }

    // Remove uma chave
    if (db.remove(1001)) {
        std::cout << "Chave 1001 removida com sucesso." << std::endl;
    }
  
    return 0;
}
```

## Desenvolvedores

Este projeto é desenvolvido e mantido por:

* [ ] **Higor** ([robitooS](https://github.com/robitooS))
* [ ] **Paulo** ([paulorf0](https://github.com/paulorf0))

## Como Contribuir

O projeto é aberto para contribuições da comunidade. Para ajudar:

1. Faça um Fork do projeto.
2. Crie uma branch com a sua modificação (`git checkout -b minha-modificacao`).
3. Envie um Pull Request detalhando o que foi alterado.

## Licença

Este projeto é distribuído sob a licença MIT. Veja o arquivo `LICENSE` para mais detalhes
