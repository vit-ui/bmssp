#include "src/estruturaD.h"
#include <algorithm>

D::D(size_t M, double B) : tamLoteM(M), limiteSuperiorB(B) {
	blocosD_1.push_back(Bloco());

	auto& i = blocosD_1.begin();
	limites[B] = i;
}

void D::insert(int vertice, double distancia) {
	auto iStatus = status.find(vertice);

	if (iStatus != status.end()) {
		if (iStatus->second > distancia) removeChave(vertice);
		else return;
	}

	auto& iLimites = limites.lower_bound(distancia);
	auto& iBloco = iLimites->second;
	iBloco->push_back({ distancia, vertice });

	status[vertice] = distancia;

	if (iBloco->size() > tamLoteM) dividir(iLimites);
}

void D::batchPrepend(std::vector<ParDistVertice>& loteL) {
	std::map<int, double> loteFiltrado;

	for (auto& par : loteL) {
		double distancia = par.first;
		int vertice = par.second;

		auto iLote = loteFiltrado.find(vertice);
		if (iLote == loteFiltrado.end()) {
			loteFiltrado[vertice] = distancia;
		}
		else if (distancia < loteFiltrado[vertice]) {
			loteFiltrado[vertice] = distancia;
		}
	}

	for (auto& par : loteFiltrado) {
		double distancia = par.second;
		int vertice = par.first;

		auto iStatus = status.find(vertice);
		if (iStatus != status.end()) {
			double distanciaAntiga = iStatus->second;

			if (distancia >= distanciaAntiga) loteFiltrado.erase(vertice);
		}
	}

	if (loteFiltrado.size() <= tamLoteM) {
		std::list<ParDistVertice> bloco;
		for (auto& par : loteFiltrado) {
			double distancia = par.second;
			int vertice = par.first;

			bloco.push_back({ distancia, vertice });
		}
		blocosD_0.push_front(bloco);
	}
	else {
		std::vector<ParDistVertice> aux;
		for (auto& par : loteFiltrado) {
			double distancia = par.second;
			int vertice = par.first;

			aux.push_back({ distancia, vertice });
		}

		for (size_t i = 0; i < aux.size(); i += tamLoteM) {
			std::list<ParDistVertice> bloco;
			size_t fim = std::min(i + tamLoteM, aux.size());

			for (size_t j = i; j < fim; j++) {
				double distancia = aux[j].first;
				int vertice = aux[j].second;

				bloco.push_back({ distancia, vertice });
			}
			blocosD_0.push_front(bloco);
		}
	}

	for (auto& par : loteFiltrado) {
		double distancia = par.second;
		int vertice = par.first;

		status[vertice] = distancia;
	}
}

std::pair<double, std::vector<ParDistVertice>> D::pull(){
	std::vector<ParDistVertice> candidatosD_0;
	std::vector<ParDistVertice> candidatosD_1;
	std::vector<ParDistVertice> candidatosTotais;

	for (auto& bloco : blocosD_0) {
		for (auto &par : bloco) {
			candidatosD_0.push_back(par);
			if (candidatosD_0.size() >= tamLoteM) break;
		}
		if (candidatosD_0.size() >= tamLoteM) break;
	}

	for (auto& bloco : limites) {
		for (auto& par : *(bloco.second)) {
			candidatosD_1.push_back(par);
			if (candidatosD_1.size() >= tamLoteM) break;
		}
		if (candidatosD_1.size() >= tamLoteM) break;
	}

	candidatosTotais.insert(candidatosTotais.end(), candidatosD_0.begin(), candidatosD_0.end());
	candidatosTotais.insert(candidatosTotais.end(), candidatosD_1.begin(), candidatosD_1.end());

	if (candidatosTotais.size() <= tamLoteM) {
		blocosD_0.clear();
		blocosD_1.clear();
		limites.clear();
		status.clear();

		blocosD_1.push_back(Bloco());

		auto& i = blocosD_1.begin();
		limites[limiteSuperiorB] = i;

		return std::make_pair(limiteSuperiorB, candidatosTotais);
	}
	else {
		std::nth_element(candidatosTotais.begin(), candidatosTotais.begin() + tamLoteM, candidatosTotais.end());
		double novoLimiteBi = candidatosTotais[tamLoteM].first;
		std::vector<ParDistVertice> loteDeRetornoSi;

		for (size_t i = 0; i < tamLoteM; i++) {
			loteDeRetornoSi.push_back(candidatosTotais[i]);
		}

		for (auto& par : loteDeRetornoSi) {
			removeChave(par.second);
		}

		return std::make_pair(novoLimiteBi, loteDeRetornoSi);
	}
}

void D::removeChave(int vertice) {
	auto iStatus = status.find(vertice);
	if (iStatus == status.end()) return;

	double distancia = iStatus->second;
	auto& iLimites = limites.lower_bound(distancia);
	auto& iBloco = iLimites ->second;

	for (auto& valor = iBloco->begin(); valor != iBloco->end(); valor++) {
		if (valor->second == vertice) {
			iBloco->erase(valor);
			break;
		}
	}

	status.erase(iStatus);

	if(iBloco->empty()) {
		limites.erase(iLimites);
		blocosD_1.erase(iBloco);
	}
}

void D::dividir(std::map<double, std::list<Bloco>::iterator>::iterator &iLimites) {
	double limiteAntigo = iLimites->first;
	auto& iBloco = iLimites->second;
	std::vector<ParDistVertice> aux(*iBloco);

	std::nth_element(aux.begin(), aux.begin() + tamLoteM / 2, aux.end());
	double valorMediana = aux[tamLoteM / 2].first;

	Bloco bloco1;
	Bloco bloco2;

	for (ParDistVertice par : aux) {
		if (par.first <= valorMediana) bloco1.push_back(par);
		else bloco2.push_back(par);
	}
	*iBloco = bloco1;
	auto iBloco2 = blocosD_1.insert(std::next(iBloco), bloco2);

	limites.erase(iLimites);
	limites[bloco1.back().first] = iBloco;
	limites[limiteAntigo] = iBloco2;
}
