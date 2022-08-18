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
    vector<HAS::Agent> enemies;
    const auto &viewer = gameView.viewer();
    for (const auto &agent: gameView.visible_agents())
        if (!agent.is_dead() && agent.team() == viewer.team() && agent.type() == viewer.type())
            enemies.push_back(agent);
    sort(all(enemies), [] (HAS::Agent const& a, HAS::Agent const& b) {
        return a.id() < b.id();
    });
    return enemies;
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
            auto& cur = map.emplace_back();
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
        vector<int> node_options;
        auto tmpmap = get_map(INF)[1];
        int maxdist = 0;
        for (int i = 1; i < graph->n; i++)
            maxdist = max(maxdist, tmpmap->dist[i]);
        for (int i = 1; i < graph->n; i++)
            if (tmpmap->dist[i] >= maxdist - 2)
                node_options.push_back(i);
        int ind = rng() % node_options.size();
        return node_options[ind];
    }
    int turn(const GameView &gameView) {
        const auto &me = gameView.viewer();
        const auto &enemies = get_enemies(gameView);
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
            for (const auto &police: enemies)
                score += get_dist(node, police.node_id(), INF);
            return score;
        });
        return selfmap->first[target];
    }
};

struct AIPolice : AIAgent {
    mt19937 rng = mt19937(SEED);
    vector<int> last_seen;

    int turn(const GameView &gameView) {
        const auto &me = gameView.viewer();
        const auto &enemies = get_enemies(gameView);
        if (!enemies.empty()) {
            cerr << "visible turn!" << endl;
            last_seen.clear();
            for (const auto &thief : enemies)
                last_seen.push_back(thief.node_id());
        }
        if (last_seen.empty())
            return me.node_id();
        const auto &polices = get_teammate(gameView);
        int target = min_by<int,int>(last_seen, [&] (int node) {
            int score = 0;
            for (const auto &police : polices)
                score += get_dist(node, police.node_id(), INF);
            cerr << "node=" << node << ", " << "score=" << score << endl;
            return score;
        });
        return get_map(gameView.balance())[me.node_id()]->first[target];
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
