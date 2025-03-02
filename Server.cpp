#include "Server.hpp"

Server::Server(int port, const std::string& password) {
	this->_password = password;
	_serverSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (_serverSocket < 0) {
		std::cerr << RED << "Error: Cannot create socket\n" << RESET;
		std::exit(EXIT_FAILURE);
	}

	sockaddr_in serverAddr;
	std::memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = INADDR_ANY;
	serverAddr.sin_port = htons(port);

	if (bind(_serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
		std::cerr << RED << "Error: Cannot bind socket\n" << RESET;
		std::exit(EXIT_FAILURE);
	}

	if (listen(_serverSocket, 10) < 0) {
		std::cerr << RED << "Error: Cannot listen on socket\n" << RESET;
		std::exit(EXIT_FAILURE);
	}

	pollfd serverPollFd;
	serverPollFd.fd = _serverSocket;
	serverPollFd.events = POLLIN;
	_pollFds.push_back(serverPollFd);

	std::cout << "Server started on port " << port << "\n";
}

Server::~Server() {
	for (size_t i = 0; i < _pollFds.size(); i++) {
		close(_pollFds[i].fd);
	}
	std::cout << "Server shut down.\n";
}

void	Server::run() {
	while (true) {
		if (poll(_pollFds.data(), _pollFds.size(), -1) < 0) {
			std::cerr << RED << "Error: poll() failed\n" << RED;
			break;
		}

		for (size_t i = 0; i < _pollFds.size(); i++) {
			if (_pollFds[i].revents & POLLIN) {
				if (_pollFds[i].fd == _serverSocket)
					acceptNewClient();
				else
					handleClientMessage(_pollFds[i].fd);
			}
		}
	}
}

void	Server::authenticateClient(int clientFd, const std::string& password) {
	if (password == _password) {
		_clients[clientFd].isAuthenticated = true;
		sendMessage(clientFd, "Authentication successful!\n");
		sendMessage(clientFd, "You can now send commands.\n");
	} else {
		sendMessage(clientFd, "Incorrect password, try again: ");
	}
}

void	Server::sendMessage(int clientFd, const std::string& message) {
	send(clientFd, message.c_str(), message.length(), 0);
}

void	Server::acceptNewClient() {
	int clientFd = accept(_serverSocket, NULL, NULL);
	if (clientFd < 0) {
		std::cerr << "Error: Cannot accept client\n";
		return;
	}

	pollfd clientPollFd;
	clientPollFd.fd = clientFd;
	clientPollFd.events = POLLIN;
	_pollFds.push_back(clientPollFd);
	_clients[clientFd] = Client(clientFd);


	sendMessage(clientFd, "Please authenticate by sending the password: ");
	std::cout << "New client connected: FD " << clientFd << "\n";
}

void	Server::handleClientMessage(int clientFd) {
	char buffer[512];
	std::memset(buffer, 0, sizeof(buffer));
	int bytesRead = recv(clientFd, buffer, sizeof(buffer) - 1, 0);

	if (bytesRead <= 0) {
		std::cout << "Client disconnected: FD " << clientFd << "\n";
		close(clientFd);
		for (std::vector<pollfd>::iterator it = _pollFds.begin(); it != _pollFds.end(); ) {
            if (it->fd == clientFd) {
                it = _pollFds.erase(it);
            } else {
                ++it;
            }
        }
		return;
	}

	std::string	message(buffer);
	if (!message.empty() && message[message.length() - 1] == '\n') {
		message.erase(message.length() - 1);
	}
	std::cout << "Received from FD " << clientFd << ": " << buffer;
	if (_clients[clientFd].isAuthenticated){
		handleClientCommands(clientFd, message);
	} else {
		authenticateClient(clientFd, message);
	}
}

void Server::handleClientCommands(int clientFd, const std::string& line) {
    std::stringstream ss(line);
    std::string cmd, channelName, password, message;
    ss >> cmd >> channelName;

    if (cmd == "JOIN") {
        ss >> password;
		join(clientFd, channelName, password);
    }
	else if (cmd == "NICK") {
		changeNickname(clientFd, channelName);
    }
    else if (cmd == "CHANGEPASS") {
		ss >> password;
        _clients[clientFd].changeChannelPassword(channelName, password);
    } 
    else if (cmd == "LEAVE") {
		_clients[clientFd].leaveChannel(channelName);
    }
    else if (cmd == "MSG") {
		std::getline(ss, message);
        if (!message.empty() && message[0] == ' ')
		message = message.substr(1);
		
        if (!message.empty())
            broadcastMessage(channelName, clientFd, message);
			else
            sendMessage(clientFd, std::string(RED) + "Error: No message provided.\n" + std::string(RESET));
		} 
		else {
			sendMessage(clientFd, "Unknown command: " + cmd + "\n");
		}
	}
	
	
void	Server::join(int clientFd, const std::string& channelName, const std::string& password) {
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
			sendMessage(clientFd, _clients[clientFd].getNickname() + " joined channel: " + channelName + "\n");
            return ;
		}
	}
    sendMessage(clientFd, _clients[clientFd].getNickname() + " created and joined channel: " + channelName + "\n");
	_clients[clientFd].joinChannel(channelName, password);
}

void Server::broadcastMessage(const std::string& channelName, int senderFd, const std::string& message) {
    for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
        if (it->second.curchannel == channelName && it->first != senderFd) {
            sendMessage(it->first, "[ " + _clients[senderFd].nickname + " ]: " + message + "\n");
        }
    }
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

    sendMessage(clientFd, "Your nickname has been changed to " + newNick + "\n");

    for (size_t i = 0; i < _clients[clientFd].channels.size(); i++) {
        std::string channel = _clients[clientFd].channels[i].channelName;
        broadcastMessage(channel , clientFd, "User " + oldNick + " is now known as " + newNick);
    }
}
