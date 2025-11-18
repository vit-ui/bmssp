#include <iostream>
#include "./dijsktra.cpp"
#include "./helpers.cpp"
int main() {
    int tamanho = 3;
    double densidade = 0.7;
    nlohmann::json Grafos;
    std::string caminhoGrafo = "../grafos/grafos.txt";
    std::ofstream cleaner(caminhoGrafo);
    cleaner.close();

    for (int i = 0; i < 10; i++) {
        if (i % 2 == 0) tamanho++;

        auto grafo = geraGrafo(tamanho, densidade);
        salvaGrafo(tamanho, densidade, grafo);
    }

    std::ifstream arquivoGrafosLeitura(caminhoGrafo);
    try {
        Grafos = nlohmann::json::parse(arquivoGrafosLeitura);
    }
    catch (const std::exception &e) {
        Grafos = nlohmann::json::array();
    }
    arquivoGrafosLeitura.close();

    for (auto &grafo : Grafos) {
        auto resposta = dijkstra(grafo["grafo"]);
        
        // 1. Imprime o cabeçalho de Distâncias
        std::cout << "DT: ";

        // 2. Itera sobre o PRIMEIRO vetor (distâncias)
        //    (Assumindo que INFINITO está acessível aqui)
        for (const auto &dist : resposta.first){
            if (dist == INFINITO) std::cout << "INF ";
            else std::cout << dist << " ";
        }

        // 3. Imprime o cabeçalho de Predecessores
        std::cout << "\nRot: ";

        // 4. Itera sobre o SEGUNDO vetor (predecessores)
        for (const auto &pred : resposta.second) std::cout << pred << " ";

        // 5. Finaliza a linha
        std::cout << std::endl;
    }
}