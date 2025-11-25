#include "../headers/estruturaD.hpp"

#include <vector>
#include <list>
#include <map>
#include <utility> // Para std::pair
#include <limits>
#include <optional>
#include <unordered_map>
#include <cmath>
#include <functional>
#include <cassert>
#include <algorithm>
#include <queue>


// Construtor
D::D(size_t M, double B) : tamLoteM(M), limiteSuperiorB(B) {
    // Inicializa D_1 com um bloco vazio e com limite B
    blocosD_1.push_back(Bloco());
    const auto& i = blocosD_1.begin();
    limites[B] = i;
}

// Insert a key/value pair in amortized O(max{1, log(N/M)}) time. 
// If the key already exists, update its value.
void D::insert(size_t vertice, double distancia) {
    auto iStatus = status.find(vertice);

    // Se achar o vertice e se nova distância é pior ou igual, ignora
    if (iStatus != status.end()) {
#ifdef O1
        if (iStatus->second.iElem->first <= distancia) return;
#else
        if (iStatus->second <= distancia) return;
#endif
        // Se existir, remove a antiga antes de inserir a nova
        removeChave(vertice);
    }

    // Encontra o bloco correto em D_1
    auto iLimites = limites.lower_bound(distancia);

    iLimites->second->push_back({ distancia, vertice });

#ifdef O1
    status.insert_or_assign(vertice, Status{ distancia, std::prev(iLimites->second->end()), iLimites->second, true });
#else
    status.insert_or_assign(vertice, distancia);
#endif
    // Se o bloco estourou o tamanho M, divide ele
    if (iLimites->second->size() > tamLoteM) dividir(iLimites);
}

// Esta função é chamada internamente por D::batchPrepend
// Processa o range [inicio, fim) do vetor 'lotes'
void D::dividirLote(std::vector<ParDistVertice>& lotes, size_t inicio, size_t fim) {
    size_t tamanho = fim - inicio;

    // A. Condição de Parada (O(M log M))
    if (tamanho <= tamLoteM) {
        // 1. ORDENAÇÃO LOCAL (CORREÇÃO SEMÂNTICA)
        std::sort(lotes.begin() + inicio, lotes.begin() + fim);

        // 2. CRIA E INSERE O BLOCO
        std::list<ParDistVertice> bloco(lotes.begin() + inicio, lotes.begin() + fim);
        
        // Captura o iterador do bloco recém-inserido
        auto iBloco = blocosD_0.insert(blocosD_0.begin(), std::move(bloco));
#ifdef O1
        // Iteração O(M) para atualizar o status para cada elemento do novo bloco
        for (auto iElem = iBloco->begin(); iElem != iBloco->end(); ++iElem) {
            status.insert_or_assign(
                iElem->second,
                Status{
                    iElem->first, // distancia
                    iElem,        // iElem (Iterador do elemento)
                    iBloco,       // iBloco (Iterador do bloco)
                    false         // pertenceD1 é FALSE (está em D0)
                }
            );
        }
#endif
        return;
    }

    // B. Encontra Mediana e Particiona (O(tamanho))
    size_t indiceMediana = inicio + tamanho / 2;

    auto inicio_it = std::next(lotes.begin(), inicio);
    auto mediana_it = std::next(lotes.begin(), indiceMediana);
    auto fim_it = std::next(lotes.begin(), fim);

    std::nth_element(inicio_it, mediana_it, fim_it);

    // C. Chamadas Recursivas para garantir a ordem de inserção (Maior antes do Menor)

    // 1. Bloco 2: Elementos MAIORES ou IGUAIS (Maior limite superior)
    // Chamamos o Bloco 2 primeiro para que ele seja inserido na frente de D0.
    dividirLote(lotes, indiceMediana, fim);

    // 2. Bloco 1: Elementos MENORES
    dividirLote(lotes, inicio, indiceMediana);
}

// BATCH PREPEND: Insere lote urgente
void D::batchPrepend(std::vector<ParDistVertice>& loteL) {
    std::unordered_map<size_t, double> loteFiltrado;
    std::vector<size_t> paraRemoverDoStatus;

    // 1. Filtra o melhor dentro do lote (resolve duplicatas no input)
    for (auto& [distancia, vertice] : loteL) {
        auto iLote = loteFiltrado.find(vertice);
        if (iLote == loteFiltrado.end() || distancia < loteFiltrado[vertice]) {
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
#ifdef O1

            double distanciaAntiga = iStatus->second.iElem->first;
#else
            double distanciaAntiga = iStatus->second;  
#endif
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
    for (size_t v : paraRemoverDoStatus) removeChave(v);

    // 3. Copia para vetor e ORDENA
    std::vector<ParDistVertice> aux;
    aux.reserve(loteFiltrado.size());
    for (auto& [vertice, distancia] : loteFiltrado) {
        aux.push_back({ distancia, vertice });
        //status.insert_or_assign(vertice, Status(distancia)); // Atualiza status global
    }

#ifdef RANGE
    struct Range { size_t inicio, fim; };
    std::vector<Range> stack;
    stack.reserve(64); // ajuste conforme necessário

    stack.push_back({ 0, aux.size() });

    while (!stack.empty()) {
        Range r = stack.back();
        stack.pop_back();

        size_t tamanho = r.fim - r.inicio;
        if (tamanho == 0) continue;

        if (tamanho <= tamLoteM) {
            // 1) ordenação local
            std::sort(aux.begin() + r.inicio, aux.begin() + r.fim);

            // 2) cria e insere o bloco na frente de blocosD_0
            std::list<ParDistVertice> bloco(aux.begin() + r.inicio, aux.begin() + r.fim);
            auto iBloco = blocosD_0.insert(blocosD_0.begin(), std::move(bloco));

            // 3) atualiza status para cada elemento do novo bloco
            for (auto iElem = iBloco->begin(); iElem != iBloco->end(); ++iElem) {
#ifdef O1
                status.insert_or_assign(
                    iElem->second,
                    Status{
                        iElem->first,
                        iElem,   // iterador do elemento no bloco
                        iBloco,  // iterador do bloco em blocosD_0
                        false    // pertenceD1 = false (está em D0)
                    }
                );
#else
                status.insert_or_assign(iElem->second, iElem->first);
#endif
            }
        }
        else
        {
            // Particiona para encontrar a mediana aproximada
            size_t indiceMediana = r.inicio + tamanho / 2;
            auto inicio_it = std::next(aux.begin(), r.inicio);
            auto mediana_it = std::next(aux.begin(), indiceMediana);
            auto fim_it = std::next(aux.begin(), r.fim);

            std::nth_element(inicio_it, mediana_it, fim_it);

            // Empilha ESQUERDA primeiro e DIREITA depois para que a DIREITA
            // seja processada primeiro (LIFO) — preserva a ordem do recursivo.
            stack.push_back({ r.inicio, indiceMediana });   // menores
            stack.push_back({ indiceMediana, r.fim });      // maiores ou iguais
        }
    }
#else
    std::sort(aux.begin(), aux.end()); // Menores distâncias primeiro
    // 4. Insere em D_0
    if (aux.size() <= tamLoteM) {
        std::list<ParDistVertice> bloco(aux.begin(), aux.end());
        //for (const auto& p : aux) bloco.push_back(p);
        auto iBloco = blocosD_0.insert(blocosD_0.begin(), std::move(bloco));

        // 2. Itera sobre o novo bloco para atualizar o Status
        for (auto iElem = iBloco->begin(); iElem != iBloco->end(); ++iElem) {
#ifdef O1
            status.insert_or_assign(iElem->second, Status{iElem->first, iElem, iBloco,false });
#else
            status.insert_or_assign(iElem->second, iElem->first);
#endif
        }
    }
    else {
        // Se o lote é maior que M, quebramos em vários blocos inserindo de trás pra frente
        size_t resto = aux.size() % tamLoteM;
        size_t inicioUltimo = (resto == 0) ? (aux.size() - tamLoteM) : (aux.size() - resto);

        for (long long i = inicioUltimo; i >= 0; i -= tamLoteM) {
            std::list<ParDistVertice> bloco(aux.begin() + i, aux.begin() + std::min((size_t)i + tamLoteM, aux.size()));
 /*           size_t fim = std::min((size_t)i + tamLoteM, aux.size());

            for (size_t j = (size_t)i; j < fim; j++) {
                bloco.push_back(aux[j]);
            }*/
            auto iBloco = blocosD_0.insert(blocosD_0.begin(), std::move(bloco));

            // 3. Itera sobre o novo bloco para atualizar o Status
            for (auto iElem = iBloco->begin(); iElem != iBloco->end(); ++iElem) {
#ifdef O1
                status.insert_or_assign(iElem->second, Status{ iElem->first, iElem, iBloco,false });
#else
                status.insert_or_assign(iElem->second, iElem->first);
#endif
            }
        }
    }
#endif
}

std::pair<double, std::vector<ParDistVertice>> D::pull() {
    std::vector<ParDistVertice> candidatosD_0;
    std::vector<ParDistVertice> candidatosD_1;
    std::vector<ParDistVertice> candidatosTotais;

    // Coleta sempre um a mais para saber se há excedente
    size_t limiteVerificacao = tamLoteM + 1;

    // 1. Coleta de D_0
    for (auto& bloco : blocosD_0) {
        for (auto& par : bloco) {
            candidatosD_0.push_back(par);
            //if (candidatosD_0.size() >= limiteVerificacao) break;
        }
        if (candidatosD_0.size() >= limiteVerificacao) break;
    }

    // 2. Coleta de D_1
    for (auto& bloco : limites) {
        for (auto& par : *(bloco.second)) {
            candidatosD_1.push_back(par);
            //if (candidatosD_1.size() >= limiteVerificacao) break;
        }
        if (candidatosD_1.size() >= limiteVerificacao) break;
    }

    candidatosTotais.insert(candidatosTotais.end(), candidatosD_0.begin(), candidatosD_0.end());
    candidatosTotais.insert(candidatosTotais.end(), candidatosD_1.begin(), candidatosD_1.end());

    double novoLimiteBi;
    std::vector<ParDistVertice> loteDeRetornoSi;

    // 3. Lógica Unificada: Sempre particiona e remove individualmente
    if (candidatosTotais.empty()) {
        return std::make_pair(limiteSuperiorB, std::vector<ParDistVertice>{});
    }

    if (candidatosTotais.size() <= tamLoteM) {
        // Caso A: Temos menos ou igual a M elementos. Retornamos TUDO o que achamos.
        // O limite vira o limite superior da estrutura, pois esgotamos os candidatos visíveis
        novoLimiteBi = limiteSuperiorB;
        loteDeRetornoSi = candidatosTotais;
    }
    else {
        // Caso B: Temos mais que M. Pegamos apenas os M melhores.
        std::nth_element(candidatosTotais.begin(), candidatosTotais.begin() + tamLoteM, candidatosTotais.end());

        novoLimiteBi = candidatosTotais[tamLoteM].first;

        // Tratamento de empate
        double maxSi = candidatosTotais[0].first;
        for (size_t k = 1; k < tamLoteM; ++k) {
            if (candidatosTotais[k].first > maxSi) maxSi = candidatosTotais[k].first;
        }
        if (novoLimiteBi <= maxSi) novoLimiteBi = maxSi + 1e-9;

        loteDeRetornoSi.reserve(tamLoteM);
        for (size_t i = 0; i < tamLoteM; i++) {
            loteDeRetornoSi.push_back(candidatosTotais[i]);
        }
    }

    // 4. REMOÇÃO SEGURA: Removemos APENAS os itens que vamos retornar.
    for (auto& par : loteDeRetornoSi) {
        removeChave(par.second);
    }

    return std::make_pair(novoLimiteBi, loteDeRetornoSi);
}

// REMOVE CHAVE
void D::removeChave(size_t vertice) {
    auto iStatus = status.find(vertice);
    if (iStatus == status.end()) return;

#ifdef O1
    auto& dadosStatus = iStatus->second;
    auto iElem = dadosStatus.iElem;       // Iterador do elemento na lista
    auto iBloco = dadosStatus.iBloco;     // Iterador do bloco na lista de blocos
    bool pertenceD1 = dadosStatus.pertenceD1;

    iBloco->erase(iElem);

    // 3. Lógica de Remoção de Bloco Vazio (O(1) ou O(log(N/M)))
    if (iBloco->empty()) {

        if (pertenceD1) {
            // Remove o bloco de D_1
            // A remoção do bloco D_1 e do limite (O(log N/M)) é a parte logarítmica,
            // que é aceitável, pois só ocorre quando um bloco fica vazio.

            // Reconstituindo o iLimites a partir de iBloco (O(log N/M) na BST limites)
            // Se o limite não fosse B (limite superior da estrutura)

            auto iLimites = limites.lower_bound(dadosStatus.iElem->first);

            // **Nota:** A busca por iLimites aqui é O(log N/M) e é o custo dominante
            // na remoção de blocos vazios, mas é aceito pela definição teórica.

            if (iLimites->first != limiteSuperiorB) {
                blocosD_1.erase(iBloco);
                limites.erase(iLimites);
            }
        }
        else {
            // Remove o bloco de D_0 (O(1) se blocosD_0 for std::list)
            blocosD_0.erase(iBloco);
        }
    }
    status.erase(iStatus);
#else

     //4. Remoção do Status (O(1) em Média)
 
    double distancia = iStatus->second;

    // 1. Tenta remover de D_1
    auto iLimites = limites.lower_bound(distancia);
    bool encontrado = false;

    if (iLimites != limites.end()) {
        auto iBloco = iLimites->second;
        auto valor = iBloco->begin();
        while(valor != iBloco->end()) {
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

    // 2. SE NÃO ACHOU EM D_1, PROCURA EM D_0
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
#endif
}

// DIVIDIR
void D::dividir(std::map<double, std::list<Bloco>::iterator>::iterator& iLimites) {
    double limiteAntigo = iLimites->first;
    auto iBloco = iLimites->second;

    std::vector<ParDistVertice> aux(iBloco->begin(), iBloco->end());

    // First, we identify the median element within the block in O(M) time[BFP + 73], 
    // partitioning the elements into two new blocks each with at most ⌈M / 2⌉ elements.
    // elements smaller than the median are placed in the first block, while the rest 
    // are placed in the second.
    std::nth_element(aux.begin(), aux.begin() + tamLoteM / 2, aux.end());
    double valorMediana = aux[tamLoteM / 2].first;

    // Criamos APENAS o bloco 2 para onde moveremos os elementos MAIORES
    Bloco bloco2;

    // 2. Iterar sobre os elementos (agora ordenados por valor) e parti-los
    //    (O(M) de list::iterator traversal)
    auto it = iBloco->begin();
    while (it != iBloco->end()) {
        if (it->first > valorMediana) {
            // Se for MAIOR, move o NÓ para bloco2. splice é O(1)
            bloco2.splice(bloco2.end(), *iBloco, it++);
        }
        else ++it;
    }

    // 4. Inserir bloco2
    auto iBloco2 = blocosD_1.insert(std::next(iBloco), std::move(bloco2));

    for (auto iElem = iBloco2->begin(); iElem != iBloco2->end(); ++iElem) {
        // O vértice é o segundo elemento do ParDistVertice
        size_t vertice = iElem->second;

        // Atualiza a localização do bloco em O(1) médio
        // Note que pertenceD1 já é TRUE aqui.
#ifdef O1
        status[vertice].iBloco = iBloco2;
#endif
    }

    double maxBloco1 = valorMediana;

    // 1. Apaga o limite antigo. Isto é O(log N) ou O(1) se usar o iterador.
    // O iterador iLimites não é mais válido após o erase.
    auto next_it = limites.erase(iLimites);

    limites.insert(next_it, { maxBloco1, iBloco });
    limites.insert(next_it, { limiteAntigo, iBloco2 });
}