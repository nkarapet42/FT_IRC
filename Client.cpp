#include "Client.hpp"

Client::Client() {}

Client::Client(int fd) : fd(fd), isAuthenticated(false), isOperator(false) {}

Client::Client(const Client& other) : fd(other.fd), nickname(other.nickname), username(other.username),
                                      channels(other.channels), isAuthenticated(other.isAuthenticated),
                                      isOperator(other.isOperator) {}

Client& Client::operator=(const Client& other) {
    if (this != &other) {
        fd = other.fd;
        nickname = other.nickname;
        username = other.username;
        channels = other.channels;
        isAuthenticated = other.isAuthenticated;
        isOperator = other.isOperator;
    }
    return (*this);
}

Client::~Client() {}

void    Client::setNickname(const std::string& nick) {
    nickname = nick;
}

void    Client::setUsername(const std::string& user) {
    username = user;
}

// void    Client::joinChannel(const std::string& channel) {
//     channels.insert(channel);
// }

// void    Client::leaveChannel(const std::string& channel) {
//     channels.erase(channel);
// }