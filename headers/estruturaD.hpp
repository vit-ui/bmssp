#pragma once

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