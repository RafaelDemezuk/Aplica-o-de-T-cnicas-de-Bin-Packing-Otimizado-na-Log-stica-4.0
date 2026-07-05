#include <iostream>
#include <vector>
#include <string.h>
#include <fstream>
#include <omp.h>
#include <numeric>
#include <algorithm>

using namespace std;

// ============================================================
// PARÂMETROS DE EXPERIMENTO 
// ============================================================
#define REPETICOES 10   // Número de rodadas (o guloso é determinístico,
                         // mas repetimos para obter uma média de tempo estável)
// ============================================================

struct Container {
    int capacidade_total; // capacidade total do container
    int capacidade_atual=0; // capacidade atual do container
};

void ordena(vector<int>& p) {//calculo da media do tempo de espera
    for(int i = p.size()-1; i >= 0; --i) {
        for (int j = 0; j < i; j++) {
            if (p[j] < p[j + 1]) {
                int temp = p[j];
                p[j] = p[j + 1];
                p[j + 1] = temp;
            }
        }
    }
}

void insere(vector<int>& item, int bin_capacity, vector<Container>& c) {
    int colocado = 0;
    for(int i =0; i<item.size(); i++){
        for(int j = 0; j < c.size(); j++){
            if(c[j].capacidade_atual + item[i] <= bin_capacity){
                c[j].capacidade_atual += item[i];
                colocado = 1;
                break;
            }
        }
        if(colocado == 0){
            Container new_container;
            new_container.capacidade_total = bin_capacity;
            c.push_back(new_container);
            c[c.size()-1].capacidade_atual += item[i];
        }
        colocado = 0;
    }
}

// Uma execução completa do guloso, retorna o número de containers usados
int executarGuloso(vector<int>& items, int bin_capacity) {
    vector<Container> containers;
    Container c;
    c.capacidade_total = bin_capacity;
    containers.push_back(c);
    insere(items, bin_capacity, containers);
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
        cerr << "Erro ao abrir o arquivo: " << ARQUIVO_ENTRADA << endl;
        return 1;
    }

    // Ler os três primeiros números (parâmetros)
    int bin_capacity, n_elementos, otimo;
    arquivo >> bin_capacity >> n_elementos >> otimo;

    // Ler os demais valores (itens)
    int valor;
    while (arquivo >> valor) {
        items_originais.push_back(valor);
    }
    arquivo.close();

    ordena(items_originais);

    cout << "Rodando " << REPETICOES << " repeticoes do algoritmo guloso..." << endl;

    vector<int> resultados_bins;
    vector<double> resultados_tempo;

    for (int rep = 0; rep < REPETICOES; rep++) {
        vector<int> items = items_originais; // copia, pois insere() consome o vetor

        double t_inicio = omp_get_wtime();
        int n_containers = executarGuloso(items, bin_capacity);
        double t_fim = omp_get_wtime();

        resultados_bins.push_back(n_containers);
        resultados_tempo.push_back(t_fim - t_inicio);
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
    string csv_nome = "guloso_resultados.csv";
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