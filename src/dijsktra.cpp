#include <vector>
#include <queue>
#include <limits>
#include <algorithm>
#include <utility>
#include <iostream>
#include "./helpers.cpp"

using Grafo = std::vector<std::vector<std::pair<int, int>>>;

std::pair<std::vector<int>, std::vector<int>> dijkstra(const Grafo& grafo){
    int tamanho = grafo.size();

    std::vector<int> minDistancia(tamanho, INFINITO); //dt
    std::vector<int> predecessores(tamanho, -1); //rot

    std::priority_queue<std::pair<int, int>, std::vector<std::pair<int, int>>, std::greater<std::pair<int, int>>> verticesParaProcessar;
    minDistancia[0] = 0;
    verticesParaProcessar.push({0,0});

    while(!verticesParaProcessar.empty()){
        auto parAtual = verticesParaProcessar.top();
        verticesParaProcessar.pop();

        int distancia = parAtual.first;
        int verticeAtual = parAtual.second;
        if (distancia > minDistancia[verticeAtual]) continue;

        for(const auto& parVizinho : grafo[verticeAtual]){
            int vizinho = parVizinho.first;
            int peso = parVizinho.second;

            // linha 14 era redundante. Pulei direto para 15.
            int distanciaNova = minDistancia[verticeAtual] + peso;
            if(distanciaNova < minDistancia[vizinho]){
                minDistancia[vizinho] = distanciaNova;
                predecessores[vizinho] = verticeAtual;
                verticesParaProcessar.push({distanciaNova, vizinho});
            }
        }
    }
    return std::make_pair(minDistancia, predecessores);
}