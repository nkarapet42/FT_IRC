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
		sendMessage(clientFd, std::string(GREEN) + "Authentication successful!\n" + std::string(RESET));
		sendMessage(clientFd, "You can now send commands.\n");
	} else {
		sendMessage(clientFd, std::string(RED) + "Incorrect password, try again.\n" + std::string(RESET));
	}
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
	
	
	sendMessage(clientFd, std::string(PURPLE) + "Please authenticate by sending the password: PASS <password>\n" + std::string(RESET));
	std::cout << "New client connected: FD " << clientFd << "\n";
}

void	Server::sendMessage(int clientFd, const std::string& message) {
	send(clientFd, message.c_str(), message.length(), 0);
}

void Server::broadcastMessage(const std::string& channelName, int senderFd, const std::string& message) {
	for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
		if (it->second.curchannel == channelName && it->first != senderFd) {
			sendMessage(it->first, std::string(CYAN) + "[ " + _clients[senderFd].nickname + " ]: " + message + "\n" + std::string(RESET));
		}
	}
}

void	Server::handleClientMessage(int clientFd) {
	char buffer[512];
	std::memset(buffer, 0, sizeof(buffer));
	int bytesRead = recv(clientFd, buffer, sizeof(buffer) - 1, 0);

	if (bytesRead <= 0) {
		if (bytesRead == 0) {
			std::cout << "Client disconnected: FD " << clientFd << "\n";
		} else {
			std::cerr << "Error: recv failed for FD " << clientFd << "\n";
		}
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
		std::stringstream ss(message);
		std::string cmd, pass_word;
		ss >> cmd >> pass_word;

		std::string restOfLine;
		std::getline(ss, restOfLine);
		if (pass_word.empty() || !restOfLine.empty()) {
			sendMessage(clientFd, std::string(RED) + "Error: PASS <password>.\n" + std::string(RESET));
		} else if (cmd == "PASS") {
			authenticateClient(clientFd, pass_word);
		} else {
			sendMessage(clientFd, std::string(RED) + "Error: PASS <password>.\n" + std::string(RESET));
		}
	}
}

void	Server::Message(int clientFd, const std::string& line) {
	std::stringstream ss(line);
	std::string message;
	std::getline(ss, message);
	if (!message.empty() && message[0] == ' ')
		message = message.substr(1);
	if (!message.empty())
		broadcastMessage(_clients[clientFd].curchannel, clientFd, message);
	else
		sendMessage(clientFd, std::string(RED) + "Error: No message provided.\n" + std::string(RESET));
}

void Server::botCommandsCall(int clientFd, const std::string& line) {
	std::stringstream ss(line);
    std::string cmd;
    ss >> cmd;

	if (!_clients[clientFd].curchannel.empty()) {
		if (cmd == "!HELP"){
			_bot.botHelp(clientFd, *this, line);
		} else if (cmd == "!MOTD") {
			_bot.sendMotd(clientFd, *this, line);
		} else if (cmd == "!TIME") {
			_bot.sendTime(clientFd, *this, line);	
		} else if (cmd == "!WEATHER") {
			_bot.sendWeather(clientFd, *this, line);
		} else {
			sendMessage(clientFd, std::string(RED) + "Unknown Bot command: " + std::string(RESET) + cmd + "\n");
		}
	} else {
		sendMessage(clientFd, std::string(RED) + "You are not in the channel,for using Bot!!!!\n" + std::string(RESET));
		sendMessage(clientFd, std::string(PURPLE) + "Suggest : Use 'JOIN <channel> [password]'\n" + std::string(RESET));
	}
}

void Server::handleClientCommands(int clientFd, const std::string& line) {
	std::stringstream ss(line);
	std::string cmd, channelName, password, nick;
	ss >> cmd >> channelName;

	if (cmd != "USER" && _clients[clientFd].username.empty()) {
		sendMessage(clientFd, std::string(RED) + "Error: Please set a username before starting conversation.\n" + std::string(RESET));
		sendMessage(clientFd, std::string(YELLOW) + "Info: USER <username>.\n" + std::string(RESET));
		return ;
	} 
	if (cmd == "USER") {
		setUsername(clientFd, channelName);
	} else if (cmd == "JOIN") {
		ss >> password;
		join(clientFd, channelName, password);
	} else if (cmd == "NICK") {
		changeNickname(clientFd, channelName);
	} else if (cmd == "QUIT") {
		quitClient(clientFd, line);
	} else if (cmd == "PASS") {
		ss >> password;
		_clients[clientFd].changeChannelPassword(channelName, password);
	}  else if (cmd == "PART") {
		partChannel(clientFd, line);
	} else if (cmd == "PRIVMSG" || cmd == "NOTICE") {
		privateNoticeMessage(clientFd, line);
	} else if (cmd == "WHO") {
		whoCommand(clientFd, line);
	} else if (cmd == "TOPIC") {
		topicCommand(clientFd, line);
	} else if (cmd == "KICK") {
		ss >> nick;
		kick(clientFd, channelName, nick);
	} else if (cmd == "DCC") {
		if (channelName == "SEND") {
			dccSend(clientFd, line);
		} else if (channelName == "GET") {
			dccGet(clientFd, line);
		} else {
			sendMessage(clientFd, std::string(RED) + "Usage: DCC SEND or DCC GET\n" + std::string(RESET));
		}
	} else if (cmd[0] == '!') {
		botCommandsCall(clientFd, line);
	} else if (!_clients[clientFd].curchannel.empty()) {
		Message(clientFd, line);
	} else if (cmd == "INVITE") {
		ss >> nick;
		invite(clientFd, channelName, nick);
	} else if (cmd == "MODE") {
		std::string mode, param;
		ss >> mode >> param;
		modeCommand(clientFd, channelName, mode, param);
	} else {
		sendMessage(clientFd, std::string(RED) + "Unknown command: " + std::string(RESET) + cmd + "\n");
		sendMessage(clientFd, std::string(PURPLE) + "Use !HELP to know more about commands" + std::string(RESET) + "\n");
	}
}

