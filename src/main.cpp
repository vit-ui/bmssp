#include "../headers/algoritmo.hpp"
#include "../headers/helpers.hpp"
#include "../lib/json.hpp"

#include <iostream>
#include <fstream>
#include <vector>
#include <string>

// Estrutura para guardar informações sobre os erros encontrados
struct InfoErro {
    int idTeste;
    int tamanhoGrafo;
    size_t verticeDivergente;
    double valorDijkstra;
    double valorBMSSP;
};

int main() {
    // Parâmetros iniciais
    int tamanho = 3;   // Começando pequeno para testar
    int origem = 0;     // Vértice de partida
    double densidade = 0.5;
    int quantidade = 100; // Numero de grafos criados e testados

    // a cada stepMudanca Grafos, o tamanho aumenta em stepTamanho
    int stepMudanca = 5;
    int stepTamanho = 2;
    std::string caminhoGrafo = "grafos.txt";;

    // Instancia o Solucionador
    // (Requer construtor vazio: Algoritmo() : ptrGrafo(nullptr) {})
    CaminhoMinimo::Algoritmo algos;

    // Limpa o arquivo de log antes de começar (opcional)
    std::ofstream cleaner(caminhoGrafo);
    cleaner.close();

    // Vetor para acumular os erros
    std::vector<InfoErro> errosEncontrados;

    std::cout << "========================================" << std::endl;
    std::cout << "       BENCHMARK: DIJKSTRA vs BMSSP     " << std::endl;
    std::cout << "========================================" << std::endl;

    for (int i = 0; i < quantidade; i++) {
        // Aumenta o tamanho do grafo a cada stepMudanca iterações
        if (i % stepMudanca == 0) tamanho += stepTamanho;

        std::cout << "\n--- Teste " << i << " (Vertices: " << tamanho << ") ---" << std::endl;

        // 1. GERAÇÃO DO GRAFO
        auto grafo = geraGrafo(tamanho, densidade);

        // 2. REGISTRO (Opcional)
        salvaGrafo(tamanho, densidade, grafo, caminhoGrafo);

        // 3. CONFIGURAÇÃO
        algos.setGrafo(grafo);

        // -------------------------------------------------
        // 4. EXECUÇÃO DO DIJKSTRA (Baseline)
        // -------------------------------------------------
        long long tempoDijkstra = algos.execDijkstra(origem);
        std::vector<double> resultadoDijkstra = algos.getDist();

        std::cout << "Tempo Dijkstra: " << tempoDijkstra << " micros" << std::endl;

        // -------------------------------------------------
        // 5. EXECUÇÃO DO BMSSP (Algoritmo Novo)
        // -------------------------------------------------
        long long tempoBMSSP = algos.execBmssp(origem);
        const std::vector<double>& resultadoBMSSP = algos.getDist();

        std::cout << "Tempo BMSSP:    " << tempoBMSSP << " micros" << std::endl;

        // -------------------------------------------------
        // 6. VALIDAÇÃO E COMPARAÇÃO
        // -------------------------------------------------

        if (resultadoDijkstra.size() != resultadoBMSSP.size()) {
            std::cout << "[ERRO CRITICO] Vetores de tamanhos diferentes!" << std::endl;
            continue;
        }

        bool iguais = true;
        // Compara valor a valor (usando margem de erro pequena para double se necessário)
        for (size_t v = 0; v < resultadoDijkstra.size(); ++v) {
            // Usando comparação com precisão de double
            // Se quiser usar epsilon: std::abs(a - b) > 1e-9
            // Se usar a limpeza de ruído na classe, == deve funcionar na maioria dos casos simples
            if (resultadoDijkstra[v] != resultadoBMSSP[v]) {
                iguais = false;

                // Imprime o erro na hora
                std::cout << "[DIVERGENCIA] Vertice " << v
                    << " -> Dijk: " << resultadoDijkstra[v]
                    << " | BMSSP: " << resultadoBMSSP[v] << std::endl;

                // GUARDA O ERRO PARA O RELATÓRIO FINAL
                errosEncontrados.push_back({ i, tamanho, v, resultadoDijkstra[v], resultadoBMSSP[v] });

                break; // Para na primeira divergência encontrada neste grafo
            }
        }

        if (iguais) {
            std::cout << "[SUCESSO] Resultados identicos." << std::endl;

            // Comparação de Performance
            if (tempoBMSSP < tempoDijkstra)
                std::cout << ">> BMSSP foi mais rapido!" << std::endl;
            else
                std::cout << ">> Dijkstra foi mais rapido." << std::endl;
        }
        else {
            std::cout << "[FALHA] Os algoritmos deram resultados diferentes." << std::endl;
        }

        // (Opcional) Imprime distâncias para grafos pequenos
        if (tamanho <= 20) {
            std::cout << "Distancias(Dijks): ";
            for (const auto& d : resultadoDijkstra) {
                if (d == CaminhoMinimo::Algoritmo::INFINITO) std::cout << "INF ";
                else std::cout << d << " ";
            }
            std::cout << std::endl;

            std::cout << "Distancias(BMSSP): ";
            for (const auto& d : resultadoBMSSP) {
                if (d == CaminhoMinimo::Algoritmo::INFINITO) std::cout << "INF ";
                else std::cout << d << " ";
            }
            std::cout << std::endl;
        }
    }

    // --- RELATÓRIO FINAL DE ERROS ---
    std::cout << "\n\n========================================" << std::endl;
    std::cout << "       RESUMO DAS DIVERGENCIAS          " << std::endl;
    std::cout << "========================================" << std::endl;

    if (errosEncontrados.empty()) {
        std::cout << "PARABENS! Todos os " << quantidade << " testes passaram sem divergencias." << std::endl;
    }
    else {
        std::cout << "Foram encontradas " << errosEncontrados.size() << " divergencias:\n" << std::endl;
        for (const auto& erro : errosEncontrados) {
            std::cout << "--- Teste " << erro.idTeste << " (N=" << erro.tamanhoGrafo << ") ---" << std::endl;
            std::cout << "[DIVERGENCIA] Vertice " << erro.verticeDivergente
                << " -> Dijk: " << erro.valorDijkstra
                << " | BMSSP: " << erro.valorBMSSP << std::endl;
            std::cout << "----------------------------------------" << std::endl;
        }
    }

    //imprimeArquivo(caminhoGrafo);
}