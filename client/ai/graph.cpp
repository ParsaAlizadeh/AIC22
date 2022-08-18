#include "graph.h"
#include <bits/stdc++.h>
using namespace std;

//const int SEED = 684345634;

Graph::Graph(const HAS::Graph& g) {
    n = g.nodes().size();
    m = g.paths().size();
    mt19937 rng(chrono::steady_clock::now().time_since_epoch().count());
    vector<int> inds;
    for (int i = 0; i < m; i++) {
        inds.push_back(i);
    }
    shuffle(begin(inds), end(inds), rng);
    adj.resize(n+1);
    for (int i = 0; i < m; i++) {
        const auto &path = g.paths()[inds[i]];
        int x = path.first_node_id(), y = path.second_node_id();
        adj[x].push_back(Edge{.id=path.id(), .u=x, .v=y, .price=path.price()});
        adj[y].push_back(Edge{.id=path.id(), .u=y, .v=x, .price=path.price()});
    }
}