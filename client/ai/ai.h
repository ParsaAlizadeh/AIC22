#ifndef HIDEANDSEEK_AI_H
#define HIDEANDSEEK_AI_H

#include "types.h"
#include "client/client.h"

namespace AI {
    using namespace Types;
    using Phone = Client::Client::Phone;

    int get_thief_starting_node(const GameView &gameView); // returns node_id

    // for thief, it would get called after get_thief starting node
    // user can use this to initialize itself and save the phone
    void initialize(const GameView &gameView, const Phone &phone);

    int thief_move_ai(const GameView &gameView); // returns to_node_id
    int police_move_ai(const GameView &gameView); // returns to_node_id
}

#endif
