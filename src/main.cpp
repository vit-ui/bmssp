#include "../headers/algoritmo.hpp"
#include "../headers/helpers.hpp"
#include "../lib/json.hpp"

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <iomanip> // Necessário para formatar a densidade no nome do arquivo se desejar precisão

// Estrutura para guardar informações sobre os erros encontrados (mantida para verificação)
struct InfoErro {
    int idTeste;
    int tamanhoGrafo;
    size_t verticeDivergente;
    double valorDijkstra;
    double valorBMSSP;
};

int main() {
    // Parâmetros iniciais
    int tamanho = 100;
    int origem = 0;
    double densidade = 0.5;
    int quantidade = 5000;

    int stepMudanca = 200;
    int stepTamanho = 50;

    // --- CONFIGURAÇÃO DO CSV ---
    // Cria o nome do arquivo: "densidade_0.200000.csv"
    std::string nomeArquivoCsv = "densidade_" + std::to_string(densidade) + ".csv";

    // Abre o arquivo para escrita
    std::ofstream arquivoCsv(nomeArquivoCsv);

    // Escreve o cabeçalho das colunas
    arquivoCsv << "N_Teste,N_Vertices,Tempo_Dijkstra_micros,Tempo_BMSSP_micros\n";

    // Feedback visual para saber que iniciou
    std::cout << "Iniciando Benchmark..." << std::endl;
    std::cout << "Gerando saida em: " << nomeArquivoCsv << std::endl;

    std::vector<InfoErro> errosEncontrados;
    CaminhoMinimo::Algoritmo algos; // Instancia o Solucionador

    for (int i = 0; i < quantidade; i++) {
        // Aumenta o tamanho do grafo a cada stepMudanca iterações
        if (i > 0 && i % stepMudanca == 0) tamanho += stepTamanho;

        // Feedback de progresso no console (opcional, para não parecer travado)
        if (i % 100 == 0) std::cout << "Processando teste " << i << "/" << quantidade << " (N=" << tamanho << ")..." << std::endl;

        // 1. GERAÇÃO DO GRAFO
        auto grafo = geraGrafo(tamanho, densidade);
        algos.setGrafo(grafo);

        // 2. EXECUÇÃO DO BMSSP
        long long tempoBMSSP = algos.execBmssp(origem);
        const std::vector<double>& resultadoBMSSP = algos.getDist();

        // 3. EXECUÇÃO DO DIJKSTRA
        long long tempoDijkstra = algos.execDijkstra(origem);
        std::vector<double> resultadoDijkstra = algos.getDist();

        // 4. ESCRITA NO CSV (Teste, Vertices, Dijkstra, BMSSP)
        arquivoCsv << i << "," << tamanho << "," << tempoDijkstra << "," << tempoBMSSP << "\n";

        // 5. VALIDAÇÃO (Mantida a lógica de erro, mas sem spammar o console)
        if (resultadoDijkstra.size() != resultadoBMSSP.size()) {
            std::cerr << "[ERRO CRITICO] Tamanhos diferentes no teste " << i << std::endl;
            continue;
        }

        for (size_t v = 0; v < resultadoDijkstra.size(); ++v) {
            // Verifica divergência
            if (resultadoDijkstra[v] != resultadoBMSSP[v]) {
                errosEncontrados.push_back({ i, tamanho, v, resultadoDijkstra[v], resultadoBMSSP[v] });
                // Mensagem de erro no console é importante manter
                std::cerr << "[DIVERGENCIA] Teste " << i << " Vertice " << v << std::endl;
                break;
            }
        }
    }

    // Fecha o arquivo CSV ao final
    arquivoCsv.close();

    std::cout << "\nBenchmark finalizado." << std::endl;

    // Relatório final de erros no console
    if (!errosEncontrados.empty()) {
        std::cout << "Foram encontradas " << errosEncontrados.size() << " divergencias." << std::endl;
    }
    else {
        std::cout << "Sucesso! Nenhuma divergencia encontrada entre os algoritmos." << std::endl;
    }

    return 0;
}