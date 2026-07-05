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

// --- Parâmetros MSBPP Aprimorado ---
int POP_SIZE = 20;
int MAX_IT = 500;
const double Pdp_initial = 0.3;
double Pdp_decay_rate = 5.0;   // taxa de decaimento exponencial
const double alpha_initial = 1.0;
const double beta_initial  = 0.5;
const double gamma_initial = 0.1;
int k = 2;
int bin_capacity;

typedef pair<int,int> SwapOp; // (item, bin alvo)

mt19937 rng(time(nullptr));

struct Solution {
    vector<vector<int>> bins;
    double fitness;
    void calculateFitness() {
        double sum = 0.0;
        for (auto &b : bins) {
            int total = accumulate(b.begin(), b.end(), 0);
            sum += pow(double(total) / bin_capacity, k);
        }
        fitness = 1.0 - sum / bins.size();
    }
};

// Heurísticas de Inicialização --------------------------------
Solution initFFD(const vector<int>& items) {
    Solution s;
    vector<int> ord = items;
    sort(ord.begin(), ord.end(), greater<int>());
    for (int it : ord) {
        bool placed = false;
        for (auto &b : s.bins) {
            int used = accumulate(b.begin(), b.end(), 0);
            if (used + it <= bin_capacity) { b.push_back(it); placed = true; break; }
        }
        if (!placed) s.bins.push_back({it});
    }
    s.calculateFitness(); return s;
}

Solution initRandomBestFit(const vector<int>& items) {
    Solution s;
    vector<int> ord = items;
    shuffle(ord.begin(), ord.end(), rng);
    for (int it : ord) {
        int best=-1, minspace=bin_capacity+1;
        for (int i=0;i<s.bins.size();++i) {
            int used=accumulate(s.bins[i].begin(),s.bins[i].end(),0);
            int sp=bin_capacity-(used+it);
            if(sp>=0 && sp<minspace){minspace=sp;best=i;}
        }
        if(best>=0) s.bins[best].push_back(it);
        else s.bins.push_back({it});
    }
    s.calculateFitness(); return s;
}

Solution initFirstFit(const vector<int>& items) {
    Solution s;
    for(int it:items){
        bool placed=false;
        for(auto &b:s.bins){
            int used=accumulate(b.begin(),b.end(),0);
            if(used+it<=bin_capacity){b.push_back(it);placed=true;break;}
        }
        if(!placed) s.bins.push_back({it});
    }
    s.calculateFitness(); return s;
}

// Operadores Básicos --------------------------------
vector<SwapOp> diff(const Solution &A,const Solution &B){
    unordered_map<int,int> mA,mB;
    for(int i=0;i<A.bins.size();++i) for(int v:A.bins[i]) mA[v]=i;
    for(int i=0;i<B.bins.size();++i) for(int v:B.bins[i]) mB[v]=i;
    vector<SwapOp> ops;
    for(auto &p:mB) if(mA[p.first]!=p.second) ops.emplace_back(p.first,p.second);
    return ops;
}

vector<SwapOp> selectOps(const vector<SwapOp>&ops,double rate){
    vector<SwapOp> sel;
    uniform_real_distribution<double>d(0,1);
    for(auto &op:ops) if(d(rng)<=rate) sel.push_back(op);
    return sel;
}

void applyOps(Solution &s,const vector<SwapOp>&ops){
    unordered_map<int,int> loc;
    for(int i=0;i<s.bins.size();++i) for(int v:s.bins[i]) loc[v]=i;
    for(auto &op:ops){
        int it=op.first, nb=op.second;
        int ob=loc[it]; if(nb>=s.bins.size()||nb==ob) continue;
        int used=accumulate(s.bins[nb].begin(),s.bins[nb].end(),0);
        if(used+it<=bin_capacity){
            auto &from=s.bins[ob]; from.erase(find(from.begin(),from.end(),it));
            s.bins[nb].push_back(it);
        }
    }
    s.bins.erase(remove_if(s.bins.begin(),s.bins.end(),[](const vector<int>&b){return b.empty();}),s.bins.end());
    s.calculateFitness();
}

bool tryMove(Solution &s){
    for(int i=0;i<s.bins.size();++i)
    for(int j=0;j<s.bins.size();++j){ if(i==j) continue;
        for(int k=0;k<s.bins[i].size();++k){int it=s.bins[i][k];
            int used=accumulate(s.bins[j].begin(),s.bins[j].end(),0);
            if(used+it<=bin_capacity){
                s.bins[j].push_back(it);
                s.bins[i].erase(s.bins[i].begin()+k);
                s.bins.erase(remove_if(s.bins.begin(),s.bins.end(),[](const vector<int>&b){return b.empty();}),s.bins.end());
                s.calculateFitness(); return true;
            }
        }
    }
    return false;
}

bool trySwap(Solution &s){
    for(int i=0;i<s.bins.size();++i)
    for(int j=i+1;j<s.bins.size();++j){
        int sum_i=accumulate(s.bins[i].begin(),s.bins[i].end(),0);
        int sum_j=accumulate(s.bins[j].begin(),s.bins[j].end(),0);
        for(int ii=0;ii<s.bins[i].size();++ii)
        for(int jj=0;jj<s.bins[j].size();++jj){
            int a=s.bins[i][ii], b=s.bins[j][jj];
            if(sum_i - a + b <= bin_capacity && sum_j - b + a <= bin_capacity){
                swap(s.bins[i][ii], s.bins[j][jj]);
                s.calculateFitness(); return true;
            }
        }
    }
    return false;
}
bool tryRemoveBin(Solution &s) {
    for (int i = 0; i < s.bins.size(); ++i) {
        vector<int> removed = s.bins[i];
        Solution temp = s;
        temp.bins.erase(temp.bins.begin() + i);
        bool allPlaced = true;
        for (int it : removed) {
            bool placed = false;
            for (auto &b : temp.bins) {
                int used = accumulate(b.begin(), b.end(), 0);
                if (used + it <= bin_capacity) {
                    b.push_back(it);
                    placed = true;
                    break;
                }
            }
            if (!placed) { allPlaced = false; break; }
        }
        if (allPlaced) {
            s = temp;
            s.calculateFitness();
            return true;
        }
    }
    return false;
}

// Busca Local Intensa (move + swap)
void localSearchIntense(Solution &s){
    bool improved = true;
while (improved) {
    improved = tryMove(s) || trySwap(s) || tryRemoveBin(s);
}

}

// Shaking variável
void shaking(Solution &s,const vector<int>&items){
    uniform_int_distribution<int>dBins(0,s.bins.size()-1);
    int level=1 + (rand() % 3); // remove 1-3 bins
    for(int l=0;l<level && !s.bins.empty();++l){
        int idx=dBins(rng);
        vector<int> rem=s.bins[idx];
        s.bins.erase(s.bins.begin()+idx);
        for(int it:rem){
            bool placed=false;
            for(auto &b:s.bins){int u=accumulate(b.begin(),b.end(),0);
                if(u+it<=bin_capacity){b.push_back(it);placed=true;break;}
            }
            if(!placed) s.bins.push_back({it});
        }
    }
    s.calculateFitness();
}

// Crossover de elites (metade bins de A + restos de B)
Solution crossover(const Solution &A,const Solution &B){
    Solution child;
    int half = A.bins.size()/2;
    // copia metade dos bins de A
    for(int i=0;i<half;++i) child.bins.push_back(A.bins[i]);
    // itens já alocados
    unordered_map<int,bool> used;
    for(auto &b:child.bins) for(int it:b) used[it]=true;
    // preenche com bins de B
    for(auto &b:B.bins){
        vector<int> newb;
        for(int it:b) if(!used[it]){ newb.push_back(it); used[it]=true; }
        if(!newb.empty()) child.bins.push_back(newb);
    }
    child.calculateFitness();
    return child;
}

// MSBPP Aprimorado (+Crossover + SA acceptance)
void MSBPP(const vector<int> &items){
    vector<Solution> pop;
    pop.reserve(POP_SIZE);
    for(int i=0;i<POP_SIZE;++i){
        if(i<POP_SIZE/3) pop.push_back(initFFD(items));
        else if(i<2*POP_SIZE/3) pop.push_back(initRandomBestFit(items));
        else pop.push_back(initFirstFit(items));
    }
    Solution best=pop[0];
    for(int i=1;i<POP_SIZE;++i) if(pop[i].fitness<best.fitness) best=pop[i];

    // SA temperature inicial
    double T=1.0;

    deque<vector<SwapOp>> tabu;
    for(int t=1;t<=MAX_IT;++t){
        // Diversificação periódica: reinicializa metade da população a cada 50 iterações
        if(t % 50 == 0){
            for(int i=POP_SIZE/2; i<POP_SIZE; ++i){
                pop[i] = (i % 2 == 0 ? initRandomBestFit(items) : initFFD(items));
            }
        }
        double Pdp = Pdp_initial * exp(-Pdp_decay_rate*double(t)/MAX_IT);
        double alpha = alpha_initial * (1.0 - double(t)/MAX_IT);
        // elitismo + crossover
        sort(pop.begin(),pop.end(),[](const Solution &a,const Solution &b){return a.fitness<b.fitness;});
        vector<Solution> next;
        // mantem top4
        for(int i=0;i<4;++i) next.push_back(pop[i]);
        // gera 4 filhos por crossover das elites
        for(int i=0;i<4;++i)
            for(int j=i+1;j<4;++j)
                next.push_back(crossover(pop[i],pop[j]));
        // restante por SSA
        while(next.size()<POP_SIZE){
            int idx=4 + rand()%(POP_SIZE-4);
            Solution s=pop[idx];
            if(rand()/double(RAND_MAX)>Pdp){
                auto ops=selectOps(diff(s,best),alpha);
                applyOps(s,ops);
                localSearchIntense(s);
            } else shaking(s,items);
            s.calculateFitness();
            // SA acceptance
            double df=s.fitness - pop[idx].fitness;
            if(df<0 || exp(-df/T)>rand()/double(RAND_MAX)){
                if(s.fitness<best.fitness) best=s;
                next.push_back(s);
            } else next.push_back(pop[idx]);
        }
        pop.swap(next);
        T*=0.99; // resfriamento
    }
    cout<<"Fitness="<<best.fitness<<" Bins="<<best.bins.size()<<"\n";
}

int main(){
    ifstream f("u500_00.txt"); if(!f){cerr<<"Erro\n";return 1;}
    int n,opt; f>>bin_capacity>>n>>opt;
    vector<int> items(n); for(int i=0;i<n;++i) f>>items[i]; f.close();
    MSBPP(items);
    cout<<"Ótimo esperado="<<opt<<"\n";
    return 0;
}
