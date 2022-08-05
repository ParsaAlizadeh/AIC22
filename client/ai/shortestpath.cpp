#include <bits/stdc++.h>
#include "shortestpath.h"

typedef pair<int, int> pii;

ShortestPath::ShortestPath(int n) {
    dist.resize(n+1);
    first.resize(n+1);
    last.resize(n+1);
}

void ShortestPath::update(const Graph* graph, int st, bool use_price) {
    start_node = st;
    queue<int> q;
    fill(dist.begin(), dist.end(), dist.size());
    dist[start_node] = 0;
    q.push(start_node);
    while (!q.empty()) {
        int u = q.front(); q.pop();
        for (const auto& edge : graph->adj[u]) {
            if (!use_price && edge.price > 0)
                continue;
            if (dist[edge.v] <= dist[u] + 1)
                continue;
            dist[edge.v] = dist[u] + 1;
            last[edge.v] = u;
            first[edge.v] = (u == start_node ? edge.v : first[u]);
            q.push(edge.v);
        }
    }
}