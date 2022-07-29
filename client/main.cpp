#include "client/client.h"
#include "yaml-cpp/yaml.h"

int main(int argc, char** argv) {
    if(argc != 2) {
        throw std::runtime_error("number of arguments should be 1:\n1-client token");
    }
    // std::string config_file_path = argv[2];
    // YAML::Node config = YAML::LoadFile(config_file_path);
    const std::string token = argv[1];
    // const auto token = config["token"].as<std::string>();
    // const auto host = config["grpc"]["server"].as<std::string>();
    // const auto port = config["grpc"]["port"].as<std::string>();
    const std::string host = "127.0.0.1";
    const std::string port = "7000";
    const auto address = host + ":" + port;
    Client::handle_client(token, address);
    return 0;
}
