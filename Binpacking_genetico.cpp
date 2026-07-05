#include <iostream>
#include <vector>
#include <string.h>
#include <fstream>
#include <algorithm>
#include <random>
#include <ctime>
#include <omp.h>
#include <map>
#include <numeric>
#include <sstream>

using namespace std;

// ============================================================
// PARÂMETROS DE EXPERIMENTO — ajuste aqui conforme necessário
// ============================================================
#define REPETICOES 10   // Número de rodadas por combinação de parâmetros

// 7 valores para cada parâmetro
// pop_inicial: todos maiores que o maior k (15), espaçados para cobrir pequeno→grande
const int POPS[]  = {16, 30, 60, 100};

// k (tamanho do torneio): de 2 (pouca pressão seletiva) até 10 (alta pressão)
//   todos menores que o menor pop (16)
const int KS[]    = {3, 5};

// porcentagem do crossover: de 20% a 90%
const int PORCS[] = {20, 35, 50, 70, 90};

const int N_POPS  = sizeof(POPS)  / sizeof(POPS[0]);
const int N_KS    = sizeof(KS)    / sizeof(KS[0]);
const int N_PORCS = sizeof(PORCS) / sizeof(PORCS[0]);
// ============================================================

struct Container
{
    int capacidade_total;
    int capacidade_atual = 0;
    vector<int> itens;
};

struct Solucao
{
    vector<int> itens;
    int fitness;
};

void ordena(vector<int> &p)
{
    for (int i = p.size() - 1; i >= 0; --i)
        for (int j = 0; j < i; j++)
            if (p[j] < p[j + 1])
            {
                int temp = p[j];
                p[j] = p[j + 1];
                p[j + 1] = temp;
            }
}

// Retorna o conjunto de containers montados (para validação)
vector<Container> insereDetalhado(vector<int> &item, int bin_capacity)
{
    vector<Container> c;
    Container initial;
    initial.capacidade_total = bin_capacity;
    c.push_back(initial);
    int colocado = 0;
    for (int i = 0; i < (int)item.size(); i++)
    {
        for (int j = 0; j < (int)c.size(); j++)
        {
            if (c[j].capacidade_atual + item[i] <= bin_capacity)
            {
                c[j].capacidade_atual += item[i];
                c[j].itens.push_back(item[i]);
                colocado = 1;
                break;
            }
        }
        if (colocado == 0)
        {
            Container new_container;
            new_container.capacidade_total = bin_capacity;
            new_container.capacidade_atual += item[i];
            new_container.itens.push_back(item[i]);
            c.push_back(new_container);
        }
        colocado = 0;
    }
    return c;
}

int insere(vector<int> &item, int bin_capacity)
{
    return (int)insereDetalhado(item, bin_capacity).size();
}

// ── Validação da solução ─────────────────────────────────────
// Verifica: (1) nenhum bin excede capacidade; (2) todos os itens
// originais estão presentes na solução exatamente com a mesma frequência.
bool validarSolucao(const Solucao &sol, const vector<int> &items_originais, int bin_capacity)
{
    // Checa tamanho
    if (sol.itens.size() != items_originais.size())
        return false;

    // Checa multiconjunto (mesmos itens, mesmas quantidades)
    map<int, int> freq_orig, freq_sol;
    for (int x : items_originais) freq_orig[x]++;
    for (int x : sol.itens)       freq_sol[x]++;
    if (freq_orig != freq_sol)
        return false;

    // Monta os bins e verifica capacidade
    vector<int> temp = sol.itens;
    vector<Container> bins = insereDetalhado(temp, bin_capacity);
    for (const auto &b : bins)
        if (b.capacidade_atual > bin_capacity)
            return false;

    return true;
}

// ── População inicial ────────────────────────────────────────
vector<int> embaralharTresPartes(const vector<int> &vetor, mt19937 &rng)
{
    vector<int> resultado = vetor;
    int tamanho = resultado.size();
    if (tamanho <= 1) return resultado;

    int fim_primeira = tamanho / 3;
    int fim_segunda  = (2 * tamanho) / 3;

    if (fim_primeira > 1)
        shuffle(resultado.begin(), resultado.begin() + fim_primeira, rng);
    if (fim_segunda - fim_primeira > 1)
        shuffle(resultado.begin() + fim_primeira, resultado.begin() + fim_segunda, rng);
    if (tamanho - fim_segunda > 1)
        shuffle(resultado.begin() + fim_segunda, resultado.end(), rng);

    return resultado;
}

void populacao_inicial(vector<Solucao> &s, int pop_ini, vector<int> &items, int bin_capacity, mt19937 &rng)
{
    for (int i = 0; i < pop_ini; ++i)
    {
        Solucao nova_solucao;
        nova_solucao.itens    = embaralharTresPartes(items, rng);
        nova_solucao.fitness  = insere(nova_solucao.itens, bin_capacity);
        s.push_back(nova_solucao);
    }
}

// ── Seleção por torneio ──────────────────────────────────────
int torneioK(const vector<Solucao> &candidatos, const vector<int> &indices, mt19937 &rng)
{
    int menor_fitness = candidatos[indices[0]].fitness;
    for (int i = 1; i < (int)indices.size(); i++)
        if (candidatos[indices[i]].fitness < menor_fitness)
            menor_fitness = candidatos[indices[i]].fitness;

    vector<int> empatados;
    for (int idx : indices)
        if (candidatos[idx].fitness == menor_fitness)
            empatados.push_back(idx);

    if (empatados.size() > 1)
    {
        uniform_int_distribution<int> dist(0, empatados.size() - 1);
        return empatados[dist(rng)];
    }
    return empatados[0];
}

vector<Solucao> selecaoTorneio(const vector<Solucao> &populacao, int k, mt19937 &rng)
{
    vector<Solucao> selecionados;
    int num_torneios = populacao.size();

    for (int i = 0; i < num_torneios; i++)
    {
        vector<int> candidatos_indices;
        for (int j = 0; j < (int)populacao.size(); j++)
            candidatos_indices.push_back(j);

        shuffle(candidatos_indices.begin(), candidatos_indices.end(), rng);
        candidatos_indices.resize(k);

        int vencedor_idx = torneioK(populacao, candidatos_indices, rng);
        selecionados.push_back(populacao[vencedor_idx]);
    }
    return selecionados;
}

// ── Crossover ────────────────────────────────────────────────
int contarItem(const vector<int> &vetor, int item)
{
    return count(vetor.begin(), vetor.end(), item);
}
int contarItemOriginal(const vector<int> &original, int item)
{
    return count(original.begin(), original.end(), item);
}

pair<Solucao, Solucao> crossover(const Solucao &pai1, const Solucao &pai2,
                                 const vector<int> &items_originais,
                                 int porcentagem, int bin_capacity, mt19937 &rng)
{
    Solucao filho1, filho2;
    int tamanho = pai1.itens.size();

    int tamanho_segmento = (tamanho * porcentagem) / 500;
    if (tamanho_segmento < 1) tamanho_segmento = 1;

    filho1.itens.clear();
    filho2.itens.clear();

    bool pegar_pai1 = true;
    for (int segmento = 0; segmento < 5; segmento++)
    {
        int inicio = segmento * tamanho_segmento;
        int fim    = min((segmento + 1) * tamanho_segmento, tamanho);

        if (pegar_pai1)
        {
            for (int i = inicio; i < fim; i++)
            {
                if (contarItem(filho1.itens, pai1.itens[i]) < contarItemOriginal(items_originais, pai1.itens[i]))
                    filho1.itens.push_back(pai1.itens[i]);
                if (contarItem(filho2.itens, pai2.itens[i]) < contarItemOriginal(items_originais, pai2.itens[i]))
                    filho2.itens.push_back(pai2.itens[i]);
            }
        }
        else
        {
            for (int i = inicio; i < fim; i++)
            {
                if (contarItem(filho1.itens, pai2.itens[i]) < contarItemOriginal(items_originais, pai2.itens[i]))
                    filho1.itens.push_back(pai2.itens[i]);
                if (contarItem(filho2.itens, pai1.itens[i]) < contarItemOriginal(items_originais, pai1.itens[i]))
                    filho2.itens.push_back(pai1.itens[i]);
            }
        }
        pegar_pai1 = !pegar_pai1;
    }

    // Preenche itens faltantes em ordem decrescente
    vector<int> items_ordenados = items_originais;
    sort(items_ordenados.begin(), items_ordenados.end(), greater<int>());

    for (int item : items_ordenados)
    {
        int qtd_orig = contarItemOriginal(items_originais, item);
        for (int i = 0; i < qtd_orig - contarItem(filho1.itens, item); i++)
            filho1.itens.push_back(item);
        for (int i = 0; i < qtd_orig - contarItem(filho2.itens, item); i++)
            filho2.itens.push_back(item);
    }

    vector<int> temp1 = filho1.itens, temp2 = filho2.itens;
    filho1.fitness = insere(temp1, bin_capacity);
    filho2.fitness = insere(temp2, bin_capacity);

    return make_pair(filho1, filho2);
}

vector<Solucao> realizarCruzamentos(const vector<Solucao> &selecionados,
                                    const vector<int> &items_originais,
                                    int porcentagem, int bin_capacity, mt19937 &rng)
{
    vector<Solucao> filhos;
    for (int i = 0; i < (int)selecionados.size() - 1; i += 2)
    {
        auto resultado = crossover(selecionados[i], selecionados[i + 1],
                                   items_originais, porcentagem, bin_capacity, rng);
        filhos.push_back(resultado.first);
        filhos.push_back(resultado.second);
    }
    return filhos;
}

// ── Mutação ──────────────────────────────────────────────────
Solucao mutacao(const Solucao &solucao_original, int bin_capacity, mt19937 &rng)
{
    Solucao solucao_mutada = solucao_original;
    int tamanho = solucao_mutada.itens.size();
    if (tamanho <= 1) return solucao_original;

    int fim_primeira = tamanho / 3;
    int fim_segunda  = (2 * tamanho) / 3;

    uniform_int_distribution<int> dist(0, 2);
    int parte_escolhida = dist(rng);

    switch (parte_escolhida)
    {
    case 0:
        if (fim_primeira > 1)
            reverse(solucao_mutada.itens.begin(), solucao_mutada.itens.begin() + fim_primeira);
        break;
    case 1:
        if (fim_segunda - fim_primeira > 1)
            reverse(solucao_mutada.itens.begin() + fim_primeira, solucao_mutada.itens.begin() + fim_segunda);
        break;
    case 2:
        if (tamanho - fim_segunda > 1)
            reverse(solucao_mutada.itens.begin() + fim_segunda, solucao_mutada.itens.end());
        break;
    }

    vector<int> temp = solucao_mutada.itens;
    solucao_mutada.fitness = insere(temp, bin_capacity);

    return (solucao_mutada.fitness < solucao_original.fitness) ? solucao_mutada : solucao_original;
}

// ── Uma execução completa do AG ──────────────────────────────
// Retorna a melhor solução encontrada
Solucao executarAG(vector<int> &items, int bin_capacity,
                   int pop_ini, int k, int porcentagem, mt19937 &rng)
{
    vector<Solucao> populacao;
    populacao_inicial(populacao, pop_ini, items, bin_capacity, rng);

    int iter_sem_melhoria = 0;
    int trocou = 0;

    while (iter_sem_melhoria < 100)
    {
        iter_sem_melhoria++;

        vector<Solucao> selecionados = selecaoTorneio(populacao, k, rng);
        vector<Solucao> filhos = realizarCruzamentos(selecionados, items, porcentagem, bin_capacity, rng);

        for (int i = 0; i < (int)filhos.size(); i++)
            filhos[i] = mutacao(filhos[i], bin_capacity, rng);

        // Substituição elitista: filho melhora alguém da população
        for (int i = 0; i < (int)filhos.size(); i++)
        {
            for (int j = 0; j < (int)populacao.size(); j++)
            {
                if (filhos[i].fitness < populacao[j].fitness)
                {
                    populacao[j] = filhos[i];
                    iter_sem_melhoria = 0;
                    trocou = 1;
                    break;
                }
            }
        }

        // Substituição por diversidade (mesmo fitness, troca aleatória)
        if (trocou == 0)
        {
            for (int i = 0; i < (int)filhos.size(); i++)
            {
                vector<int> pop_idx;
                for (int j = 0; j < (int)populacao.size(); j++)
                    pop_idx.push_back(j);

                shuffle(pop_idx.begin(), pop_idx.end(), rng);
                pop_idx.resize(1);

                if (filhos[i].fitness == populacao[pop_idx[0]].fitness)
                {
                    populacao[pop_idx[0]] = filhos[i];
                    break;
                }
            }
        }
        trocou = 0;
    }

    // Extrai melhor solução da população final
    Solucao melhor = populacao[0];
    for (auto &s : populacao)
        if (s.fitness < melhor.fitness)
            melhor = s;

    return melhor;
}

// ── Nome do arquivo CSV baseado no arquivo de entrada ────────
string nomeCsv(const string &arquivo_entrada)
{
    // Remove extensão e adiciona sufixo
    size_t pos = arquivo_entrada.rfind('.');
    string base = (pos != string::npos) ? arquivo_entrada.substr(0, pos) : arquivo_entrada;
    return base + "_resultados.csv";
}

int main()
{
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

    vector<int> items;
    ifstream arquivo(ARQUIVO_ENTRADA);
    if (!arquivo.is_open())
    {
        cerr << "Erro ao abrir o arquivo: " << ARQUIVO_ENTRADA << endl;
        return 1;
    }

    int bin_capacity, n_elementos, otimo;
    arquivo >> bin_capacity >> n_elementos >> otimo;

    int valor;
    while (arquivo >> valor)
        items.push_back(valor);
    arquivo.close();

    ordena(items);

    // ---- Abre (ou reabre em append) o CSV --------------
    const string csv_nome = nomeCsv(ARQUIVO_ENTRADA);
    bool csv_novo = false;
    {
        ifstream teste(csv_nome);
        csv_novo = !teste.is_open();
    }

    ofstream csv(csv_nome, ios::app);
    if (!csv.is_open())
    {
        cerr << "Erro ao abrir o CSV de saída." << endl;
        return 1;
    }

    // Cabeçalho apenas na primeira vez que o arquivo é criado
    if (csv_novo)
        csv << "instancia,otimo,pop_inicial,k,porcentagem,"
            << "media_bins,melhor_bins,pior_bins,"
            << "media_tempo_s,validos,invalidos\n";

    // ---- Combinações de parâmetros --------------
    int total_combinacoes = N_POPS * N_KS * N_PORCS;
    int combinacao_atual  = 0;

    for (int pi = 0; pi < N_POPS; pi++)
    for (int ki = 0; ki < N_KS;   ki++)
    for (int pc = 0; pc < N_PORCS; pc++)
    {
        int pop_ini    = POPS[pi];
        int k          = KS[ki];
        int porcentagem = PORCS[pc];
        combinacao_atual++;

        // k deve ser menor que pop_inicial (garantia)
        if (k >= pop_ini)
        {
            cout << "[SKIP] pop=" << pop_ini << " k=" << k
                 << " porc=" << porcentagem << " (k >= pop, combinação inválida)\n";
            continue;
        }

        cout << "[" << combinacao_atual << "/" << total_combinacoes << "] "
             << "pop=" << pop_ini << " k=" << k << " porc=" << porcentagem
             << " - rodando " << REPETICOES << " repeticoes..." << flush;

        vector<int> resultados_bins;
        vector<double> resultados_tempo;
        int validos   = 0;
        int invalidos = 0;

        for (int rep = 0; rep < REPETICOES; rep++)
        {
            // Seed diferente a cada rodada para garantir diversidade
            mt19937 rng(42 + rep * 1000 + pi * 10000 + ki * 100000 + pc * 1000000);

            double t_inicio = omp_get_wtime();
            Solucao melhor = executarAG(items, bin_capacity, pop_ini, k, porcentagem, rng);
            double t_fim    = omp_get_wtime();

            resultados_bins.push_back(melhor.fitness);
            resultados_tempo.push_back(t_fim - t_inicio);

            if (validarSolucao(melhor, items, bin_capacity))
                validos++;
            else
                invalidos++;
        }

        // Calcula estatísticas
        double media_bins = accumulate(resultados_bins.begin(), resultados_bins.end(), 0.0) / REPETICOES;
        int melhor_bins   = *min_element(resultados_bins.begin(), resultados_bins.end());
        int pior_bins     = *max_element(resultados_bins.begin(), resultados_bins.end());
        double media_tempo = accumulate(resultados_tempo.begin(), resultados_tempo.end(), 0.0) / REPETICOES;

        // Grava no CSV
        csv << ARQUIVO_ENTRADA << ","
            << otimo << ","
            << pop_ini << ","
            << k << ","
            << porcentagem << ","
            << media_bins << ","
            << melhor_bins << ","
            << pior_bins << ","
            << media_tempo << ","
            << validos << ","
            << invalidos << "\n";
        csv.flush(); // Garante que cada linha é gravada imediatamente

        cout << " media=" << media_bins
             << " melhor=" << melhor_bins
             << " tempo=" << media_tempo << "s"
             << " validos=" << validos << "/" << REPETICOES << "\n";
    }

    csv.close();
    cout << "\nPronto! Resultados salvos em: " << csv_nome << endl;
    cout << "Otimo conhecido: " << otimo << endl;
    return 0;
}