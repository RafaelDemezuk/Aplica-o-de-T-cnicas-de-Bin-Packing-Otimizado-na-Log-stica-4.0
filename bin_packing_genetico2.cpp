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

using namespace std;

// ============================================================
// PARÂMETROS DE EXPERIMENTO
// ============================================================
#define REPETICOES 10

const int POPS[]  = {16, 30, 60, 100};
const int KS[]    = {3, 5};
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
{ // calculo da media do tempo de espera
    for (int i = p.size() - 1; i >= 0; --i)
    {
        for (int j = 0; j < i; j++)
        {
            if (p[j] < p[j + 1])
            {
                int temp = p[j];
                p[j] = p[j + 1];
                p[j + 1] = temp;
            }
        }
    }
}
// Insere os itens nos containers
// e retorna quantidade de bins utilizados
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

bool validarSolucao(const Solucao &sol, const vector<int> &items_originais, int bin_capacity)
{
    if (sol.itens.size() != items_originais.size())
        return false;
    map<int,int> freq_orig, freq_sol;
    for (int x : items_originais) freq_orig[x]++;
    for (int x : sol.itens)       freq_sol[x]++;
    if (freq_orig != freq_sol) return false;
    vector<int> temp = sol.itens;
    for (const auto &b : insereDetalhado(temp, bin_capacity))
        if (b.capacidade_atual > bin_capacity) return false;
    return true;
}
vector<int> embaralharTresPartes(const vector<int> &vetor, mt19937 &rng)
{
    vector<int> resultado = vetor; // Cria uma cópia do vetor original
    int tamanho = resultado.size();

    if (tamanho <= 1)
        return resultado; // Não há necessidade de embaralhar

    // Calcula os índices das três partes
    int fim_primeira = tamanho / 3;      // 33% do início
    int fim_segunda = (2 * tamanho) / 3; // 33% do meio
    // A terceira parte vai de fim_segunda até o final (34% restante)

    // Embaralha a primeira parte (0 até fim_primeira-1)
    if (fim_primeira > 1)
    {
        shuffle(resultado.begin(), resultado.begin() + fim_primeira, rng);
    }

    // Embaralha a segunda parte (fim_primeira até fim_segunda-1)
    if (fim_segunda - fim_primeira > 1)
    {
        shuffle(resultado.begin() + fim_primeira, resultado.begin() + fim_segunda, rng);
    }

    // Embaralha a terceira parte (fim_segunda até o final)
    if (tamanho - fim_segunda > 1)
    {
        shuffle(resultado.begin() + fim_segunda, resultado.end(), rng);
    }

    return resultado;
}

// Função de mutação que inverte uma das três partes do vetor
Solucao mutacao(const Solucao &solucao_original, int bin_capacity, mt19937 &rng)
{
    Solucao solucao_mutada = solucao_original; // Cria uma cópia da solução original
    int tamanho = solucao_mutada.itens.size();

    if (tamanho <= 1)
        return solucao_original; // Não há necessidade de mutar

    // Calcula os índices das três partes (mesma lógica do embaralharTresPartes)
    int fim_primeira = tamanho / 3;      // 33% do início
    int fim_segunda = (2 * tamanho) / 3; // 33% do meio
    // A terceira parte vai de fim_segunda até o final (34% restante)

    // Escolhe aleatoriamente uma das três partes para inverter
    uniform_int_distribution<int> dist(0, 2); // 0, 1 ou 2
    int parte_escolhida = dist(rng);

    // Inverte a parte escolhida
    switch (parte_escolhida)
    {
    case 0: // Primeira parte (0 até fim_primeira-1)
        if (fim_primeira > 1)
        {
            reverse(solucao_mutada.itens.begin(), solucao_mutada.itens.begin() + fim_primeira);
        }
        break;

    case 1: // Segunda parte (fim_primeira até fim_segunda-1)
        if (fim_segunda - fim_primeira > 1)
        {
            reverse(solucao_mutada.itens.begin() + fim_primeira, solucao_mutada.itens.begin() + fim_segunda);
        }
        break;

    case 2: // Terceira parte (fim_segunda até o final)
        if (tamanho - fim_segunda > 1)
        {
            reverse(solucao_mutada.itens.begin() + fim_segunda, solucao_mutada.itens.end());
        }
        break;
    }

    // Calcula o fitness da solução mutada
    vector<int> temp = solucao_mutada.itens;
    solucao_mutada.fitness = insere(temp, bin_capacity);

    // Retorna a melhor solução (original ou mutada)
    if (solucao_mutada.fitness < solucao_original.fitness)
    {
        return solucao_mutada; // Mutação melhorou
    }
    else
    {
        return solucao_original; // Mantém a original
    }
}

// Função para gerar uma solução com ordenação crescente (pior caso para bin packing)
Solucao gerarSolucaoRuimCrescente(vector<int> items, int bin_capacity) {
    Solucao solucao;
    
    // Ordena os itens em ordem crescente (geralmente péssimo para bin packing)
    sort(items.begin(), items.end());
    solucao.itens = items;
    
    // Calcula o fitness
    vector<int> temp = solucao.itens;
    solucao.fitness = insere(temp, bin_capacity);
    
    return solucao;
}

// Função para intercalar itens grandes e pequenos (fragmentação máxima)
Solucao gerarSolucaoIntercalada(vector<int> items, int bin_capacity) {
    Solucao solucao;
    
    // Separa itens grandes e pequenos
    sort(items.begin(), items.end(), greater<int>());
    
    vector<int> grandes, pequenos;
    int meio = items.size() / 2;
    
    for (int i = 0; i < meio; i++) {
        grandes.push_back(items[i]);
    }
    for (int i = meio; i < items.size(); i++) {
        pequenos.push_back(items[i]);
    }
    
    // Intercala: grande, pequeno, grande, pequeno...
    solucao.itens.clear();
    int max_size = max(grandes.size(), pequenos.size());
    
    for (int i = 0; i < max_size; i++) {
        if (i < grandes.size()) {
            solucao.itens.push_back(grandes[i]);
        }
        if (i < pequenos.size()) {
            solucao.itens.push_back(pequenos[i]);
        }
    }
    
    // Calcula o fitness
    vector<int> temp = solucao.itens;
    solucao.fitness = insere(temp, bin_capacity);
    
    return solucao;
}

// Função para gerar sequência com padrão ruim específico
Solucao gerarSolucaoSequenciaRuim(vector<int> items, int bin_capacity, mt19937 &rng) {
    Solucao solucao;
    
    // Ordena crescente primeiro
    sort(items.begin(), items.end());
    
    // Inverte pequenos blocos aleatórios para criar desordem máxima
    int tamanho = items.size();
    int num_blocos = tamanho / 3;
    
    for (int i = 0; i < num_blocos; i++) {
        int inicio = (i * 3) % tamanho;
        int fim = min(inicio + 3, tamanho);
        reverse(items.begin() + inicio, items.begin() + fim);
    }
    
    solucao.itens = items;
    
    // Calcula o fitness
    vector<int> temp = solucao.itens;
    solucao.fitness = insere(temp, bin_capacity);
    
    return solucao;
}

// Função para dispersar itens grandes (anti-agrupamento)
Solucao gerarSolucaoDispersaGrandes(vector<int> items, int bin_capacity, mt19937 &rng) {
    Solucao solucao;
    
    // Identifica itens grandes (acima da média)
    int soma = 0;
    for (int item : items) soma += item;
    int media = soma / items.size();
    
    vector<int> grandes, pequenos;
    for (int item : items) {
        if (item > media) {
            grandes.push_back(item);
        } else {
            pequenos.push_back(item);
        }
    }
    
    // Embaralha ambos os grupos
    shuffle(grandes.begin(), grandes.end(), rng);
    shuffle(pequenos.begin(), pequenos.end(), rng);
    
    // Distribui os grandes de forma espaçada entre os pequenos
    solucao.itens = pequenos;
    
    if (!grandes.empty()) {
        int espacamento = max(1, (int)pequenos.size() / (int)grandes.size());
        
        for (int i = 0; i < grandes.size(); i++) {
            int pos = min((i * espacamento), (int)solucao.itens.size());
            solucao.itens.insert(solucao.itens.begin() + pos, grandes[i]);
        }
    }
    
    // Calcula o fitness
    vector<int> temp = solucao.itens;
    solucao.fitness = insere(temp, bin_capacity);
    
    return solucao;
}

// Função para criar blocos de mesmo tamanho (evita complementaridade)
Solucao gerarSolucaoGruposSimilares(vector<int> items, int bin_capacity) {
    Solucao solucao;
    
    // Agrupa itens por tamanho similar
    sort(items.begin(), items.end());
    
    // Cria blocos de itens similares
    vector<vector<int>> grupos;
    if (!items.empty()) {
        vector<int> grupo_atual;
        grupo_atual.push_back(items[0]);
        
        for (int i = 1; i < items.size(); i++) {
            // Se a diferença for pequena, adiciona ao grupo atual
            if (abs(items[i] - grupo_atual.back()) <= 2) {
                grupo_atual.push_back(items[i]);
            } else {
                // Senão, inicia novo grupo
                grupos.push_back(grupo_atual);
                grupo_atual.clear();
                grupo_atual.push_back(items[i]);
            }
        }
        grupos.push_back(grupo_atual);
    }
    
    // Coloca todos os grupos juntos (evita mistura que poderia ser boa)
    solucao.itens.clear();
    for (auto& grupo : grupos) {
        for (int item : grupo) {
            solucao.itens.push_back(item);
        }
    }
    
    // Calcula o fitness
    vector<int> temp = solucao.itens;
    solucao.fitness = insere(temp, bin_capacity);
    
    return solucao;
}

// Função para população inicial com soluções INTENCIONALMENTE RUINS
void populacao_inicial(vector<Solucao> &s, int pop_inicial, vector<int> &items, int bin_capacity, mt19937 &rng) {

    // Garante estratégias ruins específicas
    int estrategias_ruins = min(5, pop_inicial);

    // 1. Ordenação crescente (péssima para bin packing)
    if ((int)s.size() < pop_inicial) {
        s.push_back(gerarSolucaoRuimCrescente(items, bin_capacity));
    }

    // 2. Intercalação grande-pequeno (fragmentação máxima)
    if ((int)s.size() < pop_inicial) {
        s.push_back(gerarSolucaoIntercalada(items, bin_capacity));
    }

    // 3. Sequência com padrão ruim
    if ((int)s.size() < pop_inicial) {
        s.push_back(gerarSolucaoSequenciaRuim(items, bin_capacity, rng));
    }

    // 4. Dispersão de itens grandes
    if ((int)s.size() < pop_inicial) {
        s.push_back(gerarSolucaoDispersaGrandes(items, bin_capacity, rng));
    }

    // 5. Grupos de itens similares
    if ((int)s.size() < pop_inicial) {
        s.push_back(gerarSolucaoGruposSimilares(items, bin_capacity));
    }

    // Preenche o resto com soluções completamente aleatórias ruins
    for (int i = s.size(); i < pop_inicial; i++) {
        Solucao nova_solucao;
        nova_solucao.itens = items;

        int tipo_ruim = i % 4;

        switch(tipo_ruim) {
            case 0: {
                for (int j = 0; j < (int)items.size() * 2; j++) {
                    uniform_int_distribution<int> dist(0, nova_solucao.itens.size() - 1);
                    int pos1 = dist(rng);
                    int pos2 = dist(rng);
                    swap(nova_solucao.itens[pos1], nova_solucao.itens[pos2]);
                }
                break;
            }
            case 1: {
                sort(nova_solucao.itens.begin(), nova_solucao.itens.end());
                for (int j = 0; j < 5; j++) {
                    uniform_int_distribution<int> dist(0, nova_solucao.itens.size() - 1);
                    int pos1 = dist(rng);
                    int pos2 = dist(rng);
                    swap(nova_solucao.itens[pos1], nova_solucao.itens[pos2]);
                }
                break;
            }
            case 2: {
                sort(nova_solucao.itens.begin(), nova_solucao.itens.end());
                for (int j = 0; j < (int)nova_solucao.itens.size(); j += 4) {
                    int fim = min(j + 4, (int)nova_solucao.itens.size());
                    reverse(nova_solucao.itens.begin() + j, nova_solucao.itens.begin() + fim);
                }
                break;
            }
            case 3: {
                sort(nova_solucao.itens.begin(), nova_solucao.itens.end());
                vector<int> temp = nova_solucao.itens;
                nova_solucao.itens.clear();
                int inicio = 0, fim = temp.size() - 1;
                bool pegar_inicio = true;
                while (inicio <= fim) {
                    if (pegar_inicio) nova_solucao.itens.push_back(temp[inicio++]);
                    else              nova_solucao.itens.push_back(temp[fim--]);
                    pegar_inicio = !pegar_inicio;
                }
                break;
            }
        }

        vector<int> temp = nova_solucao.itens;
        nova_solucao.fitness = insere(temp, bin_capacity);
        s.push_back(nova_solucao);
    }
}
int torneioK(const vector<Solucao> &candidatos, const vector<int> &indices, mt19937 &rng)
{
    // Encontra o menor fitness entre os candidatos
    int menor_fitness = candidatos[indices[0]].fitness;
    for (int i = 1; i < indices.size(); i++)
    {
        if (candidatos[indices[i]].fitness < menor_fitness)
        {
            menor_fitness = candidatos[indices[i]].fitness;
        }
    }

    // Coleta todos os índices com o menor fitness (para caso de empate)
    vector<int> empatados;
    for (int idx : indices)
    {
        if (candidatos[idx].fitness == menor_fitness)
        {
            empatados.push_back(idx);
        }
    }

    // Se há empate, escolhe aleatoriamente entre os empatados
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

    if (populacao.size() < k)
    {
        cout << "Erro: k (" << k << ") maior que o tamanho da população (" << populacao.size() << ")." << endl;
        return selecionados;
    }

    // Realiza número de torneios igual ao tamanho da população
    int num_torneios = populacao.size();

    for (int i = 0; i < num_torneios; i++)
    {
        // Para cada torneio, seleciona k candidatos DIFERENTES aleatoriamente
        vector<int> candidatos_indices;
        for (int j = 0; j < populacao.size(); j++)
        {
            candidatos_indices.push_back(j);
        }

        // Embaralha e pega apenas os k primeiros para este torneio
        shuffle(candidatos_indices.begin(), candidatos_indices.end(), rng);
        candidatos_indices.resize(k);

        // Realiza o torneio entre esses k candidatos
        int vencedor_idx = torneioK(populacao, candidatos_indices, rng);

        // Adiciona o vencedor à lista de selecionados
        selecionados.push_back(populacao[vencedor_idx]);
    }

    return selecionados;
}
int contarItem(const vector<int> &vetor, int item)
{
    return count(vetor.begin(), vetor.end(), item);
}

// Função auxiliar para contar ocorrências de um item no vetor original
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

    // Calcula o tamanho de cada segmento (10% do tamanho total para 50%)
    int tamanho_segmento = (tamanho * porcentagem) / 500; // 500 = 100 * 5 (5 segmentos)
    if (tamanho_segmento < 1)
        tamanho_segmento = 1;

    // Inicializa os filhos
    filho1.itens.clear();
    filho2.itens.clear();

    // Faz os 5 cortes alternando
    bool pegar_pai1 = true; // Começa pegando do pai1

    for (int segmento = 0; segmento < 5; segmento++)
    {
        int inicio = segmento * tamanho_segmento;
        int fim = min((segmento + 1) * tamanho_segmento, tamanho);

        if (pegar_pai1)
        {
            // Filho1 pega do pai1, filho2 pega do pai2
            for (int i = inicio; i < fim; i++)
            {
                // Verifica se pode adicionar o item (sem exceder quantidade original)
                if (contarItem(filho1.itens, pai1.itens[i]) < contarItemOriginal(items_originais, pai1.itens[i]))
                {
                    filho1.itens.push_back(pai1.itens[i]);
                }
                if (contarItem(filho2.itens, pai2.itens[i]) < contarItemOriginal(items_originais, pai2.itens[i]))
                {
                    filho2.itens.push_back(pai2.itens[i]);
                }
            }
        }
        else
        {
            // Filho1 pega do pai2, filho2 pega do pai1
            for (int i = inicio; i < fim; i++)
            {
                if (contarItem(filho1.itens, pai2.itens[i]) < contarItemOriginal(items_originais, pai2.itens[i]))
                {
                    filho1.itens.push_back(pai2.itens[i]);
                }
                if (contarItem(filho2.itens, pai1.itens[i]) < contarItemOriginal(items_originais, pai1.itens[i]))
                {
                    filho2.itens.push_back(pai1.itens[i]);
                }
            }
        }

        pegar_pai1 = !pegar_pai1; // Alterna para o próximo segmento
    }

    // Preenche os itens faltantes em ordem decrescente
    vector<int> items_ordenados = items_originais;
    sort(items_ordenados.begin(), items_ordenados.end(), greater<int>());

    // Preenche filho1
    for (int item : items_ordenados)
    {
        int quantidade_original = contarItemOriginal(items_originais, item);
        int quantidade_atual = contarItem(filho1.itens, item);

        for (int i = 0; i < quantidade_original - quantidade_atual; i++)
        {
            filho1.itens.push_back(item);
        }
    }

    // Preenche filho2
    for (int item : items_ordenados)
    {
        int quantidade_original = contarItemOriginal(items_originais, item);
        int quantidade_atual = contarItem(filho2.itens, item);

        for (int i = 0; i < quantidade_original - quantidade_atual; i++)
        {
            filho2.itens.push_back(item);
        }
    }

    // Calcula o fitness dos filhos
    vector<int> temp1 = filho1.itens;
    vector<int> temp2 = filho2.itens;
    filho1.fitness = insere(temp1, bin_capacity);
    filho2.fitness = insere(temp2, bin_capacity);

    return make_pair(filho1, filho2);
}

// Função principal que realiza todos os cruzamentos
vector<Solucao> realizarCruzamentos(const vector<Solucao> &selecionados,
                                    const vector<int> &items_originais,
                                    int porcentagem, int bin_capacity, mt19937 &rng)
{

    vector<Solucao> filhos;

    // Cruza os selecionados de dois em dois (1º com 2º, 3º com 4º, etc.)
    for (int i = 0; i < selecionados.size() - 1; i += 2)
    {
        pair<Solucao, Solucao> resultado = crossover(selecionados[i], selecionados[i + 1],
                                                     items_originais, porcentagem, bin_capacity, rng);

        filhos.push_back(resultado.first);
        filhos.push_back(resultado.second);
    }

    // Se número ímpar de selecionados, o último não cruza
    if (selecionados.size() % 2 == 1)
    {
        cout << "Aviso: Número ímpar de selecionados. Último indivíduo não cruzou." << endl;
    }

    return filhos;
}

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

    Solucao melhor = populacao[0];
    for (auto &s : populacao)
        if (s.fitness < melhor.fitness)
            melhor = s;

    return melhor;
}

string nomeCsv(const string &arquivo_entrada)
{
    size_t pos = arquivo_entrada.rfind('.');
    string base = (pos != string::npos) ? arquivo_entrada.substr(0, pos) : arquivo_entrada;
    return base + "_resultados_popruim.csv";
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

    if (csv_novo)
        csv << "instancia,otimo,pop_inicial,k,porcentagem,"
            << "media_bins,melhor_bins,pior_bins,"
            << "media_tempo_s,validos,invalidos\n";

    int total_combinacoes = N_POPS * N_KS * N_PORCS;
    int combinacao_atual  = 0;

    for (int pi = 0; pi < N_POPS; pi++)
    for (int ki = 0; ki < N_KS;   ki++)
    for (int pc = 0; pc < N_PORCS; pc++)
    {
        int pop_ini     = POPS[pi];
        int k           = KS[ki];
        int porcentagem = PORCS[pc];
        combinacao_atual++;

        if (k >= pop_ini)
        {
            cout << "[SKIP] pop=" << pop_ini << " k=" << k
                 << " porc=" << porcentagem << " (k >= pop)\n";
            continue;
        }

        cout << "[" << combinacao_atual << "/" << total_combinacoes << "] "
             << "pop=" << pop_ini << " k=" << k << " porc=" << porcentagem
             << " = rodando " << REPETICOES << " repeticoes..." << flush;

        vector<int> resultados_bins;
        vector<double> resultados_tempo;
        int validos   = 0;
        int invalidos = 0;

        for (int rep = 0; rep < REPETICOES; rep++)
        {
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

        double media_bins  = accumulate(resultados_bins.begin(),  resultados_bins.end(),  0.0) / REPETICOES;
        int    melhor_bins = *min_element(resultados_bins.begin(), resultados_bins.end());
        int    pior_bins   = *max_element(resultados_bins.begin(), resultados_bins.end());
        double media_tempo = accumulate(resultados_tempo.begin(), resultados_tempo.end(), 0.0) / REPETICOES;

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
        csv.flush();

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