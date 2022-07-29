#ifndef AIC22_CLIENT_CPP_CLIENT_H
#define AIC22_CLIENT_CPP_CLIENT_H

#include <string>

namespace Client {
    void handle_client(const std::string &token, const std::string &address);

    class Client {
    public:
        Client(const Client &client) = delete;
        Client &operator=(const Client &client) = delete;
        Client() = default;

        class Phone {
            const Client *client_pt;
        public:
            explicit Phone(const Client *client_pt) : client_pt(client_pt) {}

            void send_message(const std::string &message) const { client_pt->SendMessage(message); }
        };
    private:
        virtual void SendMessage(const std::string &message) const {} // base class does nothing
    };
}

#endif
