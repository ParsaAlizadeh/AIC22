#pragma once

#include <vector>
#include "graph.h"
#include "types.h"

using namespace std;

struct ShortestPath {
    int start_node;
    vector<int> dist, first, last;
    ShortestPath(int n);
    void update(const Graph* graph, int start_node, bool use_price);
};