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

namespace AI {
    const Phone* my_phone;
    const Graph* graph;
    ShortestPath* shortestpath;

    int get_thief_starting_node(const GameView &gameView) {
        return 2;
    }

    void initialize(const GameView &gameView, const Phone &phone) {
        my_phone = &phone;
        auto me = gameView.viewer();
        redirect_cerr(me.id());
        log_agent(me);
        cerr << endl;
        graph = new Graph(gameView.config().graph());
        shortestpath = new ShortestPath(graph->n);
    }

    int thief_move_ai(const GameView &gameView) {
        log_turn(gameView);
        int current_node = gameView.viewer().node_id();
        vector<int> node_options;
        for (const auto &edge : graph->adj[current_node]) {
            if (edge.price > 0)
                continue;
            node_options.push_back(edge.v);
        }
        node_options.push_back(current_node);
        vector<int> scores;
        const auto &polices = get_enemies(gameView);
        for (int u : node_options) {
            int &score = scores.emplace_back(0);
            shortestpath->update(graph, u, false);
            for (const auto &police : polices) {
                score += shortestpath->dist[police.node_id()];
            }
            cerr << "node=" << u << " score=" << score << endl;
        }
        return node_options[argmax(scores.begin(), scores.end())];
    }

    int police_move_ai(const GameView &gameView) {
        log_turn(gameView);
        return 1;
    }
}