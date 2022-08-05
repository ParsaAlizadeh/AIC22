#include "graph.h"

Graph::Graph(const HAS::Graph& g) {
    n = g.nodes().size();
    m = g.paths().size();
    adj.resize(n+1);
    for (const auto& path: g.paths()) {
        int x = path.first_node_id(), y = path.second_node_id();
        adj[x].push_back(Edge{.id=path.id(), .u=x, .v=y, .price=path.price()});
        adj[y].push_back(Edge{.id=path.id(), .u=y, .v=x, .price=path.price()});
    }
}