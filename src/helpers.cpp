#pragma once
#include "../headers/helpers.hpp"

#include "../lib/json.hpp"

#include <vector>
#include <random>
#include <fstream>
#include <iostream>

/**
 * SOLVER (ORÁCULO)
 * * Roda o Algoritmo de Bellman-Ford para encontrar a "resposta correta".
 * Este algoritmo é mais lento (O(N*E)), mas sua lógica simples
 * serve como uma "prova" para validar seu Dijkstra otimizado.
 */
std::pair<std::vector<double>, std::vector<size_t>> bellmanFord(const CaminhoMinimo::Grafo& grafo) {

    // (Pode ser 'int' ou 'size_t', desde que seja consistente)
    size_t tamanho = grafo.size();

    // 1. Inicialização (Idêntica ao Dijkstra)
    std::vector<double> minDistancia(tamanho, std::numeric_limits<double>::infinity());
    std::vector<size_t> predecessores(tamanho, std::numeric_limits<size_t>::max()); // -1 = nulo
    minDistancia[0] = 0; // Origem

    // Loop principal (roda N-1 vezes)
    for (size_t i = 1; i < tamanho; ++i) {
        // u = vértice de origem da aresta
        for (size_t u = 0; u < tamanho; ++u) {

            // v = vértice de destino da aresta
            for (const auto& parVizinho : grafo[u]) {
                size_t v = parVizinho.first;
                double peso = parVizinho.second;

                // Lógica de Relaxamento (O coração do algoritmo)

                // Se a origem 'u' ainda é inalcançável, 
                // não podemos relaxar a partir dela.
                if (minDistancia[u] == std::numeric_limits<double>::infinity()) continue;

                // Verificação de overflow (igual ao Dijkstra)
                if (minDistancia[u] > std::numeric_limits<double>::infinity() - peso) continue;

                double distanciaNova = minDistancia[u] + peso;

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

// Função que gera um grafo para teste usando lista de adjacencia.
CaminhoMinimo::Grafo geraGrafo(size_t tamanho, double densidade)
{
    // Criando um gerador de numeros aleatórios usando static para persistir durante várias calls a função
    static std::random_device semente;
    static std::mt19937 motor(semente());
    std::uniform_int_distribution<size_t> escolheVertice(0, tamanho - 1);
    std::uniform_real_distribution<double> escolhePeso(1.0, 100.0);

    CaminhoMinimo::Grafo grafo(tamanho);

    // Calculando o número de arestas do grafo com base na densidade
    size_t maxArestas = static_cast<size_t>(densidade * tamanho * (tamanho - 1));

    // Populando o grafo com pesos e direções aleatórias
    for (size_t i = 0; i < maxArestas; i++){
        size_t origem = escolheVertice(motor);
        size_t chegada = escolheVertice(motor);
        double peso = escolhePeso(motor);

        if (origem == chegada) continue;

        // --- A NOVA LÓGICA DE VERIFICAÇÃO ---
        bool arestaJaExiste = false;

        // 1. Itere sobre os vizinhos que já estão na lista de 'origem'
        for (const auto &parVizinho : grafo[origem]){
            if (parVizinho.first == chegada){
                // 2. Se o destino 'chegada' já está na lista, marque e pare
                arestaJaExiste = true;
                break;
            }
        }

        // 3. Só adicione a aresta se ela NÃO foi encontrada
        if (!arestaJaExiste) grafo[origem].emplace_back(chegada, peso);
    }
    return grafo;
}

// Função que coloca grafos em formato json em um arquivo txt
void salvaGrafo(size_t tamanho, double densidade, const CaminhoMinimo::Grafo &grafo, std::string path)
{
    nlohmann::json Grafos;

    // colocando os grafos existentes na memoria para atualizar o arquivo
    std::ifstream arquivoGrafosLeitura(path);
    if (!arquivoGrafosLeitura.is_open()) return;

    try{
        Grafos = nlohmann::json::parse(arquivoGrafosLeitura);
    }
    catch (const std::exception&){
        Grafos = nlohmann::json::array();
    }
    arquivoGrafosLeitura.close();

    // construindo o novo objeto json(grafo gerado)
    nlohmann::json novoGrafo;
    static int counter = 1;
    std::stringstream ss;
    ss << "Grafo_N" << tamanho << "D" << std::fixed << std::setprecision(2) << densidade << "_" << counter++;
    novoGrafo["id"] = ss.str();
    novoGrafo["parametros"]["tamanho"] = tamanho;
    novoGrafo["parametros"]["densidade"] = densidade;
    novoGrafo["grafo"] = grafo;

    auto solucao = bellmanFord(grafo);
    novoGrafo["solucao"]["distancias"] = solucao.first;     // dt
    novoGrafo["solucao"]["predecessores"] = solucao.second; // rot

    // atualizando lista de grafos
    Grafos.push_back(novoGrafo);

    // Colocando lista atualizada no arquivo
    std::ofstream arquivoGrafosEscrita(path);
    arquivoGrafosEscrita << Grafos.dump(4);
}

void imprimeArquivo(const std::string &caminhoArquivo){
    // 1. Abre o arquivo para leitura
    std::ifstream arquivoEntrada(caminhoArquivo);
    if (!arquivoEntrada.is_open()){
        std::cerr << "ERRO: Nao foi possivel abrir o arquivo: " << caminhoArquivo << std::endl;
        return;
    }

    // 2. Analisa (Parse) o JSON
    nlohmann::json listaDeTestes;
    try {
        listaDeTestes = nlohmann::json::parse(arquivoEntrada);
        if (!listaDeTestes.is_array()){
            std::cerr << "ERRO: O JSON nao e um array." << std::endl;
            return;
        }
    }
    catch (nlohmann::json::parse_error &e){
        std::cerr << "ERRO: Falha ao analisar o JSON. " << e.what() << std::endl;
        return;
    }
    arquivoEntrada.close();

    // 3. Itera sobre cada "teste" no array JSON
    for (const auto &teste : listaDeTestes){
        // Imprime o cabeçalho de cada grafo
        std::cout << "--- Grafo Gerado (Entrada) ID: " << teste["id"] << " ---" << std::endl;

        const auto &grafoJSON = teste["grafo"];

        // 4. Itera sobre cada VÉRTICE no grafoJSON (para imprimir o grafo)
        for (size_t i = 0; i < grafoJSON.size(); ++i){
            std::cout << "Vertice " << i << ": ";
            const auto &listaVizinhos = grafoJSON[i];

            if (listaVizinhos.empty()) std::cout << "(nenhuma aresta)";

            for (const auto &vizinhoPar : listaVizinhos){
                // vizinhoPar[0] = destino, vizinhoPar[1] = peso
                std::cout << "-> (" << vizinhoPar[0] << ", Peso: " << vizinhoPar[1] << ") ";
            }
            std::cout << std::endl;
        }
        std::cout << "--------------------------------------" << std::endl;

        // =======================================================
        // == 5. IMPRIME A SOLUÇÃO (A NOVA SEÇÃO) ==
        // =======================================================

        // (Baseado no seu último código 'salvaGrafo',
        // a chave é "solucao")
        const auto &solucao = teste["solucao"];
        std::cout << "Solucao:" << std::endl;

        // Verifica se a solucao está vazia (placeholder)
        if (solucao["distancias"].empty()) 
            std::cout << "(Solucao pendente ou nao calculada)" << std::endl;
        else {
            // Imprime a tabela de resultados
            std::cout << "Vertice | Distancia | Predecessor" << std::endl;
            std::cout << "-------------------------------------" << std::endl;

            const auto &distancias = solucao["distancias"];
            const auto &predecessores = solucao["predecessores"];

            // Assumimos que o tamanho dos arrays de solucao é o mesmo
            for (size_t i = 0; i < distancias.size(); ++i){
                std::cout << "   " << i << "    |     ";
                
                // CÓDIGO CORRIGIDO
                if (distancias[i].is_null()) {
                    std::cout << "INF";
                }
                else {
                    std::cout << distancias[i].get<double>(); // Use double, pois suas distâncias são double!
                }

                std::cout << "     |      ";

                // Verifica se é nulo no JSON (caso não tenha sido salvo ou seja inválido)
                if (predecessores[i].is_null()) {
                    std::cout << "-1";
                }
                else {
                    // Usa size_t pois é o tipo que você definiu na classe/solver
                    size_t pred = predecessores[i].get<size_t>();

                    // Verifica se é o valor Sentinela (o maior size_t possível)
                    if (pred == std::numeric_limits<size_t>::max()) {
                        std::cout << "NULO";
                    }
                    else {
                        std::cout << pred;
                    }
                }
                std::cout << std::endl;
            }
        }
        std::cout << "======================================" << std::endl << std::endl;
    }
}

