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
		sendMessage(clientFd, std::string(GREEN) + "Authentication successful!\n" + std::string(RESET), " ", 001);
	} else {
		sendErrorMessage(clientFd, "Incorrect password, try again.\n", 464);
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
	
	sendErrorMessage(clientFd, "Please authenticate by sending the password: PASS <password>", 464);
	std::cout << "New client connected: FD " << clientFd << "\n";
}

std::string intToString(int value) {
    std::ostringstream oss;
    oss << value;
    return oss.str();
}

void	Server::sendErrorMessage(int clientFd, const std::string& message, int errorNumber) {
    if (_clients.find(clientFd) == _clients.end()) {
        std::cerr << "Error: Client FD " << clientFd << " not found.\n";
        return;
    }

    std::string errorMessage = std::string(RED) + ":FT_IRC " 
        + intToString(errorNumber) + " " 
        + _clients[clientFd].getNickname() + " " 
        + (!_clients[clientFd].curchannel.empty() ? _clients[clientFd].curchannel + " " : "") 
        + ":" + message + "\r\n" + std::string(RESET);

    send(clientFd, errorMessage.c_str(), errorMessage.length(), 0);
}


void	Server::sendMessage(int clientFd, const std::string& message, const std::string& cmd, int errorNumber) {
	std::string newMessage = ":FT_IRC :" + _clients[clientFd].getNickname() + " " + cmd + " " + intToString(errorNumber) + " :" + message;
	send(clientFd, newMessage.c_str(), newMessage.length(), 0);
}

void Server::broadcastMessage(const std::string& channelName, int senderFd, const std::string& message, const std::string& cmd, int errorNumber) {
	for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
		if (it->second.curchannel == channelName && it->first != senderFd) {
			sendMessage(it->first, std::string(CYAN) + "[ " + _clients[senderFd].nickname + " ]: " + message + "\n" + std::string(RESET), cmd, errorNumber);
		}
	}
}

void	Server::handleClientMessage(int clientFd) {
	std::string	message;
	char 		buffer[BUFFER_SIZE + 1];
	int 		bytesRead;
	
	do 
	{
		std::memset(buffer, 0, BUFFER_SIZE);
		bytesRead  = recv(clientFd, buffer, BUFFER_SIZE, 0);
		if (bytesRead <= 0) {
			if (bytesRead == 0) {
				std::cout << "Client disconnected: FD " << clientFd << "\n";
			} else {
				std::cerr << "Error: recv failed for FD " << clientFd << "\n";
			}
			for (std::map<std::string, FileTransfer>::iterator it = activeTransfers.begin(); it != activeTransfers.end(); ++it) {
				if (it->second.senderFd == clientFd) {
					activeTransfers.erase(it);
				} 
			}
			_clients.erase(clientFd);
			close(clientFd);
			for (std::vector<pollfd>::iterator it = _pollFds.begin(); it != _pollFds.end(); ) {
				if (it->fd == clientFd) {
					it = _pollFds.erase(it);
				} else {
					++it;
				}
			}
			return;
		} else {
			buffer[bytesRead] = 0;
			message.append(buffer);
		}
	} while (bytesRead == BUFFER_SIZE);
	

	if (!message.empty() && (message[message.length() - 1] >= 9
			&& message[message.length() - 1] <= 13)) {
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
		if (!restOfLine.empty() && (restOfLine[restOfLine.length() - 1] >= 9
			&& restOfLine[restOfLine.length() - 1] <= 13)) {
			restOfLine.erase(restOfLine.length() - 1);
		}
		if (pass_word.empty() || !restOfLine.empty()) {
			sendErrorMessage(clientFd, "Error: PASS <password>.\n", 464);\
		} else if (cmd == std::string("PASS")) {
			authenticateClient(clientFd, pass_word);
		} else {
			sendErrorMessage(clientFd, "Error: PASS <password>.\n", 464);
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
		broadcastMessage(_clients[clientFd].curchannel, clientFd, message, "PRIVMSG");
	else
		sendErrorMessage(clientFd, "Error: Message cannot be empty.", 464);
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
			sendErrorMessage(clientFd, std::string(RED) + "Unknown Bot command: " + std::string(RESET) + cmd + "\n", 421);
		}
	} else {
		sendErrorMessage(clientFd, "Error: You are not in the channel,for using Bot!!!!", 421);
		sendMessage(clientFd, std::string(PURPLE) + "Suggest : Use 'JOIN <channel> [password]'\n" + std::string(RESET), " ", 464);
	}
}

bool	Server::isChannelName(const std::string& channelName) {
	if (channelName.empty())
		return false;
	if (channelName[0] != '#')
		return false;
	return true;
}

void Server::handleClientCommands(int clientFd, const std::string& line) {
	std::stringstream ss(line);
	std::string cmd, channelName, password, nick;
	ss >> cmd >> channelName;

	if (cmd != "NICK" && _clients[clientFd].nickname.empty()) {
		sendErrorMessage(clientFd, "Error: Please set a nickname before starting conversation.", 401);
		sendErrorMessage(clientFd, std::string(YELLOW) + "Info: NICK <nickname>.\n" + std::string(RESET), 001);
		return ;
	}
	if (cmd == "NICK") {
		changeNickname(clientFd, channelName);
		return ;
	} else if (cmd == "USER") {
		setUsername(clientFd, channelName);
		return ;
	} else if (cmd == "JOIN") {
		ss >> password;
		join(clientFd, channelName, password);
	}  else if (cmd == "INVITE") {
		ss >> nick;
		invite(clientFd, channelName, nick);
	} else if (cmd == "MODE") {
		std::string mode, param;
		ss >> mode >> param;
		modeCommand(clientFd, channelName, mode, param);
	} else if (cmd == "QUIT") {
		quitClient(clientFd, line);
	} else if (cmd == "PASS") {
		ss >> password;
		_clients[clientFd].changeChannelPassword(channelName, password);
	}  else if (cmd == "PART") {
		partChannel(clientFd, line);
	} else if (cmd == "PRIVMSG") {
		privateNoticeMessage(clientFd, line);
		Message(clientFd, line);
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
			sendMessage(clientFd, std::string(RED) + "Usage: DCC SEND or DCC GET\n" + std::string(RESET), "DCC");
		}
	} else if (cmd[0] == '!') {
		botCommandsCall(clientFd, line);
	} else {
		sendErrorMessage(clientFd, "Error: Unkown command.", 421);
		sendMessage(clientFd, std::string(PURPLE) + "Use !HELP to know more about commands" + std::string(RESET) + "\n", " ");
	}
}

