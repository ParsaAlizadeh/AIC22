#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>

#include "exceptions.h"
#include "hide_and_seek.grpc.pb.h"
#include "../ai/ai.h"
#include "client.h"


namespace Client {
    using grpc::Channel;
    using grpc::ClientContext;
    using grpc::Status;
    using grpc::ClientReader;
    using grpc::ClientAsyncReader;
    using grpc::CompletionQueue;

    using namespace ir::sharif::aic::hideandseek::api::grpc;

    void throw_if_error(const Status &status) {
        if (!(status).ok()) {
//            throw Exceptions::RpcFailedException((status).error_code(), (status).error_message());
            std::cout << "Error: " << status.error_message() << std::endl;
        }
    }

    class ClientImpl: Client {
    public:
        explicit ClientImpl(const std::string &token, const std::string &address)
                : Client(), token(token) {
            try {
                channel = grpc::CreateChannel(address, grpc::InsecureChannelCredentials());
                stub_ = GameHandler::NewStub(channel);
            } catch(...) {
                throw Exceptions::ClientCreationException(token, address);
            }
        }

        void handle_client() const {
            WatchCommand request;
            ClientContext context;
            GameView gameView;

            request.set_token(token);

            std::unique_ptr<ClientReader<GameView>> reader(stub_->Watch(&context, request));
            bool first_turn = true;
            int my_turn;
            while (reader->Read(&gameView)) {
                if (first_turn) {
                    perform_initialize(gameView);
                    my_turn = gameView.turn().turnnumber();
                } else if (gameView.status() == GameStatus::ONGOING) {
                    if(my_turn == gameView.turn().turnnumber())
                        continue;
                    bool both_theif  = gameView.turn().turntype() == TurnType::THIEF_TURN && gameView.viewer().type() == AgentType::THIEF;
                    bool both_police = gameView.turn().turntype() == TurnType::POLICE_TURN && gameView.viewer().type() == AgentType::POLICE;
                    if(!both_police && !both_theif)
                        continue;
                    perform_move(gameView);
                    my_turn = gameView.turn().turnnumber();
                } else if (gameView.status() == GameStatus::FINISHED) {
                    break;
                } // todo: alive condition...
                first_turn = false;
            }
            Status status = reader->Finish();
            throw_if_error(status);
        }

    private:
        void DeclareReadiness(const int start_node_id) const {
            DeclareReadinessCommand request;
            ClientContext context;
            ::google::protobuf::Empty reply;
            request.set_token(token);
            request.set_startnodeid(start_node_id);
            Status status = stub_->DeclareReadiness(&context, request, &reply);
            throw_if_error(status);
        }

        void SendMessage(const ChatCommand &chatCommand) const {
            ClientContext context;
            ::google::protobuf::Empty reply;
            Status status = stub_->SendMessage(&context, chatCommand, &reply);
            throw_if_error(status);
        }

        void SendMessage(const std::string& message) const override {
            ChatCommand chatCommand;
            chatCommand.set_token(token);
            chatCommand.set_text(message);
            SendMessage(chatCommand);
        }

        void Move(const MoveCommand &moveCommand) const {
            ClientContext context;
            ::google::protobuf::Empty reply;
            Status status = stub_->Move(&context, moveCommand, &reply);
            throw_if_error(status);
        }

        void perform_initialize(const GameView &gameView) const {
            const auto &viewer = gameView.viewer();
            int start_node_id;
            if (viewer.type() == AgentType::THIEF) {
                start_node_id = AI::get_thief_starting_node(gameView);
            } else {
                start_node_id = 3; // todo dummy
            }
            DeclareReadiness(start_node_id);
            AI::initialize(gameView, Phone(this));
        }

        void perform_move(const GameView &gameView) const {
            const auto &viewer = gameView.viewer();
            int to_node_id;
            if (viewer.type() == AgentType::THIEF) {
                to_node_id = AI::thief_move_ai(gameView);
            } else {
                to_node_id = AI::police_move_ai(gameView);
            }
            MoveCommand moveCommand;
            moveCommand.set_token(token);
            moveCommand.set_tonodeid(to_node_id);
            Move(moveCommand);
        }

        std::unique_ptr<GameHandler::Stub> stub_;
        std::shared_ptr<Channel> channel;
        const std::string token;
    };


    void handle_client(const std::string &token, const std::string &address) {
        ClientImpl client(token, address);
        client.handle_client();
    }
}