#pragma once

#include "shortestpath.h"
#include "types.h"
#include <bits/stdc++.h>

using namespace std;
using namespace Types;

#define all(x)  begin(x), end(x)

struct World {
    const Graph *graph;
    vector<vector<ShortestPath*>> map;
    vector<double> edge_cost;

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
    ShortestPath* get_map_from(int u, double wallet) {
        return get_map(wallet)[u];
    }
    int get_dist(int u, int v, double wallet) {
        return get_map_from(u, wallet)->dist[v];
    }
};