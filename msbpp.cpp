#include <iostream>
#include <vector>
#include <fstream>
#include <algorithm>
#include <numeric>
#include <random>
#include <ctime>
#include <cmath>
#include <unordered_map>
#include <deque>

using namespace std;

// --- Parâmetros MSBPP Melhorado ---
int POP_SIZE = 20;
int MAX_IT = 500;
// Probabilidade inicial de predador com decaimento exponencial
const double Pdp_initial = 0.3;
double Pdp_decay_rate = 5.0; // quanto maior, mais rápido decai
// Taxas de seleção iniciais para operadores
const double alpha_initial = 1.0;
const double beta_initial  = 0.5;
const double gamma_initial = 0.1;
int k = 2;
int bin_capacity;

typedef pair<int,int> SwapOp; // (item, target_bin)

struct Solution {
    vector<vector<int>> bins;
    double fitness;
    void calculateFitness() {
        double sum = 0.0;
        for (size_t i = 0; i < bins.size(); ++i) {
            int total = accumulate(bins[i].begin(), bins[i].end(), 0);
            sum += pow(double(total) / bin_capacity, k);
        }
        fitness = 1.0 - sum / bins.size();
    }
};

mt19937 rng(time(nullptr));

// Gera solução FFD
Solution initFFD(const vector<int>& items) {
    Solution sol;
    vector<int> ord = items;
    sort(ord.begin(), ord.end(), greater<int>());
    for (int it : ord) {
        bool placed = false;
        for (auto &b : sol.bins) {
            int used = accumulate(b.begin(), b.end(), 0);
            if (used + it <= bin_capacity) {
                b.push_back(it);
                placed = true;
                break;
            }
        }
        if (!placed) sol.bins.push_back({it});
    }
    sol.calculateFitness();
    return sol;
}

// Gera solução Random Best Fit
Solution initRandomBestFit(const vector<int>& items) {
    Solution sol;
    vector<int> ord = items;
    shuffle(ord.begin(), ord.end(), rng);
    for (int it : ord) {
        int best = -1;
        int minspace = bin_capacity + 1;
        for (int i = 0; i < sol.bins.size(); ++i) {
            int used = accumulate(sol.bins[i].begin(), sol.bins[i].end(), 0);
            int space = bin_capacity - (used + it);
            if (space >= 0 && space < minspace) {
                minspace = space;
                best = i;
            }
        }
        if (best >= 0) sol.bins[best].push_back(it);
        else sol.bins.push_back({it});
    }
    sol.calculateFitness();
    return sol;
}

// Gera solução First Fit
Solution initFirstFit(const vector<int>& items) {
    Solution sol;
    for (int it : items) {
        bool placed = false;
        for (auto &b : sol.bins) {
            int used = accumulate(b.begin(), b.end(), 0);
            if (used + it <= bin_capacity) {
                b.push_back(it);
                placed = true;
                break;
            }
        }
        if (!placed) sol.bins.push_back({it});
    }
    sol.calculateFitness();
    return sol;
}

// Diferença entre soluções
vector<SwapOp> diff(const Solution &A, const Solution &B) {
    unordered_map<int,int> mA, mB;
    for (int i = 0; i < A.bins.size(); ++i)
        for (int v : A.bins[i]) mA[v] = i;
    for (int i = 0; i < B.bins.size(); ++i)
        for (int v : B.bins[i]) mB[v] = i;
    vector<SwapOp> ops;
    for (auto &p : mB) {
        int item = p.first;
        int target = p.second;
        if (mA[item] != target) ops.emplace_back(item, target);
    }
    return ops;
}

// Seleciona ops com probabilidade rate
vector<SwapOp> selectOps(const vector<SwapOp> &ops, double rate) {
    vector<SwapOp> sel;
    uniform_real_distribution<double> dist(0.0, 1.0);
    for (auto &op : ops) {
        if (dist(rng) <= rate) sel.push_back(op);
    }
    return sel;
}

// Aplica swaps
void applyOps(Solution &s, const vector<SwapOp> &ops) {
    unordered_map<int,int> loc;
    for (int i = 0; i < s.bins.size(); ++i)
        for (int v : s.bins[i]) loc[v] = i;
    for (auto &op : ops) {
        int it = op.first;
        int nb = op.second;
        int ob = loc[it];
        if (nb >= s.bins.size() || nb == ob) continue;
        int used = accumulate(s.bins[nb].begin(), s.bins[nb].end(), 0);
        if (used + it <= bin_capacity) {
            auto &from = s.bins[ob];
            from.erase(find(from.begin(), from.end(), it));
            s.bins[nb].push_back(it);
        }
    }
    s.bins.erase(remove_if(s.bins.begin(), s.bins.end(), [](const vector<int> &b){ return b.empty(); }), s.bins.end());
    s.calculateFitness();
}

bool tryMove(Solution &s) {
    for (int i = 0; i < s.bins.size(); ++i) {
        for (int j = 0; j < s.bins.size(); ++j) {
            if (i == j) continue;
            for (int k = 0; k < s.bins[i].size(); ++k) {
                int it = s.bins[i][k];
                int used = accumulate(s.bins[j].begin(), s.bins[j].end(), 0);
                if (used + it <= bin_capacity) {
                    s.bins[j].push_back(it);
                    s.bins[i].erase(s.bins[i].begin() + k);
                    s.bins.erase(remove_if(s.bins.begin(), s.bins.end(), [](const vector<int> &b){ return b.empty(); }), s.bins.end());
                    s.calculateFitness();
                    return true;
                }
            }
        }
    }
    return false;
}
bool trySwap(Solution &s) {
    for (int i = 0; i < s.bins.size(); ++i) {
        for (int j = i + 1; j < s.bins.size(); ++j) {
            for (int ii = 0; ii < s.bins[i].size(); ++ii) {
                for (int jj = 0; jj < s.bins[j].size(); ++jj) {
                    int a = s.bins[i][ii];
                    int b = s.bins[j][jj];
                    int sum_i = accumulate(s.bins[i].begin(), s.bins[i].end(), 0);
                    int sum_j = accumulate(s.bins[j].begin(), s.bins[j].end(), 0);
                    if (sum_i - a + b <= bin_capacity && sum_j - b + a <= bin_capacity) {
                        swap(s.bins[i][ii], s.bins[j][jj]);
                        s.calculateFitness();
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

void localSearchIntense(Solution &s) {
    for (int i = 0; i < 50; ++i) {
        if (!tryMove(s) && !trySwap(s)) break;
    }
}
void shaking(Solution &s, const vector<int> &items) {
    if (s.bins.empty()) return;
    uniform_int_distribution<int> dist(0, s.bins.size()-1);
    int idx = dist(rng);
    vector<int> rem = s.bins[idx];
    s.bins.erase(s.bins.begin() + idx);
    for (int it : rem) {
        bool placed = false;
        for (auto &b : s.bins) {
            int used = accumulate(b.begin(), b.end(), 0);
            if (used + it <= bin_capacity) {
                b.push_back(it);
                placed = true;
                break;
            }
        }
        if (!placed) s.bins.push_back({it});
    }
    s.calculateFitness();
}

void MSBPP(const vector<int> &items) {
    vector<Solution> pop;
    pop.reserve(POP_SIZE);
    for (int i = 0; i < POP_SIZE; ++i) {
        if (i < POP_SIZE/3) pop.push_back(initFFD(items));
        else if (i < 2*POP_SIZE/3) pop.push_back(initRandomBestFit(items));
        else pop.push_back(initFirstFit(items));
    }
    Solution best = pop[0];
    for (int i = 1; i < POP_SIZE; ++i)
        if (pop[i].fitness < best.fitness) best = pop[i];

    deque<vector<SwapOp>> tabuList;

    for (int t = 1; t <= MAX_IT; ++t) {
        double Pdp = Pdp_initial * exp(-Pdp_decay_rate * double(t)/MAX_IT);
        double alpha = alpha_initial * (1.0 - double(t)/MAX_IT);

        sort(pop.begin(), pop.end(), [](const Solution &a, const Solution &b){ return a.fitness < b.fitness; });
        vector<Solution> nextGen;
        for (int i = 0; i < 4; ++i)
            nextGen.push_back(pop[i]);

        for (int i = 4; i < POP_SIZE; ++i) {
            Solution s = pop[i];
            double r = uniform_real_distribution<double>(0,1)(rng);
            if (r > Pdp) {
                auto opsAll = diff(s, best);
                auto ops    = selectOps(opsAll, alpha);
                vector<SwapOp> filtered;
                for (auto &op : ops) {
                    bool forbidden = false;
                    for (auto &tb : tabuList) {
                        if (find(tb.begin(), tb.end(), op) != tb.end()) {
                            forbidden = true;
                            break;
                        }
                    }
                    if (!forbidden) filtered.push_back(op);
                }
                applyOps(s, filtered);
                localSearchIntense(s);
                tabuList.push_back(filtered);
                if (tabuList.size() > 50) tabuList.pop_front();
            } else {
                shaking(s, items);
            }
            if (s.fitness < best.fitness) best = s;
            nextGen.push_back(s);
        }
        pop.swap(nextGen);
    }

    cout << "Fitness= " << best.fitness << "  Bins= " << best.bins.size() << "\n";
}

int main() {
    ifstream f("u500_00.txt");
    if (!f.is_open()) {
        cerr << "Erro ao abrir arquivo!\n";
        return 1;
    }
    int n, opt;
    f >> bin_capacity >> n >> opt;
    vector<int> items(n);
    for (int i = 0; i < n; ++i) f >> items[i];
    f.close();

    MSBPP(items);
    cout << "Ótimo esperado= " << opt << "\n";
    return 0;
}
