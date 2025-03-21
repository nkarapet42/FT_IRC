#include "Server.hpp"

void	Server::invite(int clientFd, const std::string& channel, const std::string& nick) {
	if (nick.empty()) {
        sendErrorMessage(clientFd, "Error: Wrong Syntax.", 461);
		sendMessage(clientFd, std::string(RED) + "Usage: INVITE <channel> [nick].\n" + std::string(RESET), "INVITE", 461);
		return;
	}
	for (size_t i = 0; i < _clients[clientFd].channels.size(); i++) {
		if (_clients[clientFd].channels[i].channelName == channel) {
			if (!_clients[clientFd].channels[i].isOperator) {
				sendErrorMessage(clientFd, "Error: You must be an operator to invite.", 482);
				return;
			} else 
				break ;
		}
	}
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
				sendErrorMessage(clientFd, "Error: You are not a member of this channel.", 442);
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
				sendErrorMessage(clientFd, "Error: User is already a member of the channel.", 443);
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
				sendErrorMessage(clientFd, "Error: User not found.", 401);
				return;
			}
			it->invited.push_back(nick);
			sendMessage(clientFd, std::string(GREEN) + "User " + nick + " has been invited to the channel " + channel + ".\n" + std::string(RESET), "INVITE", 341);
			sendMessage(inviteeFd, std::string(CYAN) + "You have been invited to join channel " + channel + " by " + _clients[clientFd].nickname + ".\n" + std::string(RESET), "INVITE", 341);
			return;
		}
	}
	sendErrorMessage(clientFd, "Error: Channel not found.", 403);
}


void	Server::join(int clientFd, const std::string& channelName, const std::string& password) {
	if (!isChannelName(channelName)) {
		sendErrorMessage(clientFd, "Error: Invalid channel name.", 403);
		return;
	} 
	if (_clients[clientFd].curchannel == channelName) {
		sendErrorMessage(clientFd, "Error: You are already in this channel.", 443);
		return ;
	}
	if (_clients[clientFd].getNickname().empty()) {
		sendErrorMessage(clientFd, "Error: You must set a nickname before joining a channel.", 431);
		return ;
	}
	if (channelName.empty()) {
		sendErrorMessage(clientFd, "Error: No channel name provided.", 461);
		return ;
	}
	for (size_t i = 0; i < channelsIRC.size(); i++) {
		if (channelsIRC[i].channelName == channelName) {
			if (channelsIRC[i].haveLimit &&channelsIRC [i].members.size() >= static_cast<std::vector<std::string>::size_type>(channelsIRC[i].limit)) {
				sendErrorMessage(clientFd, "Error: Channel is full.", 471);
				return ;
			}
			if ((channelsIRC[i].isInviteOnly && !channelsIRC[i].isUserInvited(_clients[clientFd].getNickname()))
					&& (!channelsIRC[i].isUserMember(_clients[clientFd].getNickname()))) {
				sendErrorMessage(clientFd, "Error: Channel is invite-only.", 473);
				return ;
			}
			if (channelsIRC[i].havePass && channelsIRC[i].channelPass != password) {
				sendErrorMessage(clientFd, "Error: Invalid password.", 464);
				return ;
			}
			if (!channelsIRC[i].havePass && !password.empty()) {
				sendErrorMessage(clientFd, "Error: Channel does not have a password.", 464);
				return ;
			}
			std::string newMessage = ":" + _clients[clientFd].nickname + "!" + _clients[clientFd].username + "@FT_IRC JOIN :" + channelName + "\n";
			send(clientFd, newMessage.c_str(), newMessage.length(), 0);
			_clients[clientFd].joinChannel(channelName, password);
			return ;
		}
	}
	std::string newMessage = ":" + _clients[clientFd].nickname + "!" + _clients[clientFd].username + "@FT_IRC JOIN :" + channelName + "\n";
	send(clientFd, newMessage.c_str(), newMessage.length(), 0);
	_clients[clientFd].joinChannel(channelName, password);
}

void	Server::modeCommand(int clientFd, const std::string& channel, const std::string& mode, const std::string& param) {
	for (std::vector<Channel>::iterator it = channelsIRC.begin(); it != channelsIRC.end(); ++it) {
		if (it->channelName == channel) {
			for (size_t i = 0; i < _clients[clientFd].channels.size(); i++) {
				if (_clients[clientFd].channels[i].channelName == channel) {
					if (!_clients[clientFd].channels[i].isOperator) {
						sendErrorMessage(clientFd, "Error: You must be an operator to change channel modes.", 482);
						return;
					} else 
						break ;
				}
			}
			if (mode == "i") {
				it->isInviteOnly = !it->isInviteOnly;
				it->havePass = !it->havePass;
				sendMessage(clientFd, std::string(GREEN) + "Invite-only mode " + (it->havePass ? "enabled" : "disabled") + " for channel " + channel + ".\n" + std::string(RESET), "MODE", 324);
			}
			else if (mode == "t") {
				it->isTopic = !it->isTopic;
				sendMessage(clientFd, std::string(GREEN) + "Topic restriction " + (it->isTopic ? "enabled" : "disabled") + " for channel " + channel + ".\n" + std::string(RESET), "MODE", 324);
			}
			else if (mode == "k") {
				if (param.empty()) {
					sendErrorMessage(clientFd, "Error: No password provided for mode +k.", 461);
					return;
				}
				it->channelPass = param;
				it->havePass = true;
				sendMessage(clientFd, std::string(GREEN) + "Password set for channel " + channel + ".\n" + std::string(RESET), "MODE", 342);
			}
			else if (mode == "o") {
				bool userFound = false;
				for (std::map<int, Client>::iterator clientIt = _clients.begin(); clientIt != _clients.end(); ++clientIt) {
					if (clientIt->second.nickname == param) {
						for (size_t i = 0; i < _clients[clientFd].channels.size(); i++) {
							if (clientIt->second.channels[i].channelName == channel) {
								if (param == _clients[clientFd].nickname) {
									sendErrorMessage(clientFd, "Error: You can't change the mode by yourself.", 401);
									return;
								}
								clientIt->second.channels[i].isOperator = !clientIt->second.channels[i].isOperator;
								userFound = true;
								sendMessage(clientFd, std::string(GREEN) + "Operator status " + (clientIt->second.isOperator ? "granted" : "revoked") + " for user " + param + " in channel " + channel + ".\n" + std::string(RESET), "MODE", 324);
								break;
							}
						}
					}
				}
				if (!userFound) {
					sendErrorMessage(clientFd, "Error: User not found.", 441);
				}
			}
			else if (mode == "l") {
				it->haveLimit = !it->haveLimit;
				if (param.empty()) {
					sendErrorMessage(clientFd, "Error: No limit provided for mode +l.", 461);
					return;
				}
				it->limit = std::atoi(param.c_str());
				if (it->limit <= 0) {
					sendErrorMessage(clientFd, "Error: Invalid limit.", 461);
					return;
				}
				sendMessage(clientFd, std::string(GREEN) + "User limit set to " + param + " for channel " + channel + ".\n" + std::string(RESET), "MODE", 324);
			}
			else {
				sendErrorMessage(clientFd, "Error: Invalid mode.", 472);
			}
			return;
		}
	}
	sendErrorMessage(clientFd, "Error: Channel not found.", 403);
}

