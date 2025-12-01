# Benchmark: Dijkstra vs BMSSP

Este projeto implementa e compara o desempenho do algoritmo clássico de **Dijkstra** com o algoritmo **BMSSP** (*Breaking the Sorting Barrier*) para o problema de Caminho Mínimo de Fonte Única (SSSP).

O código executa testes de estresse em grafos de diferentes tamanhos, valida a corretude dos resultados e exporta as métricas de tempo para análise.
Os grafos são gerados aleatoriamente pela função `geraGrafos`.

## Estrutura
- **src/**: Implementação dos algoritmos e lógica de teste (`main.cpp`).
- **headers/**: Definições e estruturas de dados.
- **lib/**: Bibliotecas auxiliares (JSON).

## Como Rodar

### Via Terminal (G++/GCC)
Na pasta raiz do projeto, execute:

```bash
# Compilar
g++ src/*.cpp -o benchmark

# Executar (Linux/Mac)
./benchmark

# Executar (Windows)
benchmark.exe
```

## Saída
O programa gera um arquivo `.csv` contendo os tempos de execução em microssegundos e informa se houve divergências entre os algoritmos.