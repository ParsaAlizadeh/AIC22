#include "ai.h"
#include "world.h"
#include <bits/stdc++.h>

using namespace std;
using namespace Types;

const int SEED = 684345634;

template<class ForwardIterator>
inline size_t argmin(ForwardIterator first, ForwardIterator last) {
    return distance(first, min_element(first, last));
}

template<class ForwardIterator>
inline size_t argmax(ForwardIterator first, ForwardIterator last) {
    return distance(first, max_element(first, last));
}

template<class T, class U>
inline const T& max_by(const vector<T> &options, const function<U(T)> &func) {
    vector<U> scores;
    for (const auto &item : options)
        scores.push_back(func(item));
    int best_ind = argmax(begin(scores), end(scores));
    return options[best_ind];
}

template<class T, class U>
inline const T& min_by(const vector<T> &options, const function<U(T)> &func) {
    vector<U> scores;
    for (const auto &item : options)
        scores.push_back(func(item));
    int best_ind = argmin(begin(scores), end(scores));
    return options[best_ind];
}

template<class T, class U>
inline const T& random_max_by(mt19937 rng, const vector<T> &options, const function<U(T)> &func) {
    int score = INT_MIN , best_ind = -1;
    for(int i = 0; i < 10; i++){
        int ind = rng() % options.size();
        int value = func(options[ind]);
        if(value > score){
            score = value;
            best_ind = ind;
        }
    }
    return options[best_ind];
}

template<class T, class U>
inline const T& random_min_by(mt19937 rng, const vector<T> &options, const function<U(T)> &func) {
    int score = INT_MAX , best_ind = -1;
    for(int i = 0; i < 10; i++){
        int ind = rng() % options.size();
        int value = func(options[ind]);
        if(value < score){
            score = value;
            best_ind = ind;
        }
    }
    return options[best_ind];
}

void redirect_cerr(int id) {
    string filename = "./logs/client/" + to_string(id) + ".log";
    freopen(filename.c_str(), "w", stderr);
    // freopen("/dev/null", "w", stderr);
}

void log_agent(const HAS::Agent &agent) {
    cerr << "("
        << agent.id() << ", "
        << (agent.team() == HAS::Team::FIRST ? "First" : "Second") << ", "
        << (agent.type() == HAS::AgentType::POLICE ? "Police" : "Theif")
        << ")";
}

void log_turn(const GameView &gameView) {
    cerr << "turn=" << gameView.turn().turnnumber() << endl;
}

const AI::Phone* my_phone;
const Graph* graph;
World* world;

struct AIAgent {
    virtual void initialize(const GameView &gameView) {}
    virtual int starting_node(const GameView &gameView) {
        cerr << "bad" << endl;
        return 2;
    };
    virtual int turn(const GameView &gameView) {
        cerr << "bad" << endl;
        return 2;
    };
    ShortestPath* get_selfmap(const GameView &gameView) {
        return world->get_map(gameView.balance())[gameView.viewer().node_id()];
    }
};

AIAgent* aiagent;

struct AIThief : AIAgent {
    mt19937 rng = mt19937(chrono::steady_clock::now().time_since_epoch().count());
    ShortestPath *policemap;

    void initialize(const GameView &gameView) {
        policemap = new ShortestPath(graph->n);
    }
    int starting_node(const GameView &gameView) {
        mt19937 rng = mt19937(SEED);
        vector<int> node_options;
        auto tmpmap = world->get_map_from(1, INF);
        int maxdist = 0;
        for (int i = 1; i < graph->n; i++)
            maxdist = max(maxdist, tmpmap->dist[i]);
        for (int i = 1; i < graph->n; i++)
            if (tmpmap->dist[i] >= maxdist - 2)
                node_options.push_back(i);
        const auto &me = world->get_self(gameView);
        const auto &teammates = world->get_teammates(gameView);
        vector<int> ans;
        int max_sum = -1;
        for(int i = 0 ; i < 10 ; i++){
            int sum = 0;
            vector<int> nodes;
            for(int j = 0 ; j < teammates.size(); j++)
                nodes.push_back(node_options[rng() % node_options.size()]);
            for(int j : nodes)
                for(int k : nodes)
                    sum += world->get_dist(j, k, INF);
            if(sum > max_sum){
                max_sum = sum;
                ans = nodes;
            }
        }
        for(int i = 0 ; i < ans.size() ; i++)
            if(teammates[i].id == me.id)
                return ans[i];
        int ind = rng() % node_options.size();
        return node_options[ind];
    }
    int turn(const GameView &gameView) {
        const auto &me = world->get_self(gameView);
        const auto &enemies = world->get_enemies(gameView);
        const auto &teammates = world->get_teammates(gameView);
        auto selfmap = get_selfmap(gameView);
        vector<int> police_nodes;
        for (const auto &police : enemies)
            police_nodes.push_back(police.node);
        policemap->update(graph, police_nodes, INF);
        vector<int> node_options;
        for (int i = 1; i < graph->n; i++) {
            if (selfmap->dist[i] < policemap->dist[i])
                node_options.push_back(i);
        }
        shuffle(begin(node_options), end(node_options), rng);
        int target = random_max_by<int,int>(rng , node_options, [&] (int node) {
            int score = 0;
            for (const auto &police: enemies){
                int dist = world->get_dist(node, police.node, INF);
                score += dist;
                if(dist == 1){
                    score -= 1000 * 1000;
                }
            }
            for (const auto &thief : teammates)
                score += world->get_dist(node, thief.node, INF);
            cerr << "score " << node << " " << score << " ";
            for(int i = 1; i <= graph->n; i++)
                if(world->get_dist(node, i , me.balance) <= 2)
                    score++;
            cerr << score << endl;
            return score;
        });
        return selfmap->first[target];
    }
};

struct AIPolice : AIAgent {
    mt19937 rng = mt19937(SEED);
    vector<WorldAgent> minimax_order;
    vector<int> choice_node, choice_value;
    int starting_target = -1 , target_id = -1;

    int turn(const GameView &gameView) {
        int turn_number = gameView.turn().turnnumber();
        vector<int> visible_turns;
        const auto& visible = gameView.config().turnsettings().visibleturns();
        const auto &me = world->get_self(gameView);
        const auto &enemies = world->get_enemies(gameView);
        auto selfmap = get_selfmap(gameView);
        for(const auto& turn : visible){
            visible_turns.push_back(turn);
        }
        if (enemies.empty()) {
            if(starting_target == -1) {
                mt19937 rng = mt19937(chrono::steady_clock::now().time_since_epoch().count());
                int cnt_edge = (visible_turns[0] - turn_number) / 2;
                vector<int> max_dist , options;
                int min_r = graph->n * 10;
                max_dist.push_back(min_r);
                for(int i = 1; i <= graph->n; i++){
                    max_dist.push_back(-1);
                    if(selfmap->dist[i] > cnt_edge)
                        continue;
                    for(int j = 1; j <= graph->n; j++)
                        max_dist[i] = max(max_dist[i] , world->get_dist(i, j, INF));
                    min_r = min(min_r , max_dist[i]);
                }
                for(int i = 1; i <= graph->n; i++)
                    if(max_dist[i] <= min_r + 1)
                        options.push_back(i);
                int ind = rng() % options.size();
                starting_target = options[ind];
                cerr << "starting target=" << starting_target << ", " << "options=" << options.size() << endl;
            }
            return selfmap->first[starting_target];
        }
        const auto &polices = world->get_teammates(gameView);
        if(target_id == -1 || world->agents[target_id].dead){
            target_id = min_by<WorldAgent,int>(enemies, [&] (WorldAgent const& agent) {
                int score = 0, node = agent.node;
                for (const auto &police : polices)
                    score += world->get_dist(node, police.node, INF);
                for (int i = 1; i <= graph->n; i++){
                    int flag = 1;
                    for(const auto &police : polices){
                        int thief_dist = world->get_dist(agent, i);
                        int police_dist = world->get_dist(police, i) + (world->current_turn - agent.last_seen) / 2;
                        if(thief_dist >= police_dist - 1){
                            flag = 0;
                        }
                    }
                    score += flag;
                }
                cerr << "node=" << node << ", " << "score=" << score << endl;
                return score;
            }).id;
        }
        WorldAgent target = world->agents[target_id];
        minimax_order.clear();
        for (const auto& agent: polices)
            minimax_order.push_back(agent);
        minimax_order.push_back(target);
        choice_node.assign(minimax_order.size(), 0);
        choice_value.assign(minimax_order.size(), INT_MAX);
        cerr << "minimax: ";
        for (const auto& agent: minimax_order) {
            cerr << "(" << agent.id << ", " << agent.node << ", ";
            if (agent.type == HAS::AgentType::POLICE)
                cerr << "police";
            else
                cerr << "thief";
            cerr << ") ";
        }
        cerr << endl;
        minimax(1, 0);
        int choice;
        for (int i = 0; i < minimax_order.size(); i++) {
            if (minimax_order[i].id == me.id) {
                choice = choice_node[i];
                cerr << "choice=" << choice << " score=" << choice_value[i] << endl;
            }
        }
        return choice;
    }
    int minimax(int level, int ind) {
        if (ind == minimax_order.size()) {
            ind = 0;
            level--;
        }
        if (level == 0) {
            const auto& target = minimax_order.back();
            int score = 0;
            for (int i = 0; i < minimax_order.size()-1; i++) {
                const auto& police = minimax_order[i];
                score += world->get_dist(police.node, target.node, police.balance);
            }
            for(int i = 1; i <= graph->n; i++){
                int flag = 1;
                for (int j = 0; j < minimax_order.size()-1; j++) {
                    const auto& police = minimax_order[j];
                    int thief_dist = world->get_dist(target, i);
                    int police_dist = world->get_dist(police, i) + min(2, (world->current_turn - target.last_seen) / 2);
                    if(thief_dist >= police_dist - 1){
                        flag = 0;
                        break;
                    }
                }
                score += flag;
            }
            return score;
        }
        const auto& now = minimax_order[ind];
        if (now.type == HAS::AgentType::POLICE) {
            int result = INT_MAX;
            for (const auto& edge : world->get_options(now.node)) {
                if (edge.price > now.balance)
                    continue;
                WorldAgent nxt = now;
                nxt.node = edge.v;
                nxt.balance -= edge.price;
                minimax_order[ind] = nxt;
                int score = minimax(level, ind+1);
                minimax_order[ind] = now;
                result = min(result, score);
                if (score < choice_value[ind]) { // and first layer
                    choice_value[ind] = score;
                    choice_node[ind] = edge.v;
                }
            }
            return result;
        } else {
            int result = INT_MIN;
            if (now.last_seen == world->current_turn)
                for (int i = 0; i < minimax_order.size() - 1; i++)
                    if (now.node == minimax_order[i].node)
                        return result;
            for (const auto& edge : world->get_options(now.node)) {
                if (edge.price > now.balance)
                    continue;
                WorldAgent nxt = now;
                nxt.node = edge.v;
                nxt.balance -= edge.price;
                minimax_order[ind] = nxt;
                int score = minimax(level, ind+1);
                minimax_order[ind] = now;
                result = max(result, score);
            }
            return result;
        }
    }
};

namespace AI {
    void initialize(const GameView &gameView, const Phone &phone) {
        my_phone = &phone;
        auto me = gameView.viewer();
        redirect_cerr(me.id());
        log_agent(me);
        cerr << endl;
        graph = new Graph(gameView.config().graph());
        world = new World();
        world->initialize(gameView, graph);
        world->update(gameView);
        if (me.type() == HAS::AgentType::POLICE)
            aiagent = new AIPolice();
        else
            aiagent = new AIThief();
        aiagent->initialize(gameView);
    }

    int get_thief_starting_node(const GameView &gameView) {
        int node = aiagent->starting_node(gameView);
        cerr << "start from " << node << endl;
        return node;
    }

    int thief_move_ai(const GameView &gameView) {
        log_turn(gameView);
        world->update(gameView);
        return aiagent->turn(gameView);
    }

    int police_move_ai(const GameView &gameView) {
        log_turn(gameView);
        world->update(gameView);
        return aiagent->turn(gameView);
    }
}
