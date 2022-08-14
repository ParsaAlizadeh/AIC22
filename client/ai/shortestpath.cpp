#include <bits/stdc++.h>
#include "shortestpath.h"

typedef pair<int, int> pii;

ShortestPath::ShortestPath(int n) {
    dist.resize(n+1);
    first.resize(n+1);
    last.resize(n+1);
}

void ShortestPath::update(const Graph* graph, const vector<int> &start_nodes, bool use_price) {
    queue<int> q;
    fill(dist.begin(), dist.end(), dist.size());
    for (int st : start_nodes) {
        dist[st] = 0;
        first[st] = st;
        q.push(st);
    }
    while (!q.empty()) {
        int u = q.front(); q.pop();
        for (const auto& edge : graph->adj[u]) {
            if (!use_price && edge.price > 0)
                continue;
            if (dist[edge.v] <= dist[u] + 1)
                continue;
            dist[edge.v] = dist[u] + 1;
            last[edge.v] = u;
            first[edge.v] = (dist[u] == 0 ? edge.v : first[u]);
            q.push(edge.v);
        }
    }
}