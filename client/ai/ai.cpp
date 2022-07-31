#include "ai.h"

using namespace std;

namespace AI {
    const Phone* my_phone;

    void redirect_cerr(int id) {
        string filename = "./logs/client/" + to_string(id) + ".log";
        freopen(filename.c_str(), "w", stderr);
    }

    int get_thief_starting_node(const GameView &gameView) {
        return 270;
    }

    void log_agent(const HAS::Agent &agent) {
        cerr << "("
            << agent.id() << ", "
            << (agent.team() == HAS::Team::FIRST ? "First" : "Second") << ", "
            << (agent.type() == HAS::AgentType::POLICE ? "Police" : "Theif")
            << ")";
    }

    void log_turn(const GameView &gameView) {
        cerr << "turn=" << gameView.turn().turnnumber() << " ";
        const auto visibles = gameView.visible_agents();
        cerr << "visibles=" << visibles.size() << ": ";
        for (const HAS::Agent &agent : visibles) {
            log_agent(agent);
            cerr << ", ";
        }
        cerr << endl;
    }

    void initialize(const GameView &gameView, const Phone &phone) {
        my_phone = &phone;
        auto me = gameView.viewer();
        redirect_cerr(me.id());
        log_agent(me);
        cerr << endl;
    }

    int thief_move_ai(const GameView &gameView) {
        log_turn(gameView);
        return 2;
    }

    int police_move_ai(const GameView &gameView) {
        log_turn(gameView);
        return 1;
    }
}