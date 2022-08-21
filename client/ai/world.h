#pragma once

#include "shortestpath.h"
#include "types.h"
#include <bits/stdc++.h>

using namespace std;
using namespace Types;

#define all(x)  begin(x), end(x)

struct WorldAgent{
    double balance = INF;
    int id, node;
    HAS::Team team;
    HAS::AgentType type;
    int last_seen = -100;
    bool dead = false;
    WorldAgent() {}
    WorldAgent(const HAS::Agent& agent) {
        id = agent.id();
        node = agent.node_id();
        type = agent.type();
        dead = agent.is_dead();
        team = agent.team();
    }
    bool operator<(const WorldAgent& oth) const {
        return id < oth.id;
    }
};

struct World {
    const Graph *graph;
    vector<vector<ShortestPath*>> maps;
    vector<double> edge_cost;
    map<int, WorldAgent> agents;
    int current_turn;

    void initialize(const GameView &gameView, const Graph* g) {
        graph = g;
        preprocess_map(gameView);
    }
    void preprocess_map(const GameView &gameView) {
        const auto &g = gameView.config().graph();
        for (const auto& edge : g.paths()) {
            edge_cost.push_back(edge.price());
        }
        sort(all(edge_cost));
        edge_cost.erase(unique(all(edge_cost)), end(edge_cost));
        for (auto cost : edge_cost) {
            vector<ShortestPath*> cur;
            cur.push_back(nullptr);
            for (int i = 1; i <= graph->n; i++) {
                auto sp = new ShortestPath(graph->n);
                sp->update(graph, {i}, cost);
                cur.push_back(sp);
            }
            maps.push_back(cur);
        }
    }
    vector<ShortestPath*>& get_map(double wallet) {
        for (int i = edge_cost.size()-1; i >= 0; i--)
            if (edge_cost[i] <= wallet)
                return maps[i];
    }
    ShortestPath* get_map_from(int u, double wallet) {
        return get_map(wallet)[u];
    }
    int get_dist(int u, int v, double wallet) {
        return get_map_from(u, wallet)->dist[v];
    }
    int get_dist(WorldAgent agent , int v){
        return get_dist(agent.node, v, agent.balance);
    }
    double get_income_by_type(const GameView &gameView, HAS::AgentType type) {
        auto const& settings = gameView.config().incomesettings();
        if (type == HAS::POLICE)
            return settings.policeincomeeachturn();
        return settings.thievesincomeeachturn();
    }
    double get_edge_price(int u, int v) {
        for (const auto& edge : graph->adj[u])
            if (edge.v == v)
                return edge.price;
        return 0;
    }
    void update_agent(const GameView &gameView, const HAS::Agent &ag, int turn) {
        if (!agents.count(ag.id())) {
            WorldAgent wag = ag;
            wag.last_seen = turn;
            if (turn == 1) {
                wag.balance = gameView.balance(); // each agent same balance for start
                wag.last_seen = 2;
            }
            agents[ag.id()] = wag;
        }
        auto& wag = agents[ag.id()];
        if (wag.last_seen >= turn)
            return;
        int u = wag.node, v = ag.node_id(), last_seen = wag.last_seen;
        wag.node = v;
        wag.last_seen = turn;
        wag.dead = ag.is_dead();
        wag.balance += (turn - last_seen ) * get_income_by_type(gameView, wag.type);
        if (turn - last_seen <= 2)
            wag.balance -= get_edge_price(u, v);
    }
    void update(const GameView &gameView) {
        int turn = gameView.turn().turnnumber();
        current_turn = turn;
        for (const auto& agent : gameView.visible_agents()) {
            update_agent(gameView, agent, turn);
        }
        update_agent(gameView, gameView.viewer(), turn);
        if (gameView.balance() != agents[gameView.viewer().id()].balance)
            cerr << "balance mismatch!" << endl; // only when theif dies
    }
    const WorldAgent& get_self(const GameView &gameView) {
        return agents[gameView.viewer().id()];
    }
    vector<WorldAgent> get_teammates(const GameView &gameView) {
        vector<WorldAgent> result;
        const auto& self = get_self(gameView);
        for (const auto& p : agents) {
            const auto& agent = p.second;
            if (agent.dead) continue;
            if (agent.team != self.team) continue;
            if (agent.type != self.type) continue;
            result.push_back(agent);
        }
        return result;
    }
    vector<WorldAgent> get_enemies(const GameView &gameView) {
        vector<WorldAgent> result;
        const auto& self = get_self(gameView);
        for (const auto& p : agents) {
            const auto& agent = p.second;
            if (agent.dead) continue;
            if (agent.team == self.team) continue;
            if (agent.type == self.type) continue;
            result.push_back(agent);
        }
        return result;
    }
    vector<Edge> get_options(int node) {
        vector<Edge> result = graph->adj[node];
        result.push_back(Edge{.id=-1, .u=node, .v=node, .price=0});
        return result;
    }
};