#pragma once

#include "algoritmo.hpp"

#include <string>

std::pair<std::vector<double>, std::vector<size_t>> bellmanFord(const CaminhoMinimo::Grafo& grafo);

CaminhoMinimo::Grafo geraGrafo(size_t tamanho, double densidade);

void salvaGrafo(size_t tamanho, double densidade, const CaminhoMinimo::Grafo& grafo, std::string path);

void imprimeArquivo(const std::string& caminhoArquivo);