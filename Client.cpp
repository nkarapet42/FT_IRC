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

const std::string&  Client::getNickname() {
    return (nickname);
}

void    Client::joinChannel(const std::string& channelName, const std::string& password) {
    for (size_t i = 0; i < channelsIRC.size(); i++) {
        if (channelsIRC[i].channelName == channelName) {
            if (channelsIRC[i].havePass && channelsIRC[i].channelPass != password) {
                std::cout << "Incorrect password. Access denied.\n";
                return ;
            }
            std::cout << nickname << " joined channel: " << channelName << "\n";
            curchannel = channelName;
            std::cout << curchannel << "\n";

            Info newChannel;
            newChannel.channelName = channelName;
            newChannel.password = password;
            newChannel.isOperator = false;
            channels.push_back(newChannel);
            return ;
        }
    }

    Channel newChannel;
    newChannel.channelName = channelName;
    newChannel.channelPass = password;
    newChannel.havePass = !password.empty();
    channelsIRC.push_back(newChannel);

    std::cout << nickname << " created and joined channel: " << channelName << "\n";
    curchannel = channelName;

    Info newInfo;
    newInfo.channelName = channelName;
    newInfo.password = password;
    newInfo.isOperator = true;
    channels.push_back(newInfo);
}


void Client::leaveChannel(const std::string& channel) {
    for (size_t i = 0; i < channels.size(); ++i) {
        if (channels[i].channelName == channel) {
            channels.erase(channels.begin() + i);
            std::cout << nickname << " Left channel: " << channel << std::endl;
            if (curchannel == channel) {
                curchannel = channels.empty() ? "" : channels.back().channelName;
            }
            return;
        }
    }
    std::cout << nickname << " You are not in channel: " << channel << std::endl;
}


void    Client::changeChannelPassword(const std::string& channelName, const std::string& newPassword) {
    for (size_t i = 0; i < channelsIRC.size(); i++) {
        if (channelsIRC[i].channelName == channelName) {
            isOperator = false;
            for (size_t j = 0; j < channels.size(); j++) {
                if (channels[j].channelName == channelName && channels[j].isOperator) {
                    isOperator = true;
                    break;
                }
            }

            if (!isOperator) {
                std::cout << "You must be an operator to change the password.\n";
                return;
            }
            channelsIRC[i].channelPass = newPassword;
            channelsIRC[i].havePass = !newPassword.empty();
            std::cout << "Password for channel " << channelName << " changed.\n";

            for (size_t j = 0; j < channels.size(); ) {
                if (channels[j].channelName == channelName && !channels[j].isOperator) {
                    channels.erase(channels.begin() + j);
                } else {
                    j++;
                }
            }

            return;
        }
    }

    std::cout << "Channel not found.\n";
}
