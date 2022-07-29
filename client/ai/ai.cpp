#include "ai.h"

using namespace std;

namespace AI {
    const Phone* my_phone;

    int get_thief_starting_node(const GameView &gameView) {
        return 2;
    }

    void initialize(const GameView &gameView, const Phone &phone) {
        cout << "I will send a message using my phone" << endl;
        // can't send message here
        my_phone = &phone;
    }

    int thief_move_ai(const GameView &gameView) {
        // we can send message if we like
        cout << "I am moving and then sending a message as a theif" << endl;
        my_phone->send_message("000010"); // sample text
        return 2;
    }

    int police_move_ai(const GameView &gameView) {
        // we can send message if we like
        cout << "I am moving and sending a message as a police" << endl;
        my_phone->send_message("1101"); // sample text
        return 1;
    }
}