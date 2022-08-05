#pragma once

#include <vector>
#include "types.h"

using namespace std;

struct Edge {
    int id, u, v;
    double price;
};

struct Graph {
    int n, m;
    vector<vector<Edge>> adj;
    Graph(const HAS::Graph&);
};