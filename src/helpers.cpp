#pragma once
#include <vector>
#include <utility>
#include <random>
#include <climits>
#include <fstream>
#include <sstream>         // Para std::stringstream
#include <iomanip>         // Para std::setprecision e std::fixed
#include "../lib/json.hpp" // lib nlohmann/json
#include "./solver.cpp"


// Função que gera um grafo para teste usando lista de adjacencia.
std::vector<std::vector<std::pair<int, int>>> geraGrafo(int tamanho, double densidade)
{
    // Criando um gerador de numeros aleatórios usando static para persistir durante várias calls a função
    static std::random_device semente;
    static std::mt19937 motor(semente());
    std::uniform_int_distribution<int> escolheVertice(0, tamanho - 1);
    std::uniform_int_distribution<int> escolhePeso(1, 100);

    std::vector<std::vector<std::pair<int, int>>> grafo(tamanho);

    // Calculando o número de arestas do grafo com base na densidade
    size_t maxArestas = densidade * tamanho * (tamanho - 1);

    // Populando o grafo com pesos e direções aleatórias
    for (int i = 0; i < maxArestas; i++){
        int origem = escolheVertice(motor);
        int chegada = escolheVertice(motor);
        int peso = escolhePeso(motor);

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
void salvaGrafo(int tamanho, double densidade, const std::vector<std::vector<std::pair<int, int>>> &grafo)
{
    // Preparando variáveis para a operação no arquivo
    std::string caminhoGrafo = "../grafos/grafos.txt";
    nlohmann::json Grafos;

    // colocando os grafos existentes na memoria para atualizar o arquivo
    std::ifstream arquivoGrafosLeitura(caminhoGrafo);
    try{
        Grafos = nlohmann::json::parse(arquivoGrafosLeitura);
    }
    catch (const std::exception &e){
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
    std::ofstream arquivoGrafosEscrita(caminhoGrafo);
    arquivoGrafosEscrita << Grafos.dump(4);
}

// minDist e vizinhos não são mais utilizadas.
int minDist(const std::vector<int> &minDistancia, const std::vector<bool> &foiFechado){
    int indiceVerticeMinimo = -1;
    int distanciaMinimaAtual = INFINITO;

    for (int i = 0; i < minDistancia.size(); i++){
        if (!foiFechado[i] && minDistancia[i] < distanciaMinimaAtual){
            distanciaMinimaAtual = minDistancia[i];
            indiceVerticeMinimo = i;
        }
    }
    return indiceVerticeMinimo;
}

std::vector<int> vizinhos(const std::vector<std::vector<int>> &grafo, const std::vector<bool> &foiFechado, int verticeAtual){
    std::vector<int> vizinhos;
    for (int i = 0; i < grafo.size(); i++){
        if (!foiFechado[i] && grafo[verticeAtual][i] != INFINITO)
            vizinhos.emplace_back(i);
    }
    return vizinhos;
}

// Apelido para a biblioteca JSON
using json = nlohmann::json;

void imprimeArquivo(const std::string &caminhoArquivo){
    // 1. Abre o arquivo para leitura
    std::ifstream arquivoEntrada(caminhoArquivo);
    if (!arquivoEntrada.is_open()){
        std::cerr << "ERRO: Nao foi possivel abrir o arquivo: " << caminhoArquivo << std::endl;
        return;
    }

    // 2. Analisa (Parse) o JSON
    json listaDeTestes;
    try {
        listaDeTestes = json::parse(arquivoEntrada);
        if (!listaDeTestes.is_array()){
            std::cerr << "ERRO: O JSON nao e um array." << std::endl;
            return;
        }
    }
    catch (json::parse_error &e){
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

                // Tenta pegar o valor 'int' da distância
                // (Usando .get<int>() para converter do JSON)
                int dist = distancias[i].get<int>();

                if (dist == INFINITO) std::cout << "INF";
                else std::cout << dist;

                std::cout << "     |      ";

                // Tenta pegar o valor 'int' do predecessor
                std::cout << predecessores[i].get<int>();

                std::cout << std::endl;
            }
        }
        std::cout << "======================================" << std::endl << std::endl;
    }
}