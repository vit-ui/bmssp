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

#ifdef VERSAO_MODERNACHAT

// --- helpers privados ---
static double make_unique_key(std::map<double, std::list<Bloco>::iterator>& m, double key) {
    // se chave já existe, avança para o nextafter para garantir unicidade
    while (m.find(key) != m.end()) {
        key = std::nextafter(key, std::numeric_limits<double>::infinity());
    }
    return key;
}

static double bloco_max(const Bloco& b, double defaultB) {
    if (b.empty()) return defaultB;
    double mx = b.front().first;
    for (auto& p : b) if (mx < p.first) mx = p.first;
    return mx;
}

// --- construtor / inicialização ---
D::D(size_t M, double B) : tamLoteM(M), limiteSuperiorB(B) {
    // iniciar D0 vazio e D1 com um bloco vazio cujo upper = B
    blocosD_0.clear();
    blocosD_1.clear();
    limites.clear();
    status.clear();
    indice.clear();
    upperOfBlock.clear();

    blocosD_1.emplace_back(); // 1 bloco vazio
    auto it = std::prev(blocosD_1.end());
    double key = make_unique_key(limites, limiteSuperiorB);
    limites.emplace(key, it);
    upperOfBlock[&(*it)] = key;
}

// --- remoção por vértice (O(1) amortizado usando 'indice') ---
void D::removeChave(size_t vertice) {
    auto itStatus = status.find(vertice);
    if (itStatus == status.end()) return; // não existe
    auto itIndice = indice.find(vertice);
    if (itIndice == indice.end()) {
        // inconsistência — mas apenas retorne
        status.erase(itStatus);
        return;
    }

    auto blocoIt = itIndice->second.first;
    auto elemIt = itIndice->second.second;

    // apagar elemento do bloco
    blocoIt->erase(elemIt);

    // apagar índice e status
    indice.erase(itIndice);
    status.erase(itStatus);

    // se bloco pertencer a D1 e ficou vazio -> remover entrada em limites e mapa upperOfBlock
    Bloco* ptr = &(*blocoIt);
    auto upIt = upperOfBlock.find(ptr);
    if (upIt != upperOfBlock.end()) {
        if (blocoIt->empty()) {
            double oldKey = upIt->second;
            limites.erase(oldKey);
            upperOfBlock.erase(upIt);
            // apagar bloco fisicamente de blocosD_1
            // para apagar precisamos encontrar o iterator na lista blocosD_1; blocoIt já é ele.
            blocosD_1.erase(blocoIt);
        }
        else {
            // atualiza upper para refletir remoção (lemA diz que não é necessário, mas mantemos o upper consistente)
            double newUp = bloco_max(*blocoIt, limiteSuperiorB);
            if (newUp != upIt->second) {
                limites.erase(upIt->second);
                double uniqueKey = make_unique_key(limites, newUp);
                limites.emplace(uniqueKey, blocoIt);
                upIt->second = uniqueKey;
            }
        }
    }
    else {
        // bloco pertence a D0: se vazio, remover bloco
        if (blocoIt->empty()) {
            // remover bloco de blocosD_0
            blocosD_0.erase(blocoIt);
        }
    }
}

// --- divisão conforme assinatura 'dividir' (usa iterator do map 'limites') ---
void D::dividir(std::map<double, std::list<Bloco>::iterator>::iterator& iLimites) {
    // iLimites aponta para (upperKey -> blocoIt)
    auto blocoIt = iLimites->second;
    if (blocoIt == blocosD_1.end()) return;
    size_t sz = blocoIt->size();
    if (sz <= tamLoteM) return; // nada a fazer

    // copiar elementos para vetor para particionar por rank
    std::vector<ParDistVertice> tmp;
    tmp.reserve(sz);
    for (auto& p : *blocoIt) tmp.push_back(p);

    size_t leftSize = (sz + 1) / 2;
    std::nth_element(tmp.begin(), tmp.begin() + leftSize - 1, tmp.end(),
        [](const ParDistVertice& a, const ParDistVertice& b) { return a.first < b.first; });
    double pivot = tmp[leftSize - 1].first;

    // distribuir preservando ordem original e garantindo leftSize
    Bloco leftList, rightList;
    size_t placedLeft = 0;
    for (auto it = blocoIt->begin(); it != blocoIt->end(); ++it) {
        if (it->first < pivot) { leftList.push_back(*it); ++placedLeft; }
        else if (it->first > pivot) rightList.push_back(*it);
        else { if (placedLeft < leftSize) { leftList.push_back(*it); ++placedLeft; } else rightList.push_back(*it); }
    }

    // remover entrada antiga em limites
    double oldKey = iLimites->first;
    // Erase by iterator safe:
    auto mapPos = iLimites;
    ++iLimites; // avancemos o iterator externo (o chamador pode usar) — mas aqui só precisamos do mapPos
    limites.erase(mapPos);

    // inserir left e right na lista blocosD_1 antes do bloco antigo (blocoIt)
    // como blocosD_1 é uma lista de Bloco, e blocoIt aponta para o bloco a ser substituído,
    // inserimos antes dele e depois apagamos o original.
    auto leftBlock = leftList;
    auto rightBlock = rightList;
    auto itLeft = blocosD_1.insert(blocoIt, std::move(leftBlock));
    auto itRight = blocosD_1.insert(blocoIt, std::move(rightBlock));

    // apagar o bloco antigo
    auto toErase = blocoIt;
    blocosD_1.erase(toErase);

    // registrar uppers para left e right (chaves únicas)
    double leftUp = bloco_max(*itLeft, limiteSuperiorB);
    double rightUp = bloco_max(*itRight, limiteSuperiorB);
    double leftKey = make_unique_key(limites, leftUp);
    double rightKey = make_unique_key(limites, rightUp);
    limites.emplace(leftKey, itLeft);
    limites.emplace(rightKey, itRight);

    // atualizar upperOfBlock
    upperOfBlock[&(*itLeft)] = leftKey;
    upperOfBlock[&(*itRight)] = rightKey;

    // atualizar 'indice' para elementos movidos (apontadores de elemento são válidos porque usamos list)
    for (auto eit = itLeft->begin(); eit != itLeft->end(); ++eit) {
        size_t v = eit->second;
        auto kit = indice.find(v);
        if (kit != indice.end()) { kit->second.first = itLeft; kit->second.second = eit; }
    }
    for (auto eit = itRight->begin(); eit != itRight->end(); ++eit) {
        size_t v = eit->second;
        auto kit = indice.find(v);
        if (kit != indice.end()) { kit->second.first = itRight; kit->second.second = eit; }
    }
}

// --- insert conforme lema ---
void D::insert(size_t vertice, double distancia) {
    auto stIt = status.find(vertice);
    if (stIt != status.end()) {
        double old = stIt->second;
        if (!(distancia < old)) return; // não substitui
        // remove antigo
        removeChave(vertice);
    }

    // localizar bloco alvo em D1 via limites.lower_bound(distancia)
    auto mit = limites.lower_bound(distancia);
    std::list<Bloco>::iterator blocoIt;
    if (mit == limites.end()) {
        // usar último bloco de D1
        blocoIt = std::prev(blocosD_1.end());
    }
    else {
        blocoIt = mit->second;
    }

    // inserir no bloco (ao final)
    blocoIt->push_back({ distancia, vertice });
    auto elemIt = std::prev(blocoIt->end());

    // atualizar status e indice
    status[vertice] = distancia;
    indice[vertice] = std::make_pair(blocoIt, elemIt);

    // atualizar upper do bloco (se for D1)
    Bloco* ptr = &(*blocoIt);
    auto upIt = upperOfBlock.find(ptr);
    if (upIt != upperOfBlock.end()) {
        double newUp = bloco_max(*blocoIt, limiteSuperiorB);
        if (newUp != upIt->second) {
            // remover entrada antiga e inserir nova com chave única
            limites.erase(upIt->second);
            double newKey = make_unique_key(limites, newUp);
            limites.emplace(newKey, blocoIt);
            upIt->second = newKey;
        }
    }
    else {
        // bloco escolhido pode ter sido o último de D1; se não estava registado ainda, registamos
        // (normalmente todos blocosD_1 têm entrada em limites, mas por segurança:)
        double up = bloco_max(*blocoIt, limiteSuperiorB);
        double key = make_unique_key(limites, up);
        limites.emplace(key, blocoIt);
        upperOfBlock[ptr] = key;
    }

    // se estourou M, dividir: precisamos passar iterator para map correspondente ao bloco
    // encontrar chave atual do bloco no mapa
    double currentKey = upperOfBlock[&(*blocoIt)];
    auto mapIt = limites.find(currentKey);
    if (blocoIt->size() > tamLoteM) {
        // passar iterator do map para dividir
        dividir(mapIt);
    }
}

// --- batchPrepend conforme lema ---
void D::batchPrepend(std::vector<ParDistVertice>& loteL) {
    if (loteL.empty()) return;

    // 1) dedup dentro do lote: manter menor distância por vértice
    std::unordered_map<size_t, double> minInL;
    minInL.reserve(loteL.size() * 2 + 1);
    for (auto& p : loteL) {
        size_t v = p.second; double d = p.first;
        auto it = minInL.find(v);
        if (it == minInL.end() || d < it->second) minInL[v] = d;
    }
    std::vector<ParDistVertice> L;
    L.reserve(minInL.size());
    for (auto& kv : minInL) L.emplace_back(kv.second, kv.first);
    if (L.empty()) return;

    // 2) verificar pré-condição: max(L) < min(existing) (se existir)
    double maxL = L.front().first;
    for (auto& p : L) if (maxL < p.first) maxL = p.first;
    // encontrar min existing
    std::optional<double> minExisting;
    // procurar primeiro bloco não vazio em D0
    for (auto& b : blocosD_0) if (!b.empty()) { double m = b.front().first; for (auto& e : b) if (e.first < m) m = e.first; minExisting = m; break; }
    // se não encontrou em D0, checar D1
    if (!minExisting.has_value()) {
        for (auto& b : blocosD_1) if (!b.empty()) { double m = b.front().first; for (auto& e : b) if (e.first < m) m = e.first; minExisting = m; break; }
    }
    if (minExisting.has_value()) {
        assert(maxL < minExisting.value() && "batchPrepend requires max(L) < min(existing)");
    }

    // 3) remover chaves existentes se o valor novo for menor
    std::vector<ParDistVertice> filtered;
    filtered.reserve(L.size());
    for (auto& p : L) {
        double d = p.first; size_t v = p.second;
        auto stIt = status.find(v);
        if (stIt == status.end()) filtered.push_back(p);
        else {
            double old = stIt->second;
            if (d < old) { removeChave(v); filtered.push_back(p); }
        }
    }
    if (filtered.empty()) return;

    // 4) particionar filtered em blocos de <= ceil(M/2) usando recursão com nth_element
    size_t half = (tamLoteM + 1) / 2;
    std::vector<ParDistVertice> arr = filtered;
    std::vector<std::vector<ParDistVertice>> chunks;
    std::function<void(size_t, size_t)> partitionRec = [&](size_t l, size_t r) {
        size_t len = r - l;
        if (len == 0) return;
        if (len <= half) {
            chunks.emplace_back();
            for (size_t i = l; i < r; ++i) chunks.back().push_back(arr[i]);
            return;
        }
        size_t mid = l + len / 2;
        std::nth_element(arr.begin() + l, arr.begin() + mid, arr.begin() + r,
            [](const ParDistVertice& a, const ParDistVertice& b) { return a.first < b.first; });
        partitionRec(l, mid);
        partitionRec(mid, r);
        };
    partitionRec(0, arr.size());

    // 5) inserir chunks no início de D0 (inserir do último para o primeiro para manter ordem crescente)
    for (int i = (int)chunks.size() - 1; i >= 0; --i) {
        blocosD_0.emplace_front();
        auto bIt = blocosD_0.begin();
        for (auto& p : chunks[i]) {
            bIt->push_back(p);
            auto elemIt = std::prev(bIt->end());
            // registrar indice e status
            size_t v = elemIt->second;
            indice[v] = std::make_pair(bIt, elemIt);
            status[v] = elemIt->first;
        }
    }
}

// --- pull: retorna até M pares (dist,vert) e o separador x ---
std::pair<double, std::vector<ParDistVertice>> D::pull() {
    std::vector<std::list<Bloco>::iterator> collectedD0, collectedD1;
    size_t collected = 0;

    for (auto it = blocosD_0.begin(); it != blocosD_0.end() && collected < tamLoteM; ++it) {
        if (!it->empty()) { collectedD0.push_back(it); collected += it->size(); }
    }
    for (auto it = blocosD_1.begin(); it != blocosD_1.end() && collected < tamLoteM; ++it) {
        if (!it->empty()) { collectedD1.push_back(it); collected += it->size(); }
    }

    size_t total = 0;
    for (auto& b : blocosD_0) total += b.size();
    for (auto& b : blocosD_1) total += b.size();

    if (total <= tamLoteM) {
        // retornar tudo
        std::vector<ParDistVertice> all; all.reserve(total);
        for (auto& b : blocosD_0) for (auto& e : b) all.push_back(e);
        for (auto& b : blocosD_1) for (auto& e : b) all.push_back(e);
        // limpar tudo e re-inicializar um único bloco D1 com upper = B
        blocosD_0.clear();
        blocosD_1.clear(); limites.clear(); status.clear(); indice.clear(); upperOfBlock.clear();
        blocosD_1.emplace_back();
        auto it = std::prev(blocosD_1.end());
        double key = make_unique_key(limites, limiteSuperiorB);
        limites.emplace(key, it);
        upperOfBlock[&(*it)] = key;
        return { limiteSuperiorB, all };
    }

    // montar vetor de candidatos (todas os elementos dos blocos coletados)
    struct Cand { double val; std::list<Bloco>::iterator bloco; Bloco::iterator elem; };
    std::vector<Cand> cand; cand.reserve(collected);
    for (auto& bIt : collectedD0) for (auto eit = bIt->begin(); eit != bIt->end(); ++eit) cand.push_back({ eit->first, bIt, eit });
    for (auto& bIt : collectedD1) for (auto eit = bIt->begin(); eit != bIt->end(); ++eit) cand.push_back({ eit->first, bIt, eit });

    size_t want = tamLoteM;
    std::nth_element(cand.begin(), cand.begin() + (want - 1), cand.end(), [](const Cand& a, const Cand& b) { return a.val < b.val; });
    double thresh = cand[0].val;
    for (size_t i = 1; i < want; ++i) if (cand[i].val > thresh) thresh = cand[i].val;

    // selecionar exatamente want elementos
    std::vector<char> take(cand.size(), 0);
    size_t cntLess = 0;
    for (size_t i = 0; i < cand.size(); ++i) if (cand[i].val < thresh) { take[i] = 1; ++cntLess; }
    size_t needEq = want - cntLess;
    for (size_t i = 0; i < cand.size() && needEq > 0; ++i) if (!take[i] && cand[i].val == thresh) { take[i] = 1; --needEq; }

    // remover selecionados (apagar do bloco e dos índices)
    std::vector<ParDistVertice> out; out.reserve(want);
    for (size_t i = 0; i < cand.size(); ++i) if (take[i]) {
        auto v = cand[i].elem->second;
        out.push_back(ParDistVertice{ cand[i].val, v });
        // apagar do indice/status e do bloco
        auto idxIt = indice.find(v);
        if (idxIt != indice.end()) indice.erase(idxIt);
        auto stIt = status.find(v);
        if (stIt != status.end()) status.erase(stIt);
        cand[i].bloco->erase(cand[i].elem);
    }

    // após remoções: se algum bloco D1 ficou vazio, remover limite
    for (auto it = blocosD_1.begin(); it != blocosD_1.end(); ) {
        if (it->empty()) {
            Bloco* ptr = &(*it);
            auto upIt = upperOfBlock.find(ptr);
            if (upIt != upperOfBlock.end()) {
                limites.erase(upIt->second);
                upperOfBlock.erase(upIt);
            }
            it = blocosD_1.erase(it);
        }
        else ++it;
    }
    // remover blocos D0 vazios
    for (auto it = blocosD_0.begin(); it != blocosD_0.end(); ) {
        if (it->empty()) it = blocosD_0.erase(it);
        else ++it;
    }

    // calcular x = menor remaining value em D0 U D1 ou B se não houver
    std::optional<double> minRem;
    auto scanMin = [&](std::list<Bloco>::iterator bIt) -> std::optional<double> {
        if (bIt == std::list<Bloco>::iterator() || bIt->empty()) return {};
        double m = bIt->begin()->first;
        for (auto& e : *bIt) if (e.first < m) m = e.first;
        return m;
        };

    for (auto& bIt : collectedD0) { auto m = scanMin(bIt); if (m.has_value()) { if (!minRem.has_value() || m.value() < minRem.value()) minRem = m; } }
    for (auto& bIt : collectedD1) { auto m = scanMin(bIt); if (m.has_value()) { if (!minRem.has_value() || m.value() < minRem.value()) minRem = m; } }

    // também checar próximo bloco não-coletado em cada sequência (pequena verificação)
    if (!collectedD0.empty()) {
        auto last = collectedD0.back();
        auto nextIt = std::next(last);
        if (nextIt != blocosD_0.end()) { auto m = scanMin(nextIt); if (m.has_value()) { if (!minRem.has_value() || m.value() < minRem.value()) minRem = m; } }
    }
    else {
        if (!blocosD_0.empty()) { auto m = scanMin(blocosD_0.begin()); if (m.has_value()) { if (!minRem.has_value() || m.value() < minRem.value()) minRem = m; } }
    }
    if (!collectedD1.empty()) {
        auto last = collectedD1.back();
        auto nextIt = std::next(last);
        if (nextIt != blocosD_1.end()) { auto m = scanMin(nextIt); if (m.has_value()) { if (!minRem.has_value() || m.value() < minRem.value()) minRem = m; } }
    }
    else {
        if (!blocosD_1.empty()) { auto m = scanMin(blocosD_1.begin()); if (m.has_value()) { if (!minRem.has_value() || m.value() < minRem.value()) minRem = m; } }
    }

    double x = minRem.has_value() ? minRem.value() : limiteSuperiorB;
    return { x, out };
}

#elif defined(VERSAO_MODERNAGEMINI)

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

// BATCH PREPEND: Insere lote urgente
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

    // 3. Copia para vetor e ORDENA
    std::vector<ParDistVertice> aux;
    aux.reserve(loteFiltrado.size());
    for (auto& par : loteFiltrado) {
        aux.push_back({ par.second, par.first });
        status[par.first] = par.second; // Atualiza status global
    }
    std::sort(aux.begin(), aux.end()); // Menores distâncias primeiro

    // 4. Insere em D_0
    if (aux.size() <= tamLoteM) {
        std::list<ParDistVertice> bloco;
        for (const auto& p : aux) bloco.push_back(p);
        blocosD_0.push_front(bloco);
    }
    else {
        // Se o lote é maior que M, quebramos em vários blocos inserindo de trás pra frente
        size_t resto = aux.size() % tamLoteM;
        size_t inicioUltimo = (resto == 0) ? (aux.size() - tamLoteM) : (aux.size() - resto);

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

    double distancia = iStatus->second;

    // 1. Tenta remover de D_1
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
}

// DIVIDIR
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

    // Calcula o máximo real do bloco 1
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

#else

D::D(size_t M, double B) : tamLoteM(M), limiteSuperiorB(B) {
	blocosD_1.push_back(Bloco());

	const auto& i = blocosD_1.begin();
	limites[B] = i;
}

void D::insert(size_t vertice, double distancia) {
	auto iStatus = status.find(vertice);

	if (iStatus != status.end()) {
		if (iStatus->second > distancia) removeChave(vertice);
		else return;
	}

	auto iLimites = limites.lower_bound(distancia);
	auto iBloco = iLimites->second;
	iBloco->push_back({ distancia, vertice });

	status[vertice] = distancia;

	if (iBloco->size() > tamLoteM) dividir(iLimites);
}

void D::batchPrepend(std::vector<ParDistVertice>& loteL) {
	std::map<size_t, double> loteFiltrado;
	std::vector<size_t> paraRemover;

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

	for (auto& par : loteFiltrado) {
		double distancia = par.second;
		size_t vertice = par.first;

		auto iStatus = status.find(vertice);
		if (iStatus != status.end()) {
			double distanciaAntiga = iStatus->second;

			if (distancia >= distanciaAntiga) paraRemover.push_back(vertice);
		}
	}

	for (size_t v : paraRemover) loteFiltrado.erase(v);

	if (loteFiltrado.size() <= tamLoteM) {
		std::list<ParDistVertice> bloco;
		for (auto& par : loteFiltrado) {
			double distancia = par.second;
			size_t vertice = par.first;

			bloco.push_back({ distancia, vertice });
		}
		blocosD_0.push_front(bloco);
	}
	else {
		std::vector<ParDistVertice> aux;
		for (auto& par : loteFiltrado) {
			double distancia = par.second;
			size_t vertice = par.first;

			aux.push_back({ distancia, vertice });
		}

		for (size_t i = 0; i < aux.size(); i += tamLoteM) {
			std::list<ParDistVertice> bloco;
			size_t fim = std::min(i + tamLoteM, aux.size());

			for (size_t j = i; j < fim; j++) {
				double distancia = aux[j].first;
				size_t vertice = aux[j].second;

				bloco.push_back({ distancia, vertice });
			}
			blocosD_0.push_front(bloco);
		}
	}

	for (auto& par : loteFiltrado) {
		double distancia = par.second;
		size_t vertice = par.first;

		status[vertice] = distancia;
	}
}

std::pair<double, std::vector<ParDistVertice>> D::pull() {
    std::vector<ParDistVertice> candidatosD_0;
    std::vector<ParDistVertice> candidatosD_1;
    std::vector<ParDistVertice> candidatosTotais;

    // [CORREÇÃO IMPORTANTE]
    // Tente coletar um elemento a mais (M+1) para saber se existem sobras.
    // Se tamLoteM for 1, tentamos pegar 2. Se conseguirmos 2, sabemos que a lista não acabou.
    size_t limiteVerificacao = tamLoteM + 1;

    // Coleta de D_0 usando o novo limite
    for (auto& bloco : blocosD_0) {
        for (auto& par : bloco) {
            candidatosD_0.push_back(par);
            if (candidatosD_0.size() >= limiteVerificacao) break;
        }
        if (candidatosD_0.size() >= limiteVerificacao) break;
    }

    // Coleta de D_1 usando o novo limite
    for (auto& bloco : limites) {
        for (auto& par : *(bloco.second)) {
            candidatosD_1.push_back(par);
            if (candidatosD_1.size() >= limiteVerificacao) break;
        }
        if (candidatosD_1.size() >= limiteVerificacao) break;
    }

    candidatosTotais.insert(candidatosTotais.end(), candidatosD_0.begin(), candidatosD_0.end());
    candidatosTotais.insert(candidatosTotais.end(), candidatosD_1.begin(), candidatosD_1.end());

    // [LÓGICA CORRIGIDA]
    // Só limpamos tudo se a quantidade TOTAL encontrada for realmente <= M.
    // Antes, você limpava mesmo se tivesse sobrado itens nos blocos não lidos.
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
        // Se caiu aqui, temos pelo menos M+1 elementos, então NÃO limpamos os blocos originais.
        // Apenas particionamos e retornamos os M melhores.

        std::nth_element(candidatosTotais.begin(), candidatosTotais.begin() + tamLoteM, candidatosTotais.end());

        double novoLimiteBi = candidatosTotais[tamLoteM].first;

        // Tratamento de empate (mantido do original)
        double maxSi = candidatosTotais[0].first;
        for (size_t k = 1; k < tamLoteM; ++k) {
            if (candidatosTotais[k].first > maxSi) maxSi = candidatosTotais[k].first;
        }
        if (novoLimiteBi <= maxSi) novoLimiteBi = maxSi + 1e-9; // Pequeno delta para garantir desigualdade estrita

        std::vector<ParDistVertice> loteDeRetornoSi;
        loteDeRetornoSi.reserve(tamLoteM);

        for (size_t i = 0; i < tamLoteM; i++) {
            loteDeRetornoSi.push_back(candidatosTotais[i]);
        }

        // Removemos apenas os que vamos processar agora
        for (auto& par : loteDeRetornoSi) {
            removeChave(par.second);
        }

        return std::make_pair(novoLimiteBi, loteDeRetornoSi);
    }
}

void D::removeChave(size_t vertice) {
	auto iStatus = status.find(vertice);
	if (iStatus == status.end()) return;

	double distancia = iStatus->second;
	const auto& iLimites = limites.lower_bound(distancia);
	auto& iBloco = iLimites ->second;

	for (auto valor = iBloco->begin(); valor != iBloco->end(); valor++) {
		if (valor->second == vertice) {
			iBloco->erase(valor);
			break;
		}
	}

	status.erase(iStatus);

	// Só apaga o bloco se estiver vazio E NÃO FOR o bloco sentinela (o último)
	if (iBloco->empty() && iLimites->first != limiteSuperiorB) {
		blocosD_1.erase(iBloco); // Apaga da lista
		limites.erase(iLimites); // Apaga do mapa
	}
}

void D::dividir(std::map<double, std::list<Bloco>::iterator>::iterator &iLimites) {
	double limiteAntigo = iLimites->first;
	auto iBloco = iLimites->second;
	std::vector<ParDistVertice> aux(iBloco->begin(), iBloco->end());

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
#endif