#include "../headers/algoritmo.hpp"
#include "../headers/estruturaD.hpp"

#include <vector>
#include <stack>
#include <algorithm>
#include <cmath>

namespace CaminhoMinimo {
    std::pair<std::vector<size_t>, std::vector<size_t>> Algoritmo::findPivots(double limiteB, std::vector<size_t> fronteiraInicialS)
    {
        std::vector<size_t> florestaF(tamGrafo, NULO);
        std::vector<size_t> camada(tamGrafo, NULO);

        for (size_t vertice : fronteiraInicialS) {
            camada[vertice] = 0;
        }

        // Bellman-Ford
        std::vector<bool> verticesAlcancadosW(tamGrafo, false); // W
        std::vector<size_t> verticesAlcancadosWRetorno;
        std::vector<size_t> fronteiraAtualW_prev; // W_i-1. No caso: W_0

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

            std::fill(adicionadoNestaCamada.begin(), adicionadoNestaCamada.end(), false);

            for (size_t verticeU : fronteiraAtualW_prev) // vertice u da camada anterior
            {
                for (const auto& aresta : ptrGrafo->at(verticeU)) // vizinho de u(v)
                {
                    size_t verticeDestinoV = aresta.first; // v
                    double pesoUV = aresta.second; // peso u -> v
                    double novoCusto = limpaRuido(distD[verticeU] + pesoUV); // distD[u] + peso[u,v]

                    //// Verifica se é MELHOR ou se é um EMPATE SEGURO
                    //bool melhorou = novoCusto < distD[verticeDestinoV];
                    //bool empateSeguro = (novoCusto == distD[verticeDestinoV]) && (florestaF[verticeDestinoV] == -1 || camada[verticeDestinoV] == i + 1);

                    //if (melhorou || empateSeguro) {
                    //    florestaF[verticeDestinoV] = verticeU;
                    //    camada[verticeDestinoV] = i + 1;
                    //}

                    //// Atualiza distância e fronteira APENAS se melhorou (Eficiência e Correção)
                    //if (melhorou) {
                    //    distD[verticeDestinoV] = novoCusto;

                    //    if (novoCusto < limiteB) {
                    //        if (!adicionadoNestaCamada[verticeDestinoV]) {
                    //            adicionadoNestaCamada[verticeDestinoV] = true;
                    //            proximaFronteiraW_i.push_back(verticeDestinoV);
                    //        }
                    //    }
                    //}

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
                //for (const auto& aresta : grafo[pai]) {
                //    int verticeDestino = aresta.first; // v
                //    int peso = aresta.second; // peso u -> v

                //    // Testes para garantir que percorremos somente vertices em F
                //    bool testes = verticesAlcancadosW[verticeDestino] && distD[verticeDestino] == distD[pai] + peso && !visitados.count(verticeDestino);
                //    if (testes) {
                //        pilha.push(verticeDestino);
                //        visitados.insert(verticeDestino);
                //    }
                //}
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

            for (auto& parVizinho : ptrGrafo->at(verticeAtualU)) {
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

        double Bfinal = limiteSuperiorGlobalB;
        double limite = maxContagemK * pow(2, nivel * passosT);

        while (verticesResolvidosU.size() < static_cast<size_t>(limite)) {
            auto resultadoPull = estruturaD.pull();
            double limiteSuperiorLoteBi = resultadoPull.first;
            std::vector<ParDistVertice> paresExtraidosDoPull = resultadoPull.second;

            if (paresExtraidosDoPull.empty())
            {
                Bfinal = limiteSuperiorGlobalB;
                break;
            }

            std::vector<size_t> pivotsLoteAtual;
            for (const auto& par : paresExtraidosDoPull) {
                pivotsLoteAtual.push_back(par.second);
            }
            auto resultadoRecursivo = bmssp(nivel - 1, limiteSuperiorLoteBi, pivotsLoteAtual);

            double limiteAlcancadoRecursao = resultadoRecursivo.first;
            std::vector<size_t> verticesResolvidosLote = resultadoRecursivo.second;

            Bfinal = limiteAlcancadoRecursao;

            verticesResolvidosU.insert(verticesResolvidosU.end(), verticesResolvidosLote.begin(), verticesResolvidosLote.end());

            std::vector<ParDistVertice> loteTemporarioK;

            for (size_t verticeU : verticesResolvidosLote) {
                for (const auto& aresta : ptrGrafo->at(verticeU)) {
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
        for (size_t vertice : verticesAlcancadosW) {
            if (distD[vertice] < Bfinal) {
                verticesResolvidosU.push_back(vertice);
            }
        }
        return std::make_pair(Bfinal, verticesResolvidosU);
    }
}
