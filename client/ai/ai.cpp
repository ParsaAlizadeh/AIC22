#include "ai.h"
#include <bits/stdc++.h>

using namespace std;
using namespace Types;

//typedef long long ll;

#define all(x)  (x).begin(), (x).end()

//const int SEED = 684345634;
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
    return enemies;
}

vector<HAS::Agent> get_teammate(const GameView &gameView) {
    vector<HAS::Agent> enemies;
    const auto &viewer = gameView.viewer();
    for (const auto &agent: gameView.visible_agents())
        if (!agent.is_dead() && agent.team() == viewer.team() && agent.type() == viewer.type())
            enemies.push_back(agent);
    return enemies;
}

struct AIAgent {
    mt19937 rng = mt19937(chrono::steady_clock::now().time_since_epoch().count());
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
    int id , type , node; // type = (police ? 0 : 1);
    MyAgent(int _id, int _type, int _node){
        balance = INF;
        id = _id;
        type = _type;
        node = _node;
    }
};

int id_cmp(MyAgent x, MyAgent y){
    return x.id < y.id;
}

const AI::Phone* my_phone;
const Graph* graph;
AIAgent* aiagent;

struct AIThief : AIAgent {
    ShortestPath *policemap, *selfmap, *tmpmap;

    void initialize(const GameView &gameView) {
        policemap = new ShortestPath(graph->n);
        selfmap = new ShortestPath(graph->n);
        tmpmap = new ShortestPath(graph->n);
    }
    int starting_node(const GameView &gameView) {
        vector<int> node_options;
        tmpmap->update(graph, {1}, INF);
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
        selfmap->update(graph, {me.node_id()}, gameView.balance());
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
            tmpmap->update(graph, {node}, INF);
            int score = 0;
            for (const auto &police: enemies)
                score += tmpmap->dist[police.node_id()];
            return score;
        });
        return selfmap->first[target];
    }
};

struct AIPolice : AIAgent {
    ShortestPath *tmpmap;
    vector<int> last_seen;

    void initialize(const GameView &gameView) {
        tmpmap = new ShortestPath(graph->n);
    }
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
            tmpmap->update(graph, {node}, INF);
            int score = 0;
            for (const auto &police : polices)
                score += tmpmap->dist[police.node_id()];
            cerr << "node=" << node << ", " << "score=" << score << endl;
            return score;
        });
        tmpmap->update(graph, {me.node_id()}, gameView.balance());
        return tmpmap->first[target];
    }
};

struct AIPolice_v2 : AIAgent {
    ShortestPath *tmpmap;
    vector<MyAgent> last_seen;

    void initialize(const GameView &gameView) {
        tmpmap = new ShortestPath(graph->n);
    }
    int turn(const GameView &gameView) {
        const auto &me = gameView.viewer();
        const auto &enemies = get_enemies(gameView);
        if (!enemies.empty()) {
            cerr << "visible turn!" << endl;
            last_seen.clear();
            for (const auto &thief : enemies)
                last_seen.push_back(MyAgent(thief.id() , 1 , thief.node_id()));
        }
        if (last_seen.empty())
            return me.node_id(); // TODO
        vector<HAS::Agent> polices = get_teammate(gameView);
        int flag = 0;
        for(auto &i : polices){
            if(i.id() == me.id()){
                flag = 1;
            }
        }
        if(flag == 0){
            polices.push_back(me);
        }
        vector<MyAgent> police_agents;
        for(auto &police : polices)
            police_agents.push_back(MyAgent(police.id() , 0 , police.node_id()));
        sort(all(last_seen) , id_cmp);
        shuffle(all(last_seen) , rng);
        MyAgent target = min_by<MyAgent,int>(last_seen, [&] (MyAgent thief) {
            tmpmap->update(graph, {thief.node}, INF);
            int score = 0;
            vector<int> dist;
            for (const auto &police : polices)
                dist.push_back(tmpmap->dist[police.node_id()]);
            sort(all(dist), greater<int>());
            for(int i = 0 ; i < dist.size() ; i++)
                score += (i + 1) * dist[i];
            cerr << "node=" << thief.node << ", " << "score=" << score << endl;
            return score;
        });
        sort(all(police_agents) , id_cmp);
        tmpmap->update(graph, {me.node_id()}, gameView.balance());
        return tmpmap->first[target.node];
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
            aiagent = new AIPolice_v2();
        else
            aiagent = new AIThief_v2();
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
