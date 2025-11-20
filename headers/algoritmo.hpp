#pragma once

#include <vector>
#include <queue>
#include <cmath>
#include <limits>

// namespace feito para separar os algoritmos dos testes
namespace CaminhoMinimo {
	using Grafo = std::vector<std::vector<std::pair<size_t, double>>>;
	using FilaPrioridade = std::priority_queue<std::pair<double, size_t>, std::vector<std::pair<double, size_t>>, std::greater<std::pair<double, size_t>>>;

	class Algoritmo {
	public:
		Algoritmo() : ptrGrafo(nullptr) {} // as variaveis são inicializadas em setGrafo já que elas dependem do tamanho do grafo.

		long long execDijkstra(size_t origem);
		long long execBmssp(size_t origem);

		std::vector<double> getDist() { return distD; }
		void setGrafo(const Grafo& grafo);

		static constexpr double INFINITO = std::numeric_limits<double>::infinity();
		static constexpr size_t NULO = std::numeric_limits<size_t>::max();
	private:
		std::pair<std::vector<size_t>, std::vector<size_t>> findPivots(double limiteB, std::vector<size_t> fronteiraInicialS);
		std::pair<double, std::vector<size_t>> baseCase(double limiteB, size_t pivoFonteS);

		std::vector<size_t> dijkstra(size_t origem);
		std::pair<double, std::vector<size_t>> bmssp(int nivel, double limiteSuperiorGlobalB, std::vector<size_t> fronteiraS);

	private:
		const Grafo *ptrGrafo;
		std::vector<double> distD;
		size_t maxContagemK, passosT, tamGrafo;
		double logN;
	};
}

