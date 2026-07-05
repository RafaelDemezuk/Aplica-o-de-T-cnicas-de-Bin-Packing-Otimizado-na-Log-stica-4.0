#include <iostream>
#include <vector>
#include <algorithm>
#include <fstream>
#include <ctime>
#include <cstdlib>
#include <random>
#include <climits>
#include <omp.h>
#include <numeric>
using namespace std;

// ============================================================
// PARÂMETROS DE EXPERIMENTO 
// ============================================================
// IMPORTANTE: agora que a solução inicial é construída de forma
// GULOSA (determinística, sem aleatoriedade), todas as repetições
// produzirão exatamente o mesmo número de containers. Mantemos
// REPETICOES > 1 apenas para obter uma média de tempo mais estável
// (o tempo de execução pode variar levemente entre rodadas).
#define REPETICOES 10
// ============================================================

struct Container {
    int capacidade_total;
    int capacidade_atual = 0;
    vector<int> items;
};

bool is_excluded(const Container& c, int bin_capacity) {
    return (bin_capacity - c.capacidade_atual) <= 10;
}

// Ordena itens em ordem decrescente (mesma lógica do algoritmo guloso)
void ordena(vector<int>& p) {
    for (int i = (int)p.size() - 1; i >= 0; --i) {
        for (int j = 0; j < i; j++) {
            if (p[j] < p[j + 1]) {
                int temp = p[j];
                p[j] = p[j + 1];
                p[j + 1] = temp;
            }
        }
    }
}

void insere(vector<int>& items, int bin_capacity, vector<Container>& containers) {
    for (int item : items) {
        bool placed = false;
        for (Container& c : containers) {
            if (c.capacidade_atual + item <= bin_capacity) {
                c.items.push_back(item);
                c.capacidade_atual += item;
                placed = true;
                break;
            }
        }
        if (!placed) {
            Container new_container;
            new_container.capacidade_total = bin_capacity;
            new_container.items.push_back(item);
            new_container.capacidade_atual = item;
            containers.push_back(new_container);
        }
    }
}

bool try_move(vector<Container>& containers, int bin_capacity) {
    for (int i = 0; i < (int)containers.size(); ++i) {
        Container& source = containers[i];
        if (source.items.size() != 1 || is_excluded(source, bin_capacity))
            continue;
        int item = source.items[0];
        for (int j = 0; j < (int)containers.size(); ++j) {
            if (i == j) continue;
            Container& target = containers[j];
            if (is_excluded(target, bin_capacity)) continue;
            int available_space = bin_capacity - target.capacidade_atual;
            if (available_space >= item && (available_space - item) > 5) {
                target.items.push_back(item);
                target.capacidade_atual += item;
                containers.erase(containers.begin() + i);
                return true;
            }
        }
    }
    return false;
}

bool try_swap(vector<Container>& containers, int bin_capacity) {
    for (int i = 0; i < (int)containers.size(); ++i) {
        Container& a = containers[i];
        if (is_excluded(a, bin_capacity)) continue;

        auto min_a = min_element(a.items.begin(), a.items.end());
        if (min_a == a.items.end()) continue;
        int itemA = *min_a;

        for (int j = i + 1; j < (int)containers.size(); ++j) {
            Container& b = containers[j];
            if (is_excluded(b, bin_capacity)) continue;

            auto min_b = min_element(b.items.begin(), b.items.end());
            if (min_b == b.items.end()) continue;
            int itemB = *min_b;

            int new_a_cap = a.capacidade_atual - itemA + itemB;
            int new_b_cap = b.capacidade_atual - itemB + itemA;

            if (new_a_cap <= bin_capacity && new_b_cap <= bin_capacity) {
                int remA = bin_capacity - new_a_cap;
                int remB = bin_capacity - new_b_cap;
                if (remA > 5 && remB > 5) {
                    *min_a = itemB;
                    *min_b = itemA;
                    a.capacidade_atual = new_a_cap;
                    b.capacidade_atual = new_b_cap;
                    return true;
                }
            }
        }
    }
    return false;
}

// Uma execução completa da busca local, retorna o número de containers usados
// (a solução inicial agora é construída de forma GULOSA: itens ordenados
// em ordem decrescente + first-fit, em vez de embaralhamento aleatório)
int executarLocalSearch(const vector<int>& items_originais, int bin_capacity, mt19937& g) {
    vector<int> items = items_originais;
    ordena(items); // ordem decrescente, igual ao algoritmo guloso

    vector<Container> containers;
    Container initial;
    initial.capacidade_total = bin_capacity;
    containers.push_back(initial);

    insere(items, bin_capacity, containers);

    int iterations_without_improvement = 0;

    while (iterations_without_improvement < 10000) {
        int previous_bins = containers.size();

        bool move_made = try_move(containers, bin_capacity);
        bool swap_made = false;

        if (!move_made) {
            swap_made = try_swap(containers, bin_capacity);
        }

        if ((int)containers.size() < previous_bins) {
            iterations_without_improvement = 0;
        } else {
            iterations_without_improvement++;
        }
    }

    return (int)containers.size();
}

int main() {

    // ------------ Arquivo de entrada -----------
    //const string ARQUIVO_ENTRADA = "u120_00.txt";
    //const string ARQUIVO_ENTRADA = "u120_01.txt";
    const string ARQUIVO_ENTRADA = "u120_02.txt";
    //const string ARQUIVO_ENTRADA = "u250_00.txt";
    //const string ARQUIVO_ENTRADA = "u250_01.txt";
    //const string ARQUIVO_ENTRADA = "u250_02.txt";
    //const string ARQUIVO_ENTRADA = "u500_00.txt";
    //const string ARQUIVO_ENTRADA = "u500_01.txt";
    //const string ARQUIVO_ENTRADA = "u500_02.txt";
    //const string ARQUIVO_ENTRADA = "u1000_00.txt";
    //const string ARQUIVO_ENTRADA = "u1000_01.txt";
    //const string ARQUIVO_ENTRADA = "u1000_02.txt";
    // -------------------------------------------------

    vector<int> items_originais;
    ifstream arquivo(ARQUIVO_ENTRADA);
    if (!arquivo.is_open()) {
        cerr << "Erro ao abrir o arquivo!" << endl;
        return 1;
    }

    int bin_capacity, n_elementos, otimo;
    arquivo >> bin_capacity >> n_elementos >> otimo;

    int valor;
    while (arquivo >> valor) {
        items_originais.push_back(valor);
    }
    arquivo.close();

    cout << "Rodando " << REPETICOES << " repeticoes da busca local..." << endl;

    vector<int> resultados_bins;
    vector<double> resultados_tempo;

    for (int rep = 0; rep < REPETICOES; rep++) {
        mt19937 g(42 + rep * 1000); // seed diferente a cada rodada

        double t_inicio = omp_get_wtime();
        int n_containers = executarLocalSearch(items_originais, bin_capacity, g);
        double t_fim = omp_get_wtime();

        resultados_bins.push_back(n_containers);
        resultados_tempo.push_back(t_fim - t_inicio);

        cout << "  Repeticao " << (rep + 1) << "/" << REPETICOES
             << ": " << n_containers << " containers, "
             << (t_fim - t_inicio) << "s" << endl;
    }

    // Estatísticas
    double media_bins  = accumulate(resultados_bins.begin(), resultados_bins.end(), 0.0) / REPETICOES;
    int melhor_bins    = *min_element(resultados_bins.begin(), resultados_bins.end());
    int pior_bins      = *max_element(resultados_bins.begin(), resultados_bins.end());
    double media_tempo = accumulate(resultados_tempo.begin(), resultados_tempo.end(), 0.0) / REPETICOES;
    double melhor_tempo = *min_element(resultados_tempo.begin(), resultados_tempo.end());
    double pior_tempo   = *max_element(resultados_tempo.begin(), resultados_tempo.end());

    cout << "\nInstancia: " << ARQUIVO_ENTRADA << endl;
    cout << "Otimo conhecido: " << otimo << endl;
    cout << "Media de containers: " << media_bins << endl;
    cout << "Melhor (menos containers): " << melhor_bins << endl;
    cout << "Pior (mais containers): " << pior_bins << endl;
    cout << "Tempo medio: " << media_tempo << "s" << endl;
    cout << "Tempo melhor: " << melhor_tempo << "s" << endl;
    cout << "Tempo pior: " << pior_tempo << "s" << endl;

    // ----- Grava em CSV (acumulando entre execuções) -----------
    string csv_nome = "localsearch2_resultados.csv";
    bool csv_novo;
    {
        ifstream teste(csv_nome);
        csv_novo = !teste.is_open();
    }

    ofstream csv(csv_nome, ios::app);
    if (csv.is_open()) {
        if (csv_novo)
            csv << "instancia,otimo,repeticoes,media_bins,melhor_bins,pior_bins,"
                << "media_tempo_s,melhor_tempo_s,pior_tempo_s\n";

        csv << ARQUIVO_ENTRADA << ","
            << otimo << ","
            << REPETICOES << ","
            << media_bins << ","
            << melhor_bins << ","
            << pior_bins << ","
            << media_tempo << ","
            << melhor_tempo << ","
            << pior_tempo << "\n";
        csv.close();
        cout << "\nResultados salvos em: " << csv_nome << endl;
    } else {
        cerr << "Erro ao abrir o CSV de saida." << endl;
    }

    return 0;
}