#pragma once

#include <vector>
#include "graph.h"
#include "types.h"

using namespace std;

struct ShortestPath {
    vector<int> dist, first, last;
    ShortestPath(int n);
    void update(const Graph* graph, const vector<int> &start_nodes, bool use_price);
};