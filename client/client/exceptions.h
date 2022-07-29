#ifndef HIDEANDSEEK_EXCEPTIONS_H
#define HIDEANDSEEK_EXCEPTIONS_H

#include <utility>

namespace Exceptions {
    class MyException : public std::exception {
    public:
        explicit MyException(const std::string &my_error_string) {
            strcpy(error_string, my_error_string.c_str());
        }

        const char *what() const noexcept override {
            return error_string;
        }

    private:
        char error_string[200];
    };

    class RpcFailedException : public MyException {
    public:
        explicit RpcFailedException(const int error_code, const std::string &error_message)
                : MyException("code:" + std::to_string(error_code) + " (" + error_message + ")") {}
    };


    class ClientCreationException : public MyException {
    public:
        explicit ClientCreationException(const std::string &token, const std::string &address)
                : MyException("cannot create client on address: " + address + " and with token " + token) {}
    };
}

#endif
