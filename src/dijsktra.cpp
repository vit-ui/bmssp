#include "../headers/algoritmo.hpp"

#include <vector>
#include <algorithm>

namespace CaminhoMinimo {
    std::vector<size_t> Algoritmo::dijkstra(size_t origem) {
        // a distancia é salva diretamente em distD.
        std::vector<size_t> predecessores(tamGrafo, NULO); //rot

        FilaPrioridade verticesParaProcessar;
        verticesParaProcessar.push({ 0.0, origem });

        while (!verticesParaProcessar.empty()) {
            auto parAtual = verticesParaProcessar.top();
            verticesParaProcessar.pop();

            double distancia = parAtual.first;
            size_t verticeAtual = parAtual.second;
            if (distancia > distD[verticeAtual]) continue;

            for (const auto& parVizinho : ptrGrafo->at(verticeAtual)) {
                size_t vizinho = parVizinho.first;
                double peso = parVizinho.second;

                // linha 14 era redundante. Pulei direto para 15.
                double distanciaNova = limpaRuido(distD[verticeAtual] + peso);
                if (distanciaNova < distD[vizinho]) {
                    distD[vizinho] = distanciaNova;
                    predecessores[vizinho] = verticeAtual;
                    verticesParaProcessar.push({ distanciaNova, vizinho });
                }
            }
        }
        return predecessores;
    }
}