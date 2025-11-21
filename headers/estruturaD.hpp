#pragma once

#define VERSAO_MODERNAGEMINI
#ifdef VERSAO_MODERNACHAT

#include <vector>
#include <list>
#include <map>
#include <utility> // Para std::pair
#include <limits>
#include <unordered_map>

using ParDistVertice = std::pair<double, size_t>; // (distancia, vertice)
using Bloco = std::list<ParDistVertice>;

class D {
public:
	D(size_t M, double B); // Initialize(M,B)

	void insert(size_t vertice, double distancia);
	void batchPrepend(std::vector<ParDistVertice>& loteL);
	std::pair<double, std::vector<ParDistVertice>> pull();

private:
	void removeChave(size_t vertice);
	void dividir(std::map<double, std::list<Bloco>::iterator>::iterator& iLimites);

private:
	size_t tamLoteM;
	double limiteSuperiorB;
	std::list<Bloco> blocosD_0;
	std::list<Bloco> blocosD_1;

	// mapa: upper -> iterator do bloco em blocosD_1
	std::map<double, std::list<Bloco>::iterator> limites;

	// status: vertice -> distancia (presença + valor atual)
	std::map<size_t, double> status;

	// ===== mudanças necessárias =====
	// índice para localizar rapidamente onde cada vértice está:
	// vertice -> (iterador do bloco na lista correspondente, iterador do elemento dentro do bloco)
	std::unordered_map<size_t, std::pair<std::list<Bloco>::iterator, Bloco::iterator>> indice;

	// para cada bloco de D1 guardamos o upper atual usado no 'limites'
	// chave = endereço do Bloco (Bloco*) -> upper
	std::unordered_map<Bloco*, double> upperOfBlock;
};


#else

#include <vector>
#include <list>
#include <map>
#include <utility> // Para std::pair
#include <limits>

using ParDistVertice = std::pair<double, size_t>; // (distancia, vertice)
using Bloco = std::list<ParDistVertice>; 

class D {
public:
	D(size_t M, double B); // Initialize(M,B)

	void insert(size_t vertice, double distancia);
	void batchPrepend(std::vector<ParDistVertice> &loteL);
	std::pair<double, std::vector<ParDistVertice>> pull();

private:
	void removeChave(size_t vertice);
	void dividir(std::map<double, std::list<Bloco>::iterator>::iterator &iLimites);
private:
	size_t tamLoteM;
	double limiteSuperiorB;
	std::list<Bloco> blocosD_0;
	std::list<Bloco> blocosD_1;

	std::map<double, std::list<Bloco>::iterator> limites;
	std::map<size_t, double> status;
};
#endif