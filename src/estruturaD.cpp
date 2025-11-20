#include "../headers/estruturaD.hpp"

#include <algorithm>
#include <vector>

// Construtor
D::D(size_t M, double B) : tamLoteM(M), limiteSuperiorB(B) {
    // Inicializa D_1 com um bloco vazio e o limite B
    blocosD_1.push_back(Bloco());
    const auto& i = blocosD_1.begin();
    limites[B] = i;
}

// INSERT: Insere um único elemento
void D::insert(size_t vertice, double distancia) {
    auto iStatus = status.find(vertice);

    if (iStatus != status.end()) {
        // Se nova distância é pior ou igual, ignora
        if (iStatus->second <= distancia) return;

        // Se é melhor, remove a antiga antes de inserir a nova
        removeChave(vertice);
    }

    // Encontra o bloco correto em D_1
    auto iLimites = limites.lower_bound(distancia);

    // Segurança para erros numéricos
    if (iLimites == limites.end()) {
        if (!limites.empty()) iLimites = std::prev(limites.end());
        else return;
    }

    auto iBloco = iLimites->second;
    iBloco->push_back({ distancia, vertice });

    status[vertice] = distancia;

    // Se o bloco estourou o tamanho M, divide ele
    if (iBloco->size() > tamLoteM) dividir(iLimites);
}

// BATCH PREPEND: Insere lote urgente (CORRIGIDO: push_front e Loop Reverso)
void D::batchPrepend(std::vector<ParDistVertice>& loteL) {
    std::map<size_t, double> loteFiltrado;
    std::vector<size_t> paraRemoverDoStatus;

    // 1. Filtra o melhor dentro do lote (resolve duplicatas no input)
    for (auto& par : loteL) {
        double distancia = par.first;
        size_t vertice = par.second;

        auto iLote = loteFiltrado.find(vertice);
        if (iLote == loteFiltrado.end()) {
            loteFiltrado[vertice] = distancia;
        }
        else if (distancia < loteFiltrado[vertice]) {
            loteFiltrado[vertice] = distancia;
        }
    }

    // 2. Compara com status global e marca remoções
    auto it = loteFiltrado.begin();
    while (it != loteFiltrado.end()) {
        size_t vertice = it->first;
        double distancia = it->second;

        auto iStatus = status.find(vertice);
        if (iStatus != status.end()) {
            double distanciaAntiga = iStatus->second;
            // Se o caminho novo for pior ou igual, descartamos do lote
            if (distancia >= distanciaAntiga) {
                it = loteFiltrado.erase(it);
                continue;
            }
            else {
                // Se o caminho novo for melhor, removemos o antigo da estrutura D
                paraRemoverDoStatus.push_back(vertice);
            }
        }
        ++it;
    }

    // Remove as versões antigas de D (garante consistência)
    for (size_t v : paraRemoverDoStatus) {
        removeChave(v);
    }

    // 3. Copia para vetor e ORDENA (Fundamental para dividir em blocos corretos)
    std::vector<ParDistVertice> aux;
    aux.reserve(loteFiltrado.size());
    for (auto& par : loteFiltrado) {
        aux.push_back({ par.second, par.first });
        status[par.first] = par.second; // Atualiza status global
    }
    std::sort(aux.begin(), aux.end()); // Menores distâncias primeiro

    // 4. Insere em D_0 (CORREÇÃO CRÍTICA AQUI)
    // Usamos push_front para que este lote urgente seja processado antes dos antigos.

    if (aux.size() <= tamLoteM) {
        std::list<ParDistVertice> bloco;
        // Copia o vetor para a lista
        for (const auto& p : aux) bloco.push_back(p);

        // Insere na FRENTE da lista D_0
        blocosD_0.push_front(bloco);
    }
    else {
        // Se o lote é maior que M, quebramos em vários blocos.
        // Como usamos push_front, precisamos inserir do ÚLTIMO bloco para o PRIMEIRO.
        // Ex: [Bloco Menor, Bloco Médio, Bloco Maior]
        // 1. Insere Bloco Maior -> D0: [Maior]
        // 2. Insere Bloco Médio -> D0: [Médio, Maior]
        // 3. Insere Bloco Menor -> D0: [Menor, Médio, Maior] (Ordem Correta!)

        // Cálculo do índice de início do último bloco
        size_t resto = aux.size() % tamLoteM;
        size_t inicioUltimo = (resto == 0) ? (aux.size() - tamLoteM) : (aux.size() - resto);

        // Iteramos de trás para frente
        for (long long i = inicioUltimo; i >= 0; i -= tamLoteM) {
            std::list<ParDistVertice> bloco;
            size_t fim = std::min((size_t)i + tamLoteM, aux.size());

            for (size_t j = (size_t)i; j < fim; j++) {
                bloco.push_back(aux[j]);
            }
            blocosD_0.push_front(bloco);
        }
    }
}

// PULL: Extrai M menores (CORRIGIDO: OFF-BY-ONE)
std::pair<double, std::vector<ParDistVertice>> D::pull() {
    std::vector<ParDistVertice> candidatosD_0;
    std::vector<ParDistVertice> candidatosD_1;
    std::vector<ParDistVertice> candidatosTotais;

    // Coleta de D_0
    for (auto& bloco : blocosD_0) {
        for (auto& par : bloco) {
            candidatosD_0.push_back(par);
            if (candidatosD_0.size() >= tamLoteM) break;
        }
        if (candidatosD_0.size() >= tamLoteM) break;
    }

    // Coleta de D_1
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
        const auto& i = blocosD_1.begin();
        limites[limiteSuperiorB] = i;

        return std::make_pair(limiteSuperiorB, candidatosTotais);
    }
    else {
        // Particiona para encontrar os M menores
        std::nth_element(candidatosTotais.begin(), candidatosTotais.begin() + tamLoteM, candidatosTotais.end());

        // O limite é o primeiro elemento que FICOU DE FORA (index M)
        double novoLimiteBi = candidatosTotais[tamLoteM].first;

        // Correção de Borda: Se houver empates no limite, precisamos garantir
        // que Bi seja > que o maior elemento de Si para a recursão funcionar estritamente.
        // (Simplificação: pegamos o max de Si)
        double maxSi = candidatosTotais[0].first;
        for (size_t k = 1; k < tamLoteM; ++k) {
            if (candidatosTotais[k].first > maxSi) maxSi = candidatosTotais[k].first;
        }
        if (novoLimiteBi <= maxSi) novoLimiteBi = maxSi + 1e-9;

        std::vector<ParDistVertice> loteDeRetornoSi;
        loteDeRetornoSi.reserve(tamLoteM);

        for (size_t i = 0; i < tamLoteM; i++) {
            loteDeRetornoSi.push_back(candidatosTotais[i]);
        }

        for (auto& par : loteDeRetornoSi) {
            removeChave(par.second);
        }

        return std::make_pair(novoLimiteBi, loteDeRetornoSi);
    }
}

// REMOVE CHAVE: A Correção Principal (Busca em D0 e D1)
void D::removeChave(size_t vertice) {
    auto iStatus = status.find(vertice);
    if (iStatus == status.end()) return;

    double distancia = iStatus->second;

    // 1. Tenta remover de D_1 (Busca rápida)
    auto iLimites = limites.lower_bound(distancia);
    bool encontrado = false;

    if (iLimites != limites.end()) {
        auto iBloco = iLimites->second;
        for (auto valor = iBloco->begin(); valor != iBloco->end(); /*nada*/) {
            if (valor->second == vertice) {
                valor = iBloco->erase(valor);
                encontrado = true;

                if (iBloco->empty() && iLimites->first != limiteSuperiorB) {
                    blocosD_1.erase(iBloco);
                    limites.erase(iLimites);
                }
                break;
            }
            else {
                ++valor;
            }
        }
    }

    // 2. SE NÃO ACHOU EM D_1, PROCURA EM D_0 (O que faltava no seu código!)
    if (!encontrado) {
        for (auto itBloco = blocosD_0.begin(); itBloco != blocosD_0.end(); /*nada*/) {
            bool apagouDoBloco = false;
            for (auto valor = itBloco->begin(); valor != itBloco->end(); /*nada*/) {
                if (valor->second == vertice) {
                    valor = itBloco->erase(valor);
                    encontrado = true;
                    apagouDoBloco = true;
                    break;
                }
                else {
                    ++valor;
                }
            }

            if (itBloco->empty()) {
                itBloco = blocosD_0.erase(itBloco);
            }
            else {
                ++itBloco;
            }

            if (encontrado) break;
        }
    }

    if (encontrado) {
        status.erase(iStatus);
    }
}

// DIVIDIR: (CORRIGIDO: Chave Máxima Real)
void D::dividir(std::map<double, std::list<Bloco>::iterator>::iterator& iLimites) {
    double limiteAntigo = iLimites->first;
    auto iBloco = iLimites->second;

    std::vector<ParDistVertice> aux(iBloco->begin(), iBloco->end());

    std::nth_element(aux.begin(), aux.begin() + tamLoteM / 2, aux.end());
    double valorMediana = aux[tamLoteM / 2].first;

    Bloco bloco1;
    Bloco bloco2;

    for (const auto& par : aux) {
        if (par.first <= valorMediana) bloco1.push_back(par);
        else bloco2.push_back(par);
    }

    *iBloco = bloco1;
    auto iBloco2 = blocosD_1.insert(std::next(iBloco), bloco2);

    limites.erase(iLimites);

    // Calcula o máximo real do bloco 1 para usar como chave
    double maxBloco1 = 0.0;
    if (!bloco1.empty()) {
        maxBloco1 = bloco1.front().first;
        for (const auto& p : bloco1) {
            if (p.first > maxBloco1) maxBloco1 = p.first;
        }
    }
    else {
        maxBloco1 = valorMediana;
    }

    limites[maxBloco1] = iBloco;
    limites[limiteAntigo] = iBloco2;
}

//#include "../headers/estruturaD.hpp"
//
//#include <algorithm>
//
//D::D(size_t M, double B) : tamLoteM(M), limiteSuperiorB(B) {
//	blocosD_1.push_back(Bloco());
//
//	const auto& i = blocosD_1.begin();
//	limites[B] = i;
//}
//
//void D::insert(size_t vertice, double distancia) {
//	auto iStatus = status.find(vertice);
//
//	if (iStatus != status.end()) {
//		if (iStatus->second > distancia) removeChave(vertice);
//		else return;
//	}
//
//	auto iLimites = limites.lower_bound(distancia);
//	auto iBloco = iLimites->second;
//	iBloco->push_back({ distancia, vertice });
//
//	status[vertice] = distancia;
//
//	if (iBloco->size() > tamLoteM) dividir(iLimites);
//}
//
//void D::batchPrepend(std::vector<ParDistVertice>& loteL) {
//	std::map<size_t, double> loteFiltrado;
//	std::vector<size_t> paraRemover;
//
//	for (auto& par : loteL) {
//		double distancia = par.first;
//		size_t vertice = par.second;
//
//		auto iLote = loteFiltrado.find(vertice);
//		if (iLote == loteFiltrado.end()) {
//			loteFiltrado[vertice] = distancia;
//		}
//		else if (distancia < loteFiltrado[vertice]) {
//			loteFiltrado[vertice] = distancia;
//		}
//	}
//
//	for (auto& par : loteFiltrado) {
//		double distancia = par.second;
//		size_t vertice = par.first;
//
//		auto iStatus = status.find(vertice);
//		if (iStatus != status.end()) {
//			double distanciaAntiga = iStatus->second;
//
//			if (distancia >= distanciaAntiga) paraRemover.push_back(vertice);
//		}
//	}
//
//	for (size_t v : paraRemover) loteFiltrado.erase(v);
//
//	if (loteFiltrado.size() <= tamLoteM) {
//		std::list<ParDistVertice> bloco;
//		for (auto& par : loteFiltrado) {
//			double distancia = par.second;
//			size_t vertice = par.first;
//
//			bloco.push_back({ distancia, vertice });
//		}
//		blocosD_0.push_front(bloco);
//	}
//	else {
//		std::vector<ParDistVertice> aux;
//		for (auto& par : loteFiltrado) {
//			double distancia = par.second;
//			size_t vertice = par.first;
//
//			aux.push_back({ distancia, vertice });
//		}
//
//		for (size_t i = 0; i < aux.size(); i += tamLoteM) {
//			std::list<ParDistVertice> bloco;
//			size_t fim = std::min(i + tamLoteM, aux.size());
//
//			for (size_t j = i; j < fim; j++) {
//				double distancia = aux[j].first;
//				size_t vertice = aux[j].second;
//
//				bloco.push_back({ distancia, vertice });
//			}
//			blocosD_0.push_front(bloco);
//		}
//	}
//
//	for (auto& par : loteFiltrado) {
//		double distancia = par.second;
//		size_t vertice = par.first;
//
//		status[vertice] = distancia;
//	}
//}
//
//std::pair<double, std::vector<ParDistVertice>> D::pull(){
//	std::vector<ParDistVertice> candidatosD_0;
//	std::vector<ParDistVertice> candidatosD_1;
//	std::vector<ParDistVertice> candidatosTotais;
//
//	for (auto& bloco : blocosD_0) {
//		for (auto &par : bloco) {
//			candidatosD_0.push_back(par);
//			if (candidatosD_0.size() >= tamLoteM) break;
//		}
//		if (candidatosD_0.size() >= tamLoteM) break;
//	}
//
//	for (auto& bloco : limites) {
//		for (auto& par : *(bloco.second)) {
//			candidatosD_1.push_back(par);
//			if (candidatosD_1.size() >= tamLoteM) break;
//		}
//		if (candidatosD_1.size() >= tamLoteM) break;
//	}
//
//	candidatosTotais.insert(candidatosTotais.end(), candidatosD_0.begin(), candidatosD_0.end());
//	candidatosTotais.insert(candidatosTotais.end(), candidatosD_1.begin(), candidatosD_1.end());
//
//	if (candidatosTotais.size() <= tamLoteM) {
//		blocosD_0.clear();
//		blocosD_1.clear();
//		limites.clear();
//		status.clear();
//
//		blocosD_1.push_back(Bloco());
//
//		const auto& i = blocosD_1.begin();
//		limites[limiteSuperiorB] = i;
//
//		return std::make_pair(limiteSuperiorB, candidatosTotais);
//	}
//	else {
//		std::nth_element(candidatosTotais.begin(), candidatosTotais.begin() + tamLoteM, candidatosTotais.end());
//		double novoLimiteBi = candidatosTotais[tamLoteM].first;
//		std::vector<ParDistVertice> loteDeRetornoSi;
//
//		for (size_t i = 0; i < tamLoteM; i++) {
//			loteDeRetornoSi.push_back(candidatosTotais[i]);
//		}
//
//		for (auto& par : loteDeRetornoSi) {
//			removeChave(par.second);
//		}
//
//		return std::make_pair(novoLimiteBi, loteDeRetornoSi);
//	}
//}
//
//void D::removeChave(size_t vertice) {
//	auto iStatus = status.find(vertice);
//	if (iStatus == status.end()) return;
//
//	double distancia = iStatus->second;
//	const auto& iLimites = limites.lower_bound(distancia);
//	auto& iBloco = iLimites ->second;
//
//	for (auto valor = iBloco->begin(); valor != iBloco->end(); valor++) {
//		if (valor->second == vertice) {
//			iBloco->erase(valor);
//			break;
//		}
//	}
//
//	status.erase(iStatus);
//
//	// Só apaga o bloco se estiver vazio E NÃO FOR o bloco sentinela (o último)
//	if (iBloco->empty() && iLimites->first != limiteSuperiorB) {
//		blocosD_1.erase(iBloco); // Apaga da lista
//		limites.erase(iLimites); // Apaga do mapa
//	}
//}
//
//void D::dividir(std::map<double, std::list<Bloco>::iterator>::iterator &iLimites) {
//	double limiteAntigo = iLimites->first;
//	auto iBloco = iLimites->second;
//	std::vector<ParDistVertice> aux(iBloco->begin(), iBloco->end());
//
//	std::nth_element(aux.begin(), aux.begin() + tamLoteM / 2, aux.end());
//	double valorMediana = aux[tamLoteM / 2].first;
//
//	Bloco bloco1;
//	Bloco bloco2;
//
//	for (ParDistVertice par : aux) {
//		if (par.first <= valorMediana) bloco1.push_back(par);
//		else bloco2.push_back(par);
//	}
//	*iBloco = bloco1;
//	auto iBloco2 = blocosD_1.insert(std::next(iBloco), bloco2);
//
//	limites.erase(iLimites);
//	limites[bloco1.back().first] = iBloco;
//	limites[limiteAntigo] = iBloco2;
//}
