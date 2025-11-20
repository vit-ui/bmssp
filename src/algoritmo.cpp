#include "../headers/algoritmo.hpp"

#include <algorithm>
#include <cmath>
#include <chrono>

namespace CaminhoMinimo {
	long long Algoritmo::execDijkstra(size_t origem)
	{
		std::fill(distD.begin(), distD.end(), INFINITO);
		distD[origem] = 0.0;

		auto tempoInicial = std::chrono::high_resolution_clock::now();
		dijkstra(origem);
		auto duracao = std::chrono::high_resolution_clock::now() - tempoInicial;

		return std::chrono::duration_cast<std::chrono::microseconds>(duracao).count();
	}

	long long Algoritmo::execBmssp(size_t origem)
	{
		std::fill(distD.begin(), distD.end(), INFINITO);
		distD[origem] = 0.0;

		int nivelInicial = static_cast<int>(std::ceil(logN / passosT));

		auto tempoInicial = std::chrono::high_resolution_clock::now();
		bmssp(nivelInicial, INFINITO, { origem });
		auto duracao = std::chrono::high_resolution_clock::now() - tempoInicial;

		return std::chrono::duration_cast<std::chrono::microseconds>(duracao).count();
	}

	void Algoritmo::setGrafo(const Grafo& grafo)
	{
		ptrGrafo = &grafo;

		tamGrafo = ptrGrafo->size();
		logN = std::log2(tamGrafo);

		maxContagemK = static_cast<size_t>(std::max(std::floor(std::pow(logN, 1.0 / 3.0)), 1.0));
		passosT = static_cast<size_t>(std::max(std::floor(std::pow(logN, 2.0 / 3.0)), 1.0));

		distD.resize(tamGrafo);
	}
}

