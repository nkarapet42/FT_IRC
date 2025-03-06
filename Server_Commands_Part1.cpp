#include "Server.hpp"

void Server::quitClient(int clientFd, const std::string& line) {
	std::stringstream ss(line);
	std::string cmd;
	ss >> cmd;
	std::string message;
	std::getline(ss, message);
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
		broadcastMessage(it->second.channels[i].channelName, clientFd, std::string(CYAN) + quitMessage + std::string(RESET));
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
		sendMessage(clientFd, std::string(RED) + "Error: Username cannot be empty.\n" + std::string(RESET));
		return;
	}
	_clients[clientFd].setUsername(user);
	sendMessage(clientFd, std::string(CYAN) + "Your username has been seted.\n" + std::string(RESET));
}

void Server::changeNickname(int clientFd, const std::string& newNick) {
	if (newNick.empty()) {
		sendMessage(clientFd, std::string(RED) + "Error: Nickname cannot be empty.\n" + std::string(RESET));
		return;
	}
	
	for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
		if (it->second.nickname == newNick) {
			sendMessage(clientFd, std::string(RED) + "Error: Nickname is already in use.\n" + std::string(RESET));
			return;
		}
	}
	
	std::string oldNick = _clients[clientFd].nickname;
	_clients[clientFd].setNickname(newNick);
	
	sendMessage(clientFd,  std::string(CYAN) + "Your nickname has been changed to " + newNick + "\n" + std::string(RESET));
	
	for (size_t i = 0; i < _clients[clientFd].channels.size(); i++) {
		std::string channel = _clients[clientFd].channels[i].channelName;
		broadcastMessage(channel , clientFd, "User " + oldNick + " is now known as " + newNick);
	}
}

void	Server::privateNoticeMessage(int clientFd, const std::string& line) {
	std::stringstream ss(line);
	std::string cmd, client, message;
	ss >> cmd >> client;
	if (_clients[clientFd].nickname == client) {
		sendMessage(clientFd, std::string(RED) + "Error: Cant send message to yourself.\n" + std::string(RESET));
		return;
	}
	std::getline(ss, message);
	if (!message.empty() && message[0] == ' ')
		message = message.substr(1);
	if (!message.empty()) {
		for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
			if (it->second.nickname == client) {
				sendMessage(it->second.fd, std::string(CYAN) + cmd + " : [ " + _clients[clientFd].nickname +" -> " + client + " ]: " + message + "\n" + std::string(RESET));
				return;
			}
		}
		sendMessage(clientFd, std::string(RED) + "Error: Nickname not found.\n" + std::string(RESET));
	}
	else
		sendMessage(clientFd, std::string(RED) + "Error: No message provided.\n" + std::string(RESET));
}

void	Server::kick(int clientFd, const std::string& channel, const std::string& nick) {
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
				sendMessage(clientFd, std::string(RED) + "Error: You are not a member of this channel.\n" + std::string(RESET));
				return;
			}
			if (!userToKickFound) {
				sendMessage(clientFd, std::string(RED) + "Error: No member with this nick in the channel.\n" + std::string(RESET));
				return;
			}
			if (nick == _clients[clientFd].nickname) {
				sendMessage(clientFd, std::string(RED) + "Error: You can't kick yourself.\n" + std::string(RESET));
				return;
			}
			if (!_clients[clientFd].isOperator) {
				sendMessage(clientFd, std::string(RED) + "Error: Permission denied.\n" + std::string(RESET));
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
					break;
				}
			}
			
			sendMessage(clientFd, std::string(GREEN) + "User " + nick + " has been kicked from the channel " + channel + ".\n" + std::string(RESET));
			return;
		}
	}
	sendMessage(clientFd, std::string(RED) + "Error: The channel doesn't exist.\n" + std::string(RESET));
}

