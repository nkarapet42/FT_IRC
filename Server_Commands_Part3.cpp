#include "Server.hpp"

void	Server::invite(int clientFd, const std::string& channel, const std::string& nick) {
	for (std::vector<Channel>::iterator it = channelsIRC.begin(); it != channelsIRC.end(); ++it) {
		if (it->channelName == channel) {
			bool isInviterMember = false;
			for (size_t i = 0; i < it->members.size(); i++) {
				if (it->members[i] == _clients[clientFd].nickname) {
					isInviterMember = true;
					break;
				}
			}
			if (!isInviterMember) {
				sendMessage(clientFd, std::string(RED) + "Error: You are not a member of this channel.\n" + std::string(RESET));
				return;
			}
			bool isInviteeMember = false;
			for (size_t i = 0; i < it->members.size(); i++) {
				if (it->members[i] == nick) {
					isInviteeMember = true;
					break;
				}
			}
			if (isInviteeMember) {
				sendMessage(clientFd, std::string(RED) + "Error: User is already in the channel.\n" + std::string(RESET));
				return;
			}
			int inviteeFd = -1;
			for (std::map<int, Client>::iterator clientIt = _clients.begin(); clientIt != _clients.end(); ++clientIt) {
				if (clientIt->second.nickname == nick) {
					inviteeFd = clientIt->first;
					break;
				}
			}
			if (inviteeFd == -1) {
				sendMessage(clientFd, std::string(RED) + "Error: User not found.\n" + std::string(RESET));
				return;
			}
			sendMessage(clientFd, std::string(GREEN) + "User " + nick + " has been invited to the channel " + channel + ".\n" + std::string(RESET));
			sendMessage(inviteeFd, std::string(CYAN) + "You have been invited to join channel " + channel + " by " + _clients[clientFd].nickname + ".\n" + std::string(RESET));
			return;
		}
	}
	sendMessage(clientFd, std::string(RED) + "Error: The channel doesn't exist.\n" + std::string(RESET));
}

void	Server::join(int clientFd, const std::string& channelName, const std::string& password) {
    if (_clients[clientFd].curchannel == channelName) {
        sendMessage(clientFd, std::string(RED) + "Error: You are already in the channel.\n" + std::string(RESET));
        return ;
    }
    if (_clients[clientFd].getNickname().empty()) {
        sendMessage(clientFd, std::string(RED) + "Error: Please add a nickname, then try to join.\n" + std::string(RESET));
        return ;
    }
    for (size_t i = 0; i < channelsIRC.size(); i++) {
        if (channelsIRC[i].channelName == channelName) {
            if (channelsIRC[i].havePass && channelsIRC[i].channelPass != password) {
                sendMessage(clientFd, std::string(RED) + "Error: Incorrect password. Access denied.\n" + std::string(RESET));
                return ;
            }
            if (!channelsIRC[i].havePass && !password.empty()) {
                sendMessage(clientFd, std::string(RED) + "Error: This channel does not have password, try without passWord.\n" + std::string(RESET));
                return ;
            }
            sendMessage(clientFd,  std::string(CYAN) + _clients[clientFd].getNickname() + " joined channel: " + channelName + "\n" +  std::string(RESET));
            _clients[clientFd].joinChannel(channelName, password);
            return ;
        }
    }
    sendMessage(clientFd,  std::string(CYAN) + _clients[clientFd].getNickname() + " created and joined channel: " + channelName + "\n" + std::string(RESET));
    _clients[clientFd].joinChannel(channelName, password);
}

void	Server::modeCommand(int clientFd, const std::string& channel, const std::string& mode, const std::string& param) {
	for (std::vector<Channel>::iterator it = channelsIRC.begin(); it != channelsIRC.end(); ++it) {
		if (it->channelName == channel) {
			if (!_clients[clientFd].isOperator) {
				sendMessage(clientFd, std::string(RED) + "Error: You must be a channel operator to change modes.\n" + std::string(RESET));
				return;
			}
			if (mode == "i") {
				it->havePass = !it->havePass;
				sendMessage(clientFd, std::string(GREEN) + "Invite-only mode " + (it->havePass ? "enabled" : "disabled") + " for channel " + channel + ".\n" + std::string(RESET));
			}
			else if (mode == "t") {
				it->isTopic = !it->isTopic;
				sendMessage(clientFd, std::string(GREEN) + "Topic restriction " + (it->isTopic ? "enabled" : "disabled") + " for channel " + channel + ".\n" + std::string(RESET));
			}
			else if (mode == "k") {
				if (param.empty()) {
					sendMessage(clientFd, std::string(RED) + "Error: No password provided for mode +k.\n" + std::string(RESET));
					return;
				}
				it->channelPass = param;
				it->havePass = true;
				sendMessage(clientFd, std::string(GREEN) + "Password set for channel " + channel + ".\n" + std::string(RESET));
			}
			else if (mode == "o") {
				bool userFound = false;
				for (std::map<int, Client>::iterator clientIt = _clients.begin(); clientIt != _clients.end(); ++clientIt) {
					if (clientIt->second.nickname == param) {
						clientIt->second.isOperator = !clientIt->second.isOperator;
						userFound = true;
						sendMessage(clientFd, std::string(GREEN) + "Operator status " + (clientIt->second.isOperator ? "granted" : "revoked") + " for user " + param + " in channel " + channel + ".\n" + std::string(RESET));
						break;
					}
				}
				if (!userFound) {
					sendMessage(clientFd, std::string(RED) + "Error: User not found in the channel.\n" + std::string(RESET));
				}
			}
			else if (mode == "l") {
				if (param.empty()) {
					sendMessage(clientFd, std::string(RED) + "Error: No user limit provided for mode +l.\n" + std::string(RESET));
					return;
				}
				int limit = std::atoi(param.c_str());
				if (limit <= 0) {
					sendMessage(clientFd, std::string(RED) + "Error: Invalid user limit.\n" + std::string(RESET));
					return;
				}
				it->members.resize(limit);
				sendMessage(clientFd, std::string(GREEN) + "User limit set to " + param + " for channel " + channel + ".\n" + std::string(RESET));
			}
			else {
				sendMessage(clientFd, std::string(RED) + "Error: Unknown mode.\n" + std::string(RESET));
			}
			return;
		}
	}
	sendMessage(clientFd, std::string(RED) + "Error: Channel not found.\n" + std::string(RESET));
}
