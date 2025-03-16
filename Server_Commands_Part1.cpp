#include "Server.hpp"

void Server::quitClient(int clientFd, const std::string& line) {
	std::stringstream ss(line);
	std::string cmd;
	ss >> cmd;
	std::string message;
	std::getline(ss, message);
	endErase(message);
	std::map<int, Client>::iterator it = _clients.find(clientFd);
	if (it == _clients.end()) {
		return;
	}

	std::string quitMessage = "Client " + it->second.nickname + " has left the server";
	if (!message.empty()) {
		quitMessage += " (" + message + ")";
	} else {
		quitMessage += " (All eyes on me)";
	}
	quitMessage += ".\n";

	for (size_t i = 0; i < it->second.channels.size(); ++i) {
		broadcastMessage(it->second.channels[i].channelName, clientFd, std::string(CYAN) + quitMessage + std::string(RESET), "QUIT", 5);
	}
	for (size_t i = 0; i < it->second.channels.size(); ++i) {
		it->second.leaveChannel(it->second.channels[i].channelName);
	}
	std::cout << "Client disconnected: FD " << clientFd << "\n";
	for (std::map<std::string, FileTransfer>::iterator it = activeTransfers.begin(); it != activeTransfers.end(); ++it) {
		if (it->second.senderFd == clientFd) {
			activeTransfers.erase(it);
		} 
	}
	_clients.erase(it);
	close(clientFd);
}

void	Server::setUsername(int clientFd, const std::string& user) {
	if (user.empty()) {
		sendErrorMessage(clientFd, "Error: Username cannot be empty.", 461);
		return;
	}
	_clients[clientFd].setUsername(user);
	sendMessage(clientFd, std::string(CYAN) + "Your username has been seted.\n" + std::string(RESET), "USER", 1);
}

void Server::changeNickname(int clientFd, const std::string& newNick) {
	if (newNick.empty()) {
		sendErrorMessage(clientFd, "Error: Nickname cannot be empty.", 431);
		return;
	}
	
	for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
		if (it->second.nickname == newNick) {
			sendErrorMessage(clientFd, "Error: Nickname is already in use.", 433);
			return;
		}
	}
	
	std::string oldNick = _clients[clientFd].nickname;
	_clients[clientFd].setNickname(newNick);
	
	sendMessage(clientFd,  std::string(CYAN) + "Your nickname has been changed to " + newNick + "\n" + std::string(RESET), "NICK", 6);
	
	for (size_t i = 0; i < _clients[clientFd].channels.size(); i++) {
		std::string channel = _clients[clientFd].channels[i].channelName;
		broadcastMessage(channel , clientFd, "User " + oldNick + " is now known as " + newNick, "NICK", 6);
	}
}

void	Server::privateNoticeMessage(int clientFd, const std::string& line) {
	std::stringstream ss(line);
	std::string cmd, client, message;
	ss >> cmd >> client;
	endErase(cmd);
	endErase(client);
	if (client.empty()) {
		sendErrorMessage(clientFd, "Error: No nickname provided.", 411);
		return;
	}
	if (_clients[clientFd].nickname == client) {
		sendErrorMessage(clientFd, "Error: Cant send message to yourself.", 404);
		return;
	}
	std::getline(ss, message);
	endErase(message);
	if (!message.empty() && message[0] == ' ')
		message = message.substr(1);
	if (!message.empty()) {
		for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
			if (it->second.nickname == client) {
			sendMessage(it->second.fd, std::string(CYAN) + cmd + " : [ " + _clients[clientFd].nickname +" -> " + client + " ]: " + message + "\n" + std::string(RESET), "PRIVMSG", 8);
				return;
			}
		}
		sendErrorMessage(clientFd, "Error: Nickname not found.", 401);
	}
	else
		sendErrorMessage(clientFd, "Error: No message provided.", 412);
}

void Server::kick(int clientFd, const std::string& channel, const std::string& nick) {
    for (std::vector<Channel>::iterator it = channelsIRC.begin(); it != channelsIRC.end(); ++it) {
        if (it->channelName == channel) {
            bool memberFound = false;
            bool userToKickFound = false;

            for (std::vector<std::string>::iterator memberIt = it->members.begin(); memberIt != it->members.end(); ++memberIt) {
                if (*memberIt == _clients[clientFd].nickname) {
                    memberFound = true;
                }
                if (*memberIt == nick) {
                    userToKickFound = true;
                }
            }

            if (!memberFound) {
				sendErrorMessage(clientFd, "Error: You are not a member of this channel.", 442);
                return;
            }
            if (!userToKickFound) {
				sendErrorMessage(clientFd, "Error: No member with this nick in the channel.", 441);
                return;
            }
            if (nick == _clients[clientFd].nickname) {
				sendErrorMessage(clientFd, "Error: You can't kick yourself.", 401);
                return;
            }
            if (!_clients[clientFd].isOperator) {
				sendErrorMessage(clientFd, "Error: You are not an operator of this channel.", 482);
                return;
            }

            for (std::vector<std::string>::iterator memberIt = it->members.begin(); memberIt != it->members.end(); ++memberIt) {
                if (*memberIt == nick) {
                    it->members.erase(memberIt);
                    break;
                }
            }

            for (std::vector<Info>::iterator infoIt = _clients[clientFd].channels.begin(); infoIt != _clients[clientFd].channels.end(); ++infoIt) {
                if (infoIt->channelName == channel) {
                    for (std::vector<std::string>::iterator memIt = infoIt->members.begin(); memIt != infoIt->members.end(); ++memIt) {
                        if (*memIt == nick) {
                            infoIt->members.erase(memIt);
                            break;
                        }
                    }
                }
            }

            for (std::map<int, Client>::iterator clientIt = _clients.begin(); clientIt != _clients.end(); ++clientIt) {
                if (clientIt->second.nickname == nick) {
                    clientIt->second.curchannel.clear();
                    clientIt->second.isOperator = false;

                    for (std::vector<Info>::iterator chIt = clientIt->second.channels.begin(); chIt != clientIt->second.channels.end(); ++chIt) {
                        if (chIt->channelName == channel) {
                            clientIt->second.channels.erase(chIt);
                            break;
                        }
                    }
                    break;
                }
            }

            sendMessage(clientFd, std::string(GREEN) + "User " + nick + " has been kicked from the channel " + channel + ".\n" + std::string(RESET), "KICK", 9);
            return;
        }
    }
	sendErrorMessage(clientFd, "Error: The channel doesn't exist.", 403);
}
