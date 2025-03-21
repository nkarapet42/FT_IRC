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

void    Client::createChannel(const std::string& channelName, const std::string& password) {
    Channel newChannel;
    newChannel.channelName = channelName;
    newChannel.channelPass = password;
    newChannel.havePass = !password.empty();
    newChannel.isTopic = false;
    newChannel.haveLimit = false;
    newChannel.isInviteOnly = false;
    newChannel.members.push_back(nickname);
    channelsIRC.push_back(newChannel);

    Info newInfo;
    newInfo.channelName = channelName;
    newInfo.password = password;
    newInfo.isOperator = true;
    newInfo.members.push_back(nickname);
    channels.push_back(newInfo);

    this->curchannel = channelName;
    this->isOperator = true;
}

void    Client::joinChannel(const std::string& channelName, const std::string& password) {
    for (size_t i = 0; i < channelsIRC.size(); i++) {
        if (channelsIRC[i].channelName == channelName) {
            for (size_t i = 0; i < channels.size(); i++) {
                if (channels[i].channelName == channelName) {
                    curchannel = channelName;
                    return ;
                }
            }
            if (channelsIRC[i].havePass) {
                if (channelsIRC[i].isUserInvited(nickname)) {
                    std::cout << nickname << " joined channel: " << channelName << "\n";
                    curchannel = channelName;
                    channelsIRC[i].members.push_back(nickname);
                    Info newChannel;
                    newChannel.channelName = channelName;
                    newChannel.password = password;
                    newChannel.isOperator = (channelsIRC[i].members[0] == nickname); 
                    newChannel.members.push_back(nickname);
                    channels.push_back(newChannel);
                    return;
                }
            }
            if (channelsIRC[i].channelPass != password) {
                std::cout << "Incorrect password. Access denied.\n";
                return ;
            }
            std::cout << nickname << " joined channel: " << channelName << "\n";
            curchannel = channelName;
            channelsIRC[i].members.push_back(nickname);

            if (channelsIRC[i].members[0] == nickname) {
                this->isOperator = true;
            }
            Info newChannel;
            newChannel.channelName = channelName;
            newChannel.password = password;
            newChannel.isOperator = (channelsIRC[i].members[0] == nickname); 
            newChannel.members.push_back(nickname);
            channels.push_back(newChannel);
            return ;
        }
    }
    createChannel(channelName, password);
}

bool    Client::leaveChannel(const std::string& channel) {
    for (size_t i = 0; i < channels.size(); ++i) {
        if (channels[i].channelName == channel) {
            std::vector<std::string>::iterator it = channels[i].members.begin();
            while (it != channels[i].members.end()) {
                if (*it == nickname) {
                    it = channels[i].members.erase(it);
                } else {
                    ++it;
                }
            }
            if (channels[i].members.empty()) {
                for (std::vector<Channel>::iterator ch_it = channelsIRC.begin(); ch_it != channelsIRC.end(); ++ch_it) {
                    if (ch_it->channelName == channel) {
                        
                        break;
                    }
                }
            }
            channels.erase(channels.begin() + i);
            for (size_t j = 0; j < channelsIRC.size(); ++j) {
                if (channelsIRC[j].channelName == channel) {
                    std::vector<std::string>::iterator it = std::find(
                        channelsIRC[j].members.begin(), channelsIRC[j].members.end(), nickname);
                    if (it != channelsIRC[j].members.end()) {
                        channelsIRC[j].members.erase(it);
                        if (channelsIRC[j].members.empty()) {
                            channelsIRC.erase(channelsIRC.begin() + j);
                            std::cout << "Channel " << channel << " has been deleted (no members left)." << std::endl;
                        }
                    }
                }
            }
            std::cout << nickname << " Left channel: " << channel << std::endl;
            if (curchannel == channel) {
                curchannel = channels.empty() ? "" : channels.back().channelName;
            }
            return true;
        }
    }
    std::cout << nickname << " You are not in channel: " << channel << std::endl;
    return false;
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
