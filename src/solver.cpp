#include <vector>
#include <utility>   // Para std::pair
#include <limits>   // Para INT_MAX
#include <iostream>  // (Apenas para o main de exemplo)

const int INFINITO = std::numeric_limits<double>::infinity();

using Par = std::pair<int, int>;           // {Destino, Peso}
using Grafo = std::vector<std::vector<Par>>; // Lista de Adjacência

/**
 * SOLVER (ORÁCULO)
 * * Roda o Algoritmo de Bellman-Ford para encontrar a "resposta correta".
 * Este algoritmo é mais lento (O(N*E)), mas sua lógica simples
 * serve como uma "prova" para validar seu Dijkstra otimizado.
 */
std::pair<std::vector<int>, std::vector<int>> bellmanFord(const Grafo& grafo) {
    
    // (Pode ser 'int' ou 'size_t', desde que seja consistente)
    size_t tamanho = grafo.size();

    // 1. Inicialização (Idêntica ao Dijkstra)
    std::vector<int> minDistancia(tamanho, INFINITO);
    std::vector<int> predecessores(tamanho, -1); // -1 = nulo
    minDistancia[0] = 0; // Origem

    // Loop principal (roda N-1 vezes)
    for (size_t i = 1; i < tamanho; ++i) {
        // u = vértice de origem da aresta
        for (size_t u = 0; u < tamanho; ++u) {
            
            // v = vértice de destino da aresta
            for (const auto& parVizinho : grafo[u]) {
                int v = parVizinho.first;
                int peso = parVizinho.second;

                // Lógica de Relaxamento (O coração do algoritmo)
                
                // Se a origem 'u' ainda é inalcançável, 
                // não podemos relaxar a partir dela.
                if (minDistancia[u] == INFINITO) continue;

                // Verificação de overflow (igual ao Dijkstra)
                if (minDistancia[u] > INFINITO - peso) continue;

                int distanciaNova = minDistancia[u] + peso;

                if (distanciaNova < minDistancia[v]) {
                    minDistancia[v] = distanciaNova;
                    predecessores[v] = u;
                }
            }
        }
    }
    // 3. Retorna o resultado (o "gabarito")
    return std::make_pair(minDistancia, predecessores);
}