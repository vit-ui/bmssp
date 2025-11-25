#include "../headers/algoritmo.hpp"
#include "../headers/estruturaD.hpp"

#include <vector>
#include <stack>
#include <algorithm>
#include <cmath>
#include <unordered_set>

namespace CaminhoMinimo {
    std::pair<std::vector<size_t>, std::vector<size_t>> Algoritmo::findPivots(double limiteB, std::vector<size_t> fronteiraInicialS)
    {
        std::vector<size_t> florestaF(tamGrafo, NULO);
        std::vector<size_t> camada(tamGrafo, NULO);

        for (size_t vertice : fronteiraInicialS) {
            camada[vertice] = 0;
        }

        // Bellman-Ford
        std::vector<char> verticesAlcancadosW(tamGrafo, false); // W
        std::vector<size_t> verticesAlcancadosWRetorno;
        std::vector<size_t> fronteiraAtualW_prev; // W_i-1. No caso: W_0

        // reserve some space to reduce reallocations
        verticesAlcancadosWRetorno.reserve(fronteiraInicialS.size() * (maxContagemK > 0 ? maxContagemK : 1));
        fronteiraAtualW_prev.reserve(fronteiraInicialS.size());

        for (size_t vertice : fronteiraInicialS) {
            if (!verticesAlcancadosW[vertice]) {
                verticesAlcancadosW[vertice] = true;
                verticesAlcancadosWRetorno.push_back(vertice);
                fronteiraAtualW_prev.push_back(vertice);
            }
        }

        std::vector<bool> adicionadoNestaCamada(tamGrafo, false); // para manter a ordem em que os vetores foram encontrados
        for (size_t i = 0; i < maxContagemK; i++) // no algo: i = 1 até k.
        {
            //std::set<int> proximaFronteiraW_i;
            std::vector<size_t> proximaFronteiraW_i;

            // reserve based on previous frontier size to reduce growth churn
            proximaFronteiraW_i.reserve(fronteiraAtualW_prev.size() * 2 + 1);

            std::fill(adicionadoNestaCamada.begin(), adicionadoNestaCamada.end(), false);

            for (size_t verticeU : fronteiraAtualW_prev) // vertice u da camada anterior
            {
                const auto& neighbors = ptrGrafo->at(verticeU);
                for (const auto& aresta : neighbors) // vizinho de u(v)
                {
                    size_t verticeDestinoV = aresta.first; // v
                    double pesoUV = aresta.second; // peso u -> v
                    double novoCusto = limpaRuido(distD[verticeU] + pesoUV); // distD[u] + peso[u,v]

                    if (novoCusto <= distD[verticeDestinoV]) // novo menor caminho?
                    {
                        if (novoCusto < limiteB) { // B é limite de distancia(janela que me importo)
                            // W_i U {v}
                            if (!adicionadoNestaCamada[verticeDestinoV]) {
                                adicionadoNestaCamada[verticeDestinoV] = true;
                                proximaFronteiraW_i.push_back(verticeDestinoV);
                            }
                            // já que tem que pertencer a W:
                            bool melhorou = novoCusto < distD[verticeDestinoV];
                            bool empateSeguro = (novoCusto == distD[verticeDestinoV]) && (florestaF[verticeDestinoV] == NULO || camada[verticeDestinoV] == i + 1);

                            if (melhorou || empateSeguro) {
                                florestaF[verticeDestinoV] = verticeU;
                                camada[verticeDestinoV] = i + 1;
                            }
                        }

                        // Importante ser a última coisa a ser feita!
                        distD[verticeDestinoV] = novoCusto; // Atualiza menor distancia
                    }
                }
            }
            fronteiraAtualW_prev.clear();
            // faz W U W_i
            for (size_t vertice : proximaFronteiraW_i) {
                if (!verticesAlcancadosW[vertice]) {
                    verticesAlcancadosW[vertice] = true;
                    verticesAlcancadosWRetorno.push_back(vertice);
                }
                fronteiraAtualW_prev.push_back(vertice); // precisamos fazer a fronteira atual ir para a anterior. W_i-1 = W_i implicito no artigo.
            }

            if (verticesAlcancadosWRetorno.size() > maxContagemK * fronteiraInicialS.size())
                return std::make_pair(fronteiraInicialS, verticesAlcancadosWRetorno); // retorna S direto já que P = S aqui.
        }
        // Construção de F: vetor florestaF já satisfaz as condições para pertencer a F. F == florestaF.
        // filhos servem para construir P
        std::vector<std::vector<size_t>> filhos(tamGrafo);

        for (size_t vertice : verticesAlcancadosWRetorno) {
            if (florestaF[vertice] != -1) {
                filhos[florestaF[vertice]].push_back(vertice);
            }
        }

        // Construção de P
        std::vector<bool> pivots(tamGrafo, false);
        std::vector<size_t> pivotsRetorno;

        for (size_t vertice : fronteiraInicialS) {
            if (!verticesAlcancadosW[vertice] || florestaF[vertice] != -1) continue;

            // Usando um DFS para percorrer F e achar os pivots.
            int contador = 0;
            //std::set<int> visitados;
            std::stack<size_t> pilha;

            pilha.push(vertice);
            //visitados.insert(vertice);

            while (!pilha.empty()) {
                int pai = pilha.top();
                pilha.pop();
                contador++;

                if (contador >= maxContagemK) {
                    if (!pivots[vertice]) {
                        pivots[vertice] = true;
                        pivotsRetorno.push_back(vertice);
                    }
                    break;
                }
                for (int v : filhos[pai]) {
                    pilha.push(v);
                }
            }
        }

        return std::make_pair(pivotsRetorno, verticesAlcancadosWRetorno);
    }

    // mini-Dijkstra modificado
    std::pair<double, std::vector<size_t>> Algoritmo::baseCase(double limiteB, size_t pivoFonteS) {
        std::vector<size_t> verticesCompletosU_0; // a primeira iteração do while adiciona pivoFonteS

        // fila de prioridades H: (distancia, vertice)
        FilaPrioridade filaFronteiraH;
        filaFronteiraH.push({ distD[pivoFonteS], pivoFonteS });

        while (!filaFronteiraH.empty() && verticesCompletosU_0.size() < maxContagemK + 1) {
            auto minPar = filaFronteiraH.top();
            filaFronteiraH.pop();

            double pesoAresta = minPar.first;
            size_t verticeAtualU = minPar.second;

            if (pesoAresta > distD[verticeAtualU]) continue; // isso cuida de duplicatas obsoletas na fila

            verticesCompletosU_0.push_back(verticeAtualU); // cada vertice é adicionado somente uma vez por causa da verificação acima

            const auto& neighbors = ptrGrafo->at(verticeAtualU);
            for (const auto& parVizinho : neighbors) {
                size_t vizinho = parVizinho.first; // vizinho é v - verticeAtualU é u
                double pesoUV = parVizinho.second; // peso[u, v]

                double novoCusto = limpaRuido(distD[verticeAtualU] + pesoUV);
                if (novoCusto <= distD[vizinho] && novoCusto < limiteB) {
                    distD[vizinho] = novoCusto;

                    // Não precisa verificar se já está na fila
                    filaFronteiraH.push({ distD[vizinho], vizinho });
                }
            }
        }
        if (verticesCompletosU_0.size() <= maxContagemK)
            return std::make_pair(limiteB, verticesCompletosU_0);
        else {
            // Encontrando a distancia maxima em U_0
            double Blinha = -std::numeric_limits<double>::infinity(); // isso funciona como o infinito do artigo
            for (size_t vertice : verticesCompletosU_0)
                if (Blinha < distD[vertice]) Blinha = distD[vertice];

            // Construindo U para retorno(retirando vertices onde distancia < Blinha
            std::vector<size_t> U;
            U.reserve(verticesCompletosU_0.size());
            for (size_t vertice : verticesCompletosU_0)
                if (Blinha > distD[vertice]) U.push_back(vertice);

            return std::make_pair(Blinha, U);
        }
    }

    // Bounded Multi-Source Shortest Path(BMSSP)
    std::pair<double, std::vector<size_t>> Algoritmo::bmssp(int nivel, double limiteSuperiorGlobalB, std::vector<size_t> fronteiraS) {
        if (nivel == 0) { // S é um singleton(único elemento)
            return baseCase(limiteSuperiorGlobalB, fronteiraS[0]);
        }

        auto resultadoPivots = findPivots(limiteSuperiorGlobalB, fronteiraS);

        std::vector<size_t> pivotsP = resultadoPivots.first;
        std::vector<size_t> verticesAlcancadosW = resultadoPivots.second;

        //size_t tamLoteM = size_t(std::pow(2, (nivel - 1) * passosT));

        size_t tamLoteM = 1ULL << ((static_cast<size_t>(nivel) - 1) * passosT); // evita mexer com ponto flutuante. ULL = unsigned long long

        D estruturaD(tamLoteM, limiteSuperiorGlobalB);

        double limiteInferiorAnterior;

        if (pivotsP.empty())
            limiteInferiorAnterior = limiteSuperiorGlobalB;
        else {
            double menor = distD[pivotsP[0]];
            for (size_t vertice : pivotsP) {
                double dist = distD[vertice];
                if (menor > dist) menor = dist;
            }

            limiteInferiorAnterior = menor;
        }

        for (size_t vertice : pivotsP) {
            estruturaD.insert(vertice, distD[vertice]);
        }

        std::vector<size_t> verticesResolvidosU;
        // replace unordered_set with a dense boolean vector for faster membership checks
        std::vector<char> setControleDuplicatas(tamGrafo, 0);

        double Bfinal = limiteSuperiorGlobalB;
        // replace pow with integer shift when safe
        size_t shift = static_cast<size_t>(nivel) * passosT;
        double limite;
        if (shift < (sizeof(unsigned long long) * 8 - 1))
            limite = static_cast<double>(maxContagemK) * static_cast<double>(1ULL << shift);
        else
            limite = static_cast<double>(maxContagemK) * std::pow(2.0, static_cast<double>(shift));

        while (verticesResolvidosU.size() < static_cast<size_t>(limite)) {
            auto resultadoPull = estruturaD.pull();
            double limiteSuperiorLoteBi = resultadoPull.first;
            // avoid copying the vector if possible
            std::vector<ParDistVertice> paresExtraidosDoPull = std::move(resultadoPull.second);

            if (paresExtraidosDoPull.empty())
            {
                Bfinal = limiteSuperiorGlobalB;
                break;
            }

            std::vector<size_t> pivotsLoteAtual;
            pivotsLoteAtual.reserve(paresExtraidosDoPull.size());
            for (const auto& par : paresExtraidosDoPull) {
                pivotsLoteAtual.push_back(par.second);
            }
            auto resultadoRecursivo = bmssp(nivel - 1, limiteSuperiorLoteBi, pivotsLoteAtual);

            double limiteAlcancadoRecursao = resultadoRecursivo.first;
            std::vector<size_t> verticesResolvidosLote = resultadoRecursivo.second;

            Bfinal = limiteAlcancadoRecursao;

            for (size_t v : verticesResolvidosLote) {
                if (!setControleDuplicatas[v]) {
                    verticesResolvidosU.push_back(v);
                    setControleDuplicatas[v] = 1;
                }
            }

            std::vector<ParDistVertice> loteTemporarioK;
            loteTemporarioK.reserve(verticesResolvidosLote.size() * 2 + pivotsLoteAtual.size());

            for (size_t verticeU : verticesResolvidosLote) {
                const auto& neighbors = ptrGrafo->at(verticeU);
                for (const auto& aresta : neighbors) {
                    size_t vizinhoV = aresta.first; // v
                    double pesoUV = aresta.second; // peso u -> v

                    double novoCusto = limpaRuido(distD[verticeU] + pesoUV);

                    if (novoCusto <= distD[vizinhoV]) {
                        distD[vizinhoV] = novoCusto;
                        if ((novoCusto >= limiteSuperiorLoteBi && novoCusto < limiteSuperiorGlobalB))
                            estruturaD.insert(vizinhoV, novoCusto);
                        else if (novoCusto >= limiteAlcancadoRecursao && novoCusto < limiteSuperiorLoteBi)
                            loteTemporarioK.push_back({ novoCusto, vizinhoV });
                    }
                }
            }

            for (size_t vertice : pivotsLoteAtual) {
                if (distD[vertice] >= limiteAlcancadoRecursao && distD[vertice] < limiteSuperiorLoteBi)
                    loteTemporarioK.push_back({ distD[vertice], vertice });
            }

            estruturaD.batchPrepend(loteTemporarioK);
            limiteInferiorAnterior = limiteAlcancadoRecursao;
        }
        // --- CÓDIGO NOVO PARA O FINAL DA FUNÇÃO ---

        // Não precisamos declarar 'jaProcessados' nem recriá-lo.
        // Usamos o 'setControleDuplicatas' que mantivemos atualizado durante o while.

        for (size_t vertice : verticesAlcancadosW) {
            if (distD[vertice] < Bfinal) {
                // Verifica se já processamos usando o set principal
                if (!setControleDuplicatas[vertice]) {
                    verticesResolvidosU.push_back(vertice);
                    setControleDuplicatas[vertice] = 1; // Mantém atualizado
                }
            }
        }

        return std::make_pair(Bfinal, verticesResolvidosU);
    }
}
