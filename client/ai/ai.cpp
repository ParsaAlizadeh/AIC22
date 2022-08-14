#include "ai.h"
#include <bits/stdc++.h>

using namespace std;
using namespace Types;

template<class ForwardIterator>
inline size_t argmin(ForwardIterator first, ForwardIterator last) {
    return distance(first, min_element(first, last));
}

template<class ForwardIterator>
inline size_t argmax(ForwardIterator first, ForwardIterator last) {
    return distance(first, max_element(first, last));
}

void redirect_cerr(int id) {
    string filename = "./logs/client/" + to_string(id) + ".log";
    freopen(filename.c_str(), "w", stderr);
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
        if (agent.team() != viewer.team() && agent.type() != viewer.type())
            enemies.push_back(agent);
    return enemies;
}

vector<HAS::Agent> get_teammate(const GameView &gameView) {
    vector<HAS::Agent> enemies;
    const auto &viewer = gameView.viewer();
    for (const auto &agent: gameView.visible_agents())
        if (agent.team() == viewer.team() && agent.type() == viewer.type())
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
        tmpmap->update(graph, {1}, false);
        int maxdist = 0;
        for (int i = 1; i < graph->n; i++) {
            maxdist = max(maxdist, tmpmap->dist[i]);
        }
        for (int i = 1; i < graph->n; i++) {
            if (tmpmap->dist[i] >= maxdist - 2) {
                node_options.push_back(i);
            }
        }
        int ind = rng() % node_options.size();
        return node_options[ind];
    }
    int turn(const GameView &gameView) {
        const auto &me = gameView.viewer();
        const auto &enemies = get_enemies(gameView);
        selfmap->update(graph, {me.node_id()}, false);
        vector<int> police_nodes;
        for (const auto &police : enemies)
            police_nodes.push_back(police.node_id());
        policemap->update(graph, police_nodes, false);
        vector<int> node_options;
        for (int i = 1; i < graph->n; i++) {
            if (selfmap->dist[i] < policemap->dist[i])
                node_options.push_back(i);
        }
        shuffle(begin(node_options), end(node_options), rng);
        vector<int> node_scores;
        for (int node : node_options) {
            tmpmap->update(graph, {node}, false);
            int score = 0;
            for (const auto &police: enemies) {
                score += tmpmap->dist[police.node_id()];
            }
            node_scores.push_back(score);
        }
        int best_ind = argmax(begin(node_scores), end(node_scores));
        int target = node_options[best_ind];
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
            for (const auto &thief : enemies) {
                last_seen.push_back(thief.node_id());
            }
        }
        if (last_seen.empty())
            return me.node_id();
        const auto &polices = get_teammate(gameView);
        vector<int> node_scores;
        for (int node : last_seen) {
            tmpmap->update(graph, {node}, false);
            int score = 0;
            for (const auto &police : polices)
                score += tmpmap->dist[police.node_id()];
            cerr << "node=" << node << ", " << "score=" << score << endl;
            node_scores.push_back(score);
        }
        int best_ind = argmin(begin(node_scores), end(node_scores));
        int target = last_seen[best_ind];
        tmpmap->update(graph, {me.node_id()}, false);
        return tmpmap->first[target];
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
