
# kvdbase

O kvdbase é um banco de dados chave-valor desenvolvido do zero em C++. Projetamos este sistema para salvar dados diretamente no disco físico de forma eficiente, sem a necessidade de carregar o banco inteiro na memória RAM.

Para garantir a velocidade de leitura e gravação, construímos uma arquitetura baseada em Árvore B+ (B+ Tree) com gerenciamento próprio de páginas de disco.

## Como Funciona

O projeto é estruturado em três componentes principais:

1. **Gerenciador de Disco (Pager):** O banco não lê ou escreve dados de forma aleatória. Ele divide o arquivo físico em blocos de 4 KB (Páginas). Quando você faz uma consulta, o Pager carrega para a memória apenas o bloco exato onde o dado está.
2. **Árvore B+ (B+ Tree):** É o índice do banco. Em vez de varrer o arquivo do início ao fim procurando uma chave, a árvore guia o algoritmo de busca diretamente para a página correta no disco.
3. **Controle de Concorrência:** O código é protegido por travas de segurança (Mutex) para garantir que o banco possa receber múltiplas conexões e operações ao mesmo tempo sem corromper os dados gravados.

## Como Iniciar

**Pré-requisitos:**

* Compilador com suporte a C++17 (GCC 7+ ou Clang).
* GNU Make.

**Compilação e Execução:**

**Bash**

```
# Clonar o repositório
git clone git@github.com:robitooS/kvdbase.git
cd kvdbase/

# Compilar o projeto (o binário será gerado na pasta bin/)
make

# Executar
./bin/kvdbase
```

**Exemplo de Uso:**

**C++**

```
#include "kvdbase.hpp"
#include <iostream>

int main() {
    // Inicializa o banco apontando para o arquivo no disco
    KVdbase db("dados.db");

    // Insere uma chave e um valor
    db.put("usuario_1", "Higor"); 

    // Recupera o valor
    auto val = db.get("usuario_1");  
  
    if (val.has_value()) {
        std::cout << "Encontrado: " << *val << std::endl;
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
