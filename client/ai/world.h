#pragma once

#include "shortestpath.h"
#include "types.h"
#include "client/client.h"
#include <bits/stdc++.h>

using namespace std;
using namespace Types;
using Phone = Client::Client::Phone;

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

void log_agent(const HAS::Agent &agent) {
    cerr << "(";
    cerr << agent.id() << ", ";
    cerr << (agent.team() == HAS::Team::FIRST ? "First" : "Second") << ", ";
    if (agent.type() == HAS::AgentType::POLICE) {
        cerr << "police";
    } else if (agent.type() == HAS::AgentType::BATMAN) {
        cerr << "batman";
    } else if (agent.type() == HAS::AgentType::THIEF) {
        cerr << "theif";
    } else {
        cerr << "joker";
    }
    cerr << ")";
}

void log_agent(const WorldAgent& agent) {
    cerr << "(" << agent.id << ", " << agent.node << ", ";
    if (agent.type == HAS::AgentType::POLICE) {
        cerr << "police";
    } else if (agent.type == HAS::AgentType::BATMAN) {
        cerr << "batman";
    } else if (agent.type == HAS::AgentType::THIEF) {
        cerr << "theif";
    } else {
        cerr << "joker";
    }
    cerr << ") ";
}

struct World {
    const Graph *graph;
    vector<vector<ShortestPath*>> maps;
    vector<vector<vector<Edge>>> options;
    vector<double> edge_cost;
    map<int, WorldAgent> agents;
    int current_turn;
    int last_message_index = 0;
    string message_to_send = "";

    void initialize(const GameView &gameView, const Graph* g) {
        graph = g;
        preprocess_map(gameView);
        preprocess_options();
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
    void preprocess_options(){
        options.push_back({});
        for(int i = 1; i <= graph->n; i++){
            vector<vector<Edge>> cur = graph->adj;
            for(int j = 1; j <= graph->n; j++){
                sort(all(cur[j]), [&] (Edge const& x, Edge const& y)  {
                    return get_dist(i, x.v, INF) < get_dist(i, y.v, INF);
                });
            }
            options.push_back(cur);
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
    HAS::AgentType get_general_type(HAS::AgentType type) {
        if (type == HAS::AgentType::BATMAN)
            return HAS::AgentType::POLICE;
        if (type == HAS::AgentType::JOKER)
            return HAS::AgentType::THIEF;
        return type;
    }
    double get_income_by_type(const GameView &gameView, HAS::AgentType type) {
        auto const& settings = gameView.config().incomesettings();
        type = get_general_type(type);
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
        wag.type = ag.type();
        wag.node = v;
        wag.last_seen = turn;
        wag.dead = ag.is_dead();
        wag.balance += (turn - last_seen) * get_income_by_type(gameView, wag.type);
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
            cerr << "balance mismatch! " << gameView.balance() << " " << agents[gameView.viewer().id()].balance << endl; // only when theif dies
        update_chatbox(gameView);
    }
    const WorldAgent& get_self(const GameView &gameView) {
        return agents[gameView.viewer().id()];
    }
    vector<WorldAgent> get_teammates(const GameView &gameView) {
        vector<WorldAgent> result;
        const auto& self = get_self(gameView);
        for (const auto& p : agents) {
            const auto& agent = p.second;
            if (agent.dead)
                continue;
            if (agent.team != self.team)
                continue;
            if (get_general_type(agent.type) != get_general_type(self.type))
                continue;
            result.push_back(agent);
        }
        return result;
    }
    vector<WorldAgent> get_enemies(const GameView &gameView) {
        vector<WorldAgent> result;
        const auto& self = get_self(gameView);
        for (const auto& p : agents) {
            const auto& agent = p.second;
            if (agent.dead)
                continue;
            if (agent.team == self.team)
                continue;
            if (get_general_type(agent.type) == get_general_type(self.type))
                continue;
            result.push_back(agent);
        }
        return result;
    }
    const vector<Edge>& get_options(int node) {
        return graph->adj[node];
    }
    const vector<Edge>& get_options(int node, int target) {
        return options[target][node];
    }
    void update_chatbox(const GameView &gameView) {
        int turn = gameView.turn().turnnumber();
        const auto& chatbox = gameView.chatbox();
        for (; last_message_index < chatbox.size(); last_message_index++) {
            const auto& chat = chatbox[last_message_index];
            cerr << "a new message index=" << last_message_index << endl;
            update_chat(gameView, chat, turn-2);
        }
    }
    void update_chat(const GameView &gameView, const HAS::Chat& chat, int turn) {
        const auto& text = chat.text();
        cerr << "update from a chat: text=" << text << " id=" << chat.fromagentid() << endl;
        if (chat.fromagentid() != get_self(gameView).id)
            agents[chat.fromagentid()].balance -= text.size();
        for (int i = 0; i < text.size(); i += 12) {
            const auto substr = text.substr(i, 12);
            read_from_text(gameView, text, turn);
        }
    }
    void read_from_text(const GameView &gameView, const string& text, int turn) {
        bitset<4> bit_id(text, 0, 4);
        bitset<8> bit_node(text, 4, 8);
        int agent_id = bit_id.to_ulong() + 1;
        int agent_node = bit_node.to_ulong();
        if (agents.count(agent_id) && agents[agent_id].last_seen >= turn)
            return;
        WorldAgent agent;
        agent.id = agent_id;
        if (agent_node == 0) {
            agent.dead = true;
        } else {
            agent.node = agent_node;
        }
        const auto& me = get_self(gameView);
        agent.team = (me.team == HAS::FIRST ? HAS::SECOND : HAS::FIRST);
        if (get_general_type(me.type) == HAS::POLICE) {
            agent.type = (agents.count(agent_id) ? agents[agent_id].type : HAS::THIEF);
        } else {
            agent.type = HAS::BATMAN;
        }
        agent.last_seen = turn;
        agents[agent_id] = agent;
        cerr << "read from chat ";
        log_agent(agent);
        cerr << endl;
    }
    string write_to_text(WorldAgent const& agent) {
        bitset<4> bit_id(agent.id - 1);
        bitset<8> bit_node(agent.dead ? 0 : agent.node);
        string result = bit_id.to_string() + bit_node.to_string();
        cerr << "write to chat ";
        log_agent(agent);
        cerr << endl;
        return result;
    }
    void send_chat(const GameView &gameView, WorldAgent const& agent) {
        string chat = write_to_text(agent);
        int id = get_self(gameView).id;
        if (agents[id].balance >= chat.size()) {
            agents[id].balance -= chat.size();
            message_to_send += chat;
        } else {
            cerr << "not enough money for send chat" << endl;
        }
    }
    void send_chatbox(const GameView &gameView, const Phone* phone) {
        if (message_to_send.size()) {
            phone->send_message(message_to_send);
        }
        message_to_send = "";
    }
};