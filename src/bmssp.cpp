#include <vector>
#include <set>
#include <queue>
#include <stack>
#include <algorithm>
#include <cmath>

#include "estruturaD.h"
//#include <limits>

using Grafo = std::vector<std::vector<std::pair<int, int>>>;

// Placeholders
 //std::vector<double> distD(std::numeric_limits<double>::infinity());
 //distD[0] = 0.0;

std::pair<std::vector<int>, std::vector<int>> findPivots(const Grafo &grafo, std::vector<double> &distD, double limiteB, std::vector<int> fronteiraInicialS, int passosK)
{

    std::vector<int> florestaF(grafo.size(), -1);
    std::vector<int> camada(grafo.size(), -1);

    for (int vertice : fronteiraInicialS) {
        camada[vertice] = 0;
    }

    // Bellman-Ford
    std::vector<bool> verticesAlcancadosW(grafo.size(), false); // W
    std::vector<int> verticesAlcancadosWRetorno;
    std::vector<int> fronteiraAtualW_prev; // W_i-1. No caso: W_0

    for (int vertice : fronteiraInicialS) {
        if (!verticesAlcancadosW[vertice]) {
            verticesAlcancadosW[vertice] = true;
            verticesAlcancadosWRetorno.push_back(vertice);
            fronteiraAtualW_prev.push_back(vertice);
        }
    }

    std::vector<bool> adicionadoNestaCamada(grafo.size(), false); // para manter a ordem em que os vetores foram encontrados
    for (int i = 0; i < passosK; i++) // no algo: i = 1 até k.
    {
        //std::set<int> proximaFronteiraW_i;
        std::vector<int> proximaFronteiraW_i;

        std::fill(adicionadoNestaCamada.begin(), adicionadoNestaCamada.end(), false);

        for (int verticeU : fronteiraAtualW_prev) // vertice u da camada anterior
        {
            //adicionadoNestaCamada[verticeU] = false;

            for (const auto& aresta : grafo[verticeU]) // vizinho de u(v)
            {
                int verticeDestinoV = aresta.first; // v
                int pesoUV = aresta.second; // peso u -> v
                double novoCusto = distD[verticeU] + pesoUV; // distD[u] + peso[u,v]


                // Verifica se é MELHOR ou se é um EMPATE SEGURO
                bool melhorou = novoCusto < distD[verticeDestinoV];
                bool empateSeguro = (novoCusto == distD[verticeDestinoV]) && (florestaF[verticeDestinoV] == -1 || camada[verticeDestinoV] == i + 1);

                if (melhorou || empateSeguro) {
                    florestaF[verticeDestinoV] = verticeU;
                    camada[verticeDestinoV] = i + 1;
                }

                // Atualiza distância e fronteira APENAS se melhorou (Eficiência e Correção)
                if (melhorou) {
                    distD[verticeDestinoV] = novoCusto;

                    if (novoCusto < limiteB) {
                        if (!adicionadoNestaCamada[verticeDestinoV]) {
                            adicionadoNestaCamada[verticeDestinoV] = true;
                            proximaFronteiraW_i.push_back(verticeDestinoV);
                        }
                    }
                }

                //if (novoCusto <= distD[verticeDestinoV]) // novo menor caminho?
                //{
                //    if (novoCusto != distD[verticeDestinoV]) {
                //        florestaF[verticeDestinoV] = verticeU; // para casos de empate, não altera florestaF. Tambem: (florestaF[v], v) é uma aresta tight
                //    }

                //    if (novoCusto == distD[verticeDestinoV] && (florestaF[verticeDestinoV] == -1 || camada[verticeDestinoV] == i + 1)) {
                //        florestaF[verticeDestinoV] = verticeU;
                //        camada[verticeDestinoV] = i + 1;
                //    }
                //    distD[verticeDestinoV] = novoCusto; // Atualiza menor distancia

                //    if (novoCusto < limiteB) { // B é limite de distancia(janela que me importo)
                //        // W_i U {v}
                //        if (!adicionadoNestaCamada[verticeDestinoV]) {
                //            adicionadoNestaCamada[verticeDestinoV] = true;
                //            proximaFronteiraW_i.push_back(verticeDestinoV);
                //        }
                //    }
                //}
            }
        }
        fronteiraAtualW_prev.clear();
        // faz W U W_i
        for (int vertice : proximaFronteiraW_i) {
            if (!verticesAlcancadosW[vertice]) {
                verticesAlcancadosW[vertice] = true;
                verticesAlcancadosWRetorno.push_back(vertice);
            }
            fronteiraAtualW_prev.push_back(vertice); // precisamos fazer a fronteira atual ir para a anterior. W_i-1 = W_i implicito no artigo.
        }

        if (verticesAlcancadosWRetorno.size() > passosK * fronteiraInicialS.size())
            return std::make_pair(fronteiraInicialS, verticesAlcancadosWRetorno); // retorna S direto já que P = S aqui.
    }
    // Construção de F: vetor florestaF já satisfaz as condições para pertencer a F. F == florestaF, onde florestaF[v] != -1.
    // filhos servem para construir P
    std::vector<std::vector<int>> filhos(grafo.size());

    for (int vertice : verticesAlcancadosWRetorno) {
        if (florestaF[vertice] != -1) {
            filhos[florestaF[vertice]].push_back(vertice);
        }
    }

    // Construção de P
    std::vector<bool> pivots(grafo.size(), false);
    std::vector<int> pivotsRetorno;

    for (int vertice : fronteiraInicialS) {
        if (!verticesAlcancadosW[vertice] || florestaF[vertice] != -1) continue;

        // Usando um DFS para percorrer F e achar os pivots.
        int contador = 0;
        //std::set<int> visitados;
        std::stack<int> pilha;

        pilha.push(vertice);
        //visitados.insert(vertice);

        while (!pilha.empty()) {
            int pai = pilha.top();
            pilha.pop();
            contador++;

            if (contador >= passosK) {
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

std::pair<double, std::vector<int>> baseCase(const Grafo& grafo, std::vector<double>& distD, double limiteB, int pivoFonteS, int maxContagemK) {
    std::vector<int> verticesCompletosU_0; // a primeira iteração do while adiciona pivoFonteS
    // fila de prioridades H: (distancia, vertice)
    std::priority_queue<std::pair<double, int>, std::vector<std::pair<double, int>>, std::greater<std::pair<double, int>>> filaFronteiraH;
    filaFronteiraH.push({ distD[pivoFonteS], pivoFonteS });

    while (!filaFronteiraH.empty() && verticesCompletosU_0.size() < maxContagemK + 1) {
        auto minPar = filaFronteiraH.top();
        filaFronteiraH.pop();

        double pesoAresta = minPar.first;
        int verticeAtualU = minPar.second;
        if (pesoAresta > distD[verticeAtualU]) continue; // isso cuida de duplicatas obsoletas na fila

        verticesCompletosU_0.push_back(verticeAtualU); // cada vertice é adicionado somente uma vez por causa da verificação acima

        for (auto& parVizinho : grafo[verticeAtualU]) {
            int vizinho = parVizinho.first; // vizinho é v - verticeAtualU é u
            int pesoUV = parVizinho.second; // peso[u, v]

            int novoCusto = distD[verticeAtualU] + pesoUV;
            if (novoCusto <= distD[vizinho] && novoCusto < limiteB) {
                distD[vizinho] = novoCusto;

                // Não precisa verificar se já está na fila
                filaFronteiraH.push({ distD[vizinho], vizinho });
            }
        }
    }
    if (verticesCompletosU_0.size() <= maxContagemK) return std::make_pair(limiteB, verticesCompletosU_0);
    else {
        // Encontrando a distancia maxima em U_0
        double Blinha = -1; // não tem distancia negativa pelo artigo
        for (int vertice : verticesCompletosU_0)
            if (Blinha < distD[vertice]) Blinha = distD[vertice];

        // Construindo U para retorno(retirando vertices onde distancia < Blinha
        std::vector<int> U;
        for (int vertice : verticesCompletosU_0)
            if (Blinha > distD[vertice]) U.push_back(vertice);

        return std::make_pair(Blinha, U);
    }
}

// Bounded Multi-Source Shortest Path(BMSSP)
std::pair<double, std::vector<int>> bmssp(const Grafo& grafo, int nivel, double limiteSuperiorGlobalB, std::vector<int> fronteiraS) {
    if (nivel == 0) { // S é um singleton(único elemento)
        return baseCase(grafo, distD, limiteSuperiorGlobalB, fronteiraS[0], maxContagemK);
    }

    auto resultadoPivots = findPivots(grafo, distD, limiteSuperiorGlobalB, fronteiraS, maxContagemK);

    std::vector<int> pivotsP = resultadoPivots.first;
    std::vector<int> verticesAlcancadosW = resultadoPivots.second;

    size_t tamLoteM = std::pow(2, (nivel - 1) * passosT);

    D estruturaD(tamLoteM, limiteSuperiorGlobalB);

    double limiteInferiorAnterior;

    if (pivotsP.empty())
        limiteInferiorAnterior = limiteSuperiorGlobalB;
    else {
        double menor = distD[pivotsP[0]];
        for (int vertice : pivotsP) {
            double dist = distD[vertice];
            if (menor > dist) menor = dist;
        }

        limiteInferiorAnterior = menor;
    }

    for (int vertice : pivotsP) {
        estruturaD.insert(vertice, distD[vertice]);
    }

    std::vector<int> verticesResolvidosU;

    double Bfinal = limiteSuperiorGlobalB;
    double limite = maxContagemK * pow(2, nivel * passosT);

    while (verticesResolvidosU.size() < limite) {
        auto resultadoPull = estruturaD.pull();
        double limiteSuperiorLoteBi = resultadoPull.first;
        std::vector<ParDistVertice> paresExtraidosDoPull = resultadoPull.second;

        if (paresExtraidosDoPull.empty())
        {
            Bfinal = limiteSuperiorGlobalB;
            break;
        }

        std::vector<int> pivotsLoteAtual;
        for (const auto& par : paresExtraidosDoPull) {
            pivotsLoteAtual.push_back(par.second);
        }
        auto resultadoRecursivo = bmssp(grafo, nivel - 1, limiteSuperiorLoteBi, pivotsLoteAtual);
        double limiteAlcancadoRecursao = resultadoRecursivo.first;
        std::vector<int> verticesResolvidosLote = resultadoRecursivo.second;

        Bfinal = limiteAlcancadoRecursao;

        verticesResolvidosU.insert(verticesResolvidosU.end(), verticesResolvidosLote.begin(), verticesResolvidosLote.end());

        std::vector<ParDistVertice> loteTemporarioK;

        for (int verticeU : verticesResolvidosLote) {
            for (const auto& aresta : grafo[verticeU]) {
                int vizinhoV = aresta.first; // v
                int pesoUV = aresta.second; // peso u -> v

                double novoCusto = distD[verticeU] + pesoUV;

                if (novoCusto <= distD[vizinhoV]) {
                    distD[vizinhoV] = novoCusto;
                    if ((novoCusto >= limiteSuperiorLoteBi && novoCusto < limiteSuperiorGlobalB))
                        estruturaD.insert(vizinhoV, novoCusto);
                    else if (novoCusto >= limiteAlcancadoRecursao && novoCusto < limiteSuperiorLoteBi)
                        loteTemporarioK.push_back({ novoCusto, vizinhoV });
                }
            }
        }

        for (int vertice : pivotsLoteAtual) {
            if (distD[vertice] >= limiteAlcancadoRecursao && distD[vertice] < limiteSuperiorLoteBi)
                loteTemporarioK.push_back({ distD[vertice], vertice });
        }

        estruturaD.batchPrepend(loteTemporarioK);
        limiteInferiorAnterior = limiteAlcancadoRecursao;
    }
    for (int vertice : verticesAlcancadosW) {
        if (distD[vertice] < Bfinal) {
            verticesResolvidosU.push_back(vertice);
        }
    }
    return std::make_pair(Bfinal, verticesResolvidosU);
}