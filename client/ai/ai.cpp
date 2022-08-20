#include "ai.h"
#include <bits/stdc++.h>

using namespace std;
using namespace Types;

//typedef long long ll;

#define all(x)  (x).begin(), (x).end()

const int SEED = 684345634;
const double INF = numeric_limits<double>::max();

template<class ForwardIterator>
inline size_t argmin(ForwardIterator first, ForwardIterator last) {
    return distance(first, min_element(first, last));
}

template<class ForwardIterator>
inline size_t argmax(ForwardIterator first, ForwardIterator last) {
    return distance(first, max_element(first, last));
}

template<class T, class U>
inline const T& max_by(const vector<T> &options, const function<U(T)> &func) {
    vector<U> scores;
    for (const auto &item : options)
        scores.push_back(func(item));
    int best_ind = argmax(begin(scores), end(scores));
    return options[best_ind];
}

template<class T, class U>
inline const T& min_by(const vector<T> &options, const function<U(T)> &func) {
    vector<U> scores;
    for (const auto &item : options)
        scores.push_back(func(item));
    int best_ind = argmin(begin(scores), end(scores));
    return options[best_ind];
}

void redirect_cerr(int id) {
    string filename = "./logs/client/" + to_string(id) + ".log";
    freopen(filename.c_str(), "w", stderr);
    // freopen("/dev/null", "w", stderr);
}

void log_agent(const HAS::Agent &agent) {
    cerr << "("
        << agent.id() << ", "
        << (agent.team() == HAS::Team::FIRST ? "First" : "Second") << ", "
        << (agent.type() == HAS::AgentType::POLICE ? "Police" : "Theif")
        << ")";
}

void log_turn(const GameView &gameView) {
    cerr << "turn=" << gameView.turn().turnnumber() << endl;
}

vector<HAS::Agent> get_enemies(const GameView &gameView) {
    vector<HAS::Agent> enemies;
    const auto &viewer = gameView.viewer();
    for (const auto &agent: gameView.visible_agents())
        if (!agent.is_dead() && agent.team() != viewer.team() && agent.type() != viewer.type())
            enemies.push_back(agent);
    sort(all(enemies), [] (HAS::Agent const& a, HAS::Agent const& b) {
        return a.id() < b.id();
    });
    return enemies;
}

vector<HAS::Agent> get_teammate(const GameView &gameView) {
    vector<HAS::Agent> teammates;
    const auto &viewer = gameView.viewer();
    bool has_me = false;
    for (const auto &agent: gameView.visible_agents())
        if (!agent.is_dead() && agent.team() == viewer.team() && agent.type() == viewer.type()) {
            teammates.push_back(agent);
            has_me = has_me || viewer.id() == agent.id();
        }
    if (!has_me)
        teammates.push_back(viewer);
    sort(all(teammates), [] (HAS::Agent const& a, HAS::Agent const& b) {
        return a.id() < b.id();
    });
    return teammates;
}

const AI::Phone* my_phone;
const Graph* graph;

struct AIAgent {
    vector<vector<ShortestPath*>> map;
    vector<double> edge_cost;

    void preprocess(const GameView &gameView) {
        const auto &g = gameView.config().graph();
        for (const auto& edge : g.paths()) {
            edge_cost.push_back(edge.price());
        }
        sort(all(edge_cost));
        edge_cost.erase(unique(all(edge_cost)), end(edge_cost));
        for (auto cost : edge_cost) {
            map.emplace_back();
            auto& cur = map.back();
            cur.push_back(nullptr);
            for (int i = 1; i <= graph->n; i++) {
                auto sp = new ShortestPath(graph->n);
                sp->update(graph, {i}, cost);
                cur.push_back(sp);
            }
        }
    }
    vector<ShortestPath*>& get_map(double wallet) {
        for (int i = edge_cost.size()-1; i >= 0; i--)
            if (edge_cost[i] <= wallet)
                return map[i];
    }
    int get_dist(int u, int v, double wallet) {
        return get_map(wallet)[u]->dist[v];
    }
    virtual void initialize(const GameView &gameView) {}
    virtual int starting_node(const GameView &gameView) {
        cerr << "bad" << endl;
        return 2;
    };
    virtual int turn(const GameView &gameView) {
        cerr << "bad" << endl;
        return 2;
    };
};

struct MyAgent{
    double balance;
    int id, node;
    HAS::AgentType type;
    MyAgent(const HAS::Agent& agent) {
        balance = INF;
        id = agent.id();
        node = agent.node_id();
        type = agent.type();
    }
};

AIAgent* aiagent;

struct AIThief : AIAgent {
    mt19937 rng = mt19937(chrono::steady_clock::now().time_since_epoch().count());
    ShortestPath *policemap;

    void initialize(const GameView &gameView) {
        policemap = new ShortestPath(graph->n);
    }
    int starting_node(const GameView &gameView) {
        mt19937 rng = mt19937(SEED);
        vector<int> node_options;
        auto tmpmap = get_map(INF)[1];
        int maxdist = 0;
        for (int i = 1; i < graph->n; i++)
            maxdist = max(maxdist, tmpmap->dist[i]);
        for (int i = 1; i < graph->n; i++)
            if (tmpmap->dist[i] >= maxdist - 2)
                node_options.push_back(i);
        const auto &me = gameView.viewer();
        const auto &teammates = get_teammate(gameView);
        vector<int> ans;
        int max_sum = -1;
        for(int i = 0 ; i < 10 ; i++){
            int sum = 0;
            vector<int> nodes;
            for(int j = 0 ; j < teammates.size(); j++)
                nodes.push_back(node_options[rng() % node_options.size()]);
            for(int j : nodes)
                for(int k : nodes)
                    sum += get_dist(j , k , INF);
            if(sum > max_sum){
                max_sum = sum;
                ans = nodes;
            }
        }
        for(int i = 0 ; i < ans.size() ; i++)
            if(teammates[i].id() == me.id())
                return ans[i];
        int ind = rng() % node_options.size();
        return node_options[ind];
    }
    int turn(const GameView &gameView) {
        const auto &me = gameView.viewer();
        const auto &enemies = get_enemies(gameView);
        const auto &teammates = get_teammate(gameView);
        auto selfmap = get_map(gameView.balance())[me.node_id()];
        vector<int> police_nodes;
        for (const auto &police : enemies)
            police_nodes.push_back(police.node_id());
        policemap->update(graph, police_nodes, INF);
        vector<int> node_options;
        for (int i = 1; i < graph->n; i++) {
            if (selfmap->dist[i] < policemap->dist[i])
                node_options.push_back(i);
        }
        shuffle(begin(node_options), end(node_options), rng);
        int target = max_by<int,int>(node_options, [&] (int node) {
            int score = 0;
            for (const auto &police: enemies){
                int dist = get_dist(node, police.node_id(), INF);
                score += dist;
                if(dist == 1){
                    score -= 1000 * 1000;
                }
            }
            for (const auto &thief : teammates)
                score += get_dist(node, thief.node_id(), INF);
            return score;
        });
        return selfmap->first[target];
    }
};

struct AIPolice : AIAgent {
    mt19937 rng = mt19937(SEED);
    vector<MyAgent> last_seen, minimax_order;
    vector<int> choice_node, choice_value;
    int starting_target = -1;

    int turn(const GameView &gameView) {
        int turn_number = gameView.turn().turnnumber();
        vector<int> visible_turns;
        const auto& visible = gameView.config().turnsettings().visibleturns();
        const auto &me = gameView.viewer();
        const auto &enemies = get_enemies(gameView);
        for(const auto& turn : visible){
            visible_turns.push_back(turn);
        }
        if (!enemies.empty()) {
            cerr << "visible turn!" << endl;
            last_seen.clear();
            for (const auto &thief : enemies)
                last_seen.push_back(MyAgent(thief));
        }
        if (last_seen.empty()){
            if(starting_target == -1){
                mt19937 rng = mt19937(chrono::steady_clock::now().time_since_epoch().count());
                int cnt_edge = (visible_turns[0] - turn_number) / 2;
                vector<int> max_dist , options;
                int min_r = graph->n * 10;
                max_dist.push_back(min_r);
                for(int i = 1; i <= graph->n; i++){
                    max_dist.push_back(-1);
                    if(get_dist(me.node_id() , i , gameView.balance()) > cnt_edge)
                        continue;
                    for(int j = 1; j <= graph->n; j++)
                        max_dist[i] = max(max_dist[i] , get_dist(i , j , INF));
                    min_r = min(min_r , max_dist[i]);
                }
                for(int i = 1; i <= graph->n; i++)
                    if(max_dist[i] == min_r)
                        options.push_back(i);
                int ind = rng() % options.size(); 
                starting_target = options[ind];
                cerr << "starting target=" << starting_target << ", " << "options=" << options.size() << endl;
            }
            return get_map(gameView.balance())[me.node_id()]->first[starting_target];
        }
        const auto &polices = get_teammate(gameView);
        MyAgent target = min_by<MyAgent,int>(last_seen, [&] (MyAgent const& agent) {
            int score = 0, node = agent.node;
            for (const auto &police : polices)
                score += get_dist(node, police.node_id(), INF);
            cerr << "node=" << node << ", " << "score=" << score << endl;
            return score;
        });
        minimax_order.clear();
        for (const auto& agent : get_teammate(gameView))
            minimax_order.push_back(MyAgent(agent));
        minimax_order.push_back(target);
        choice_node.assign(minimax_order.size(), 0);
        choice_value.assign(minimax_order.size(), INT_MAX);
        cerr << "minimax: ";
        for (const auto& agent: minimax_order) {
            cerr << "(" << agent.id << ", ";
            if (agent.type == HAS::AgentType::POLICE)
                cerr << "police";
            else
                cerr << "thief";
            cerr << ") ";
        }
        cerr << endl;
        minimax(1, 0);
        int choice;
        for (int i = 0; i < minimax_order.size(); i++) {
            if (minimax_order[i].id == me.id()) {
                choice = choice_node[i];
                cerr << "choice=" << choice << " score=" << choice_value[i] << endl;
            }
        }
        return choice;
    }
    int minimax(int level, int ind) {
        if (ind == minimax_order.size()) {
            ind = 0;
            level--;
        }
        if (level == 0) {
            const auto& target = minimax_order.back();
            int score = 0;
            for (int i = 0; i < minimax_order.size()-1; i++) {
                const auto& police = minimax_order[i];
                score += get_dist(police.node, target.node, police.balance);
            }
            return score;
        }
        MyAgent now = minimax_order[ind];
        if (now.type == HAS::AgentType::POLICE) {
            int result = INT_MAX;
            for (const auto& edge : graph->adj[now.node]) {
                if (edge.price > now.balance)
                    continue;
                MyAgent nxt = now;
                nxt.node = edge.v;
                nxt.balance -= edge.price;
                minimax_order[ind] = nxt;
                int score = minimax(level, ind+1);
                minimax_order[ind] = now;
                result = min(result, score);
                if (score < choice_value[ind]) { // and first layer
                    choice_value[ind] = score;
                    choice_node[ind] = edge.v;
                }
            }
            return result;
        } else {
            int result = minimax(level, ind+1);
            for (const auto& edge : graph->adj[now.node]) {
                if (edge.price > now.balance)
                    continue;
                MyAgent nxt = now;
                nxt.node = edge.v;
                nxt.balance -= edge.price;
                minimax_order[ind] = nxt;
                int score = minimax(level, ind+1);
                minimax_order[ind] = now;
                result = max(result, score);
            }
            return result;
        }
    }
};

namespace AI {
    void initialize(const GameView &gameView, const Phone &phone) {
        my_phone = &phone;
        auto me = gameView.viewer();
        redirect_cerr(me.id());
        log_agent(me);
        cerr << endl;
        graph = new Graph(gameView.config().graph());
        if (me.type() == HAS::AgentType::POLICE)
            aiagent = new AIPolice();
        else
            aiagent = new AIThief();
        aiagent->preprocess(gameView);
        aiagent->initialize(gameView);
    }

    int get_thief_starting_node(const GameView &gameView) {
        int node = aiagent->starting_node(gameView);
        cerr << "start from " << node << endl;
        return node;
    }

    int thief_move_ai(const GameView &gameView) {
        log_turn(gameView);
        return aiagent->turn(gameView);
    }

    int police_move_ai(const GameView &gameView) {
        log_turn(gameView);
        return aiagent->turn(gameView);
    }
}
