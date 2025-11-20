#include "../headers/algoritmo.hpp"
#include "../headers/helpers.hpp"

#include "../lib/json.hpp"

#include <iostream>
#include <fstream>
#include <vector>
#include <string>

int main() {
    // Parâmetros iniciais
    int tamanho = 3;   // Começando pequeno para testar
    int origem = 0;     // Vértice de partida
    double densidade = 0.5;
    int quantidade = 10;

    // a cada stepMudanca Grafos, o tamanho aumenta em stepTamanho
    int stepMudanca = 2;
    int stepTamanho = 1;
    std::string caminhoGrafo = "grafos.txt";;

    // Instancia o Solucionador
    // (Requer construtor vazio: Algoritmo() : ptrGrafo(nullptr) {})
    CaminhoMinimo::Algoritmo algos;

    // Limpa o arquivo de log antes de começar (opcional)
    std::ofstream cleaner(caminhoGrafo);
    cleaner.close();

    std::cout << "========================================" << std::endl;
    std::cout << "       BENCHMARK: DIJKSTRA vs BMSSP     " << std::endl;
    std::cout << "========================================" << std::endl;

    for (int i = 0; i < quantidade; i++) {
        // Aumenta o tamanho do grafo a cada iteração par
        if (i % stepMudanca == 0) tamanho += stepTamanho;

        std::cout << "\n--- Teste " << i + 1 << " (Vertices: " << tamanho << ") ---" << std::endl;

        // 1. GERAÇÃO DO GRAFO
        // Gera em memória (vetor de vetores com peso double)
        CaminhoMinimo::Grafo grafo = geraGrafo(tamanho, densidade);

        // 2. REGISTRO (Opcional)
        // Salva no arquivo para conferência futura
        salvaGrafo(tamanho, densidade, grafo, caminhoGrafo);

        // 3. CONFIGURAÇÃO
        // Passa o ponteiro do grafo atual para a classe
        algos.setGrafo(grafo);

        // -------------------------------------------------
        // 4. EXECUÇÃO DO DIJKSTRA (Baseline)
        // -------------------------------------------------
        long long tempoDijkstra = algos.execDijkstra(origem);

        // salva o resultado para comparar depois
        std::vector<double> resultadoDijkstra = algos.getDist();

        std::cout << "Tempo Dijkstra: " << tempoDijkstra << " micros" << std::endl;

        // -------------------------------------------------
        // 5. EXECUÇÃO DO BMSSP (Algoritmo Novo)
        // -------------------------------------------------
        long long tempoBMSSP = algos.execBmssp(origem);

        // Pega referência do resultado (leitura)
        const std::vector<double>& resultadoBMSSP = algos.getDist();

        std::cout << "Tempo BMSSP:    " << tempoBMSSP << " micros" << std::endl;

        // -------------------------------------------------
        // 6. VALIDAÇÃO E COMPARAÇÃO
        // -------------------------------------------------

        // Verifica se os tamanhos batem (segurança básica)
        if (resultadoDijkstra.size() != resultadoBMSSP.size()) {
            std::cout << "[ERRO CRITICO] Vetores de tamanhos diferentes!" << std::endl;
            continue;
        }

        bool iguais = true;
        // Compara valor a valor (usando margem de erro pequena para double se necessário, 
        // mas == costuma funcionar se a aritmética for determinística)
        for (size_t v = 0; v < resultadoDijkstra.size(); ++v) {
            if (resultadoDijkstra[v] != resultadoBMSSP[v]) {
                iguais = false;
                // Imprime o primeiro erro encontrado para debug
                std::cout << "[DIVERGENCIA] Vertice " << v
                    << " -> Dijk: " << resultadoDijkstra[v]
                    << " | BMSSP: " << resultadoBMSSP[v] << std::endl;
                break;
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

    imprimeArquivo(caminhoGrafo);
}
// Main antigo
//int main() {
//    int tamanho = 3, origem = 0;
//    double densidade = 0.7;
//    std::string caminhoGrafo = "../grafos/grafos.txt";
//
//    CaminhoMinimo::Algoritmo algos;
//    //// limpando o arquivo
//    //std::ofstream cleaner(caminhoGrafo);
//    //cleaner.close();
//
//    // construindo, salvando e testando os grafos
//    for (int i = 0; i < 10; i++) {
//        if (i % 2 == 0) tamanho++;
//
//        auto grafo = geraGrafo(tamanho, densidade);
//        salvaGrafo(tamanho, densidade, grafo, caminhoGrafo); // Apenas para registro.
//
//        algos.setGrafo(grafo);
//        long long tempoDijkstra = algos.execDijkstra(origem);
//        std::vector<double> dist = algos.getDist();
//
//        std::cout << "DT: ";
//        for (const auto& custo : dist) {
//            if (custo == CaminhoMinimo::Algoritmo::INFINITO) std::cout << "INF ";
//            else std::cout << custo << " ";
//        }
//        std::cout << "Tempo gasto: " << tempoDijkstra << "microsegundos";
//
//        std::cout << std::endl;
//    }
//}