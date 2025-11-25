#pragma once
//#define O1
//#define RANGE

#include <vector>
#include <list>
#include <map>
#include <unordered_map>
#include <utility> // Para std::pair
#include <limits>

using ParDistVertice = std::pair<double, size_t>; // (distancia, vertice)
using Bloco = std::list<ParDistVertice>; 

#ifdef O1
struct Status {
	double distancia;
	std::list<ParDistVertice>::iterator iElem;
	std::list<Bloco>::iterator iBloco;
	bool pertenceD1;
};
#endif

class D {
public:
	D(size_t M, double B); // Initialize(M,B)

	void insert(size_t vertice, double distancia);
	void batchPrepend(std::vector<ParDistVertice> &loteL);
	std::pair<double, std::vector<ParDistVertice>> pull();

private:
	void removeChave(size_t vertice);
	void dividir(std::map<double, std::list<Bloco>::iterator>::iterator &iLimites);
	void dividirLote(std::vector<ParDistVertice>& lotes, size_t inicio, size_t fim);
private:
	size_t tamLoteM;
	double limiteSuperiorB;
	std::list<Bloco> blocosD_0;
	std::list<Bloco> blocosD_1;

	std::map<double, std::list<Bloco>::iterator> limites;
#ifdef O1
	std::unordered_map<size_t, Status> status;
#else
	std::unordered_map<size_t, double> status;
#endif
};