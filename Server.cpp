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


	sendMessage(clientFd, std::string(PURPLE) + "Please authenticate by sending the password: " + std::string(RESET));
	std::cout << "New client connected: FD " << clientFd << "\n";
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
		authenticateClient(clientFd, message);
	}
}
void	Server::setUsername(int clientFd, const std::string& user) {
	if (user.empty()) {
		sendMessage(clientFd, std::string(RED) + "Error: Username cannot be empty.\n" + std::string(RESET));
		return;
	}
	_clients[clientFd].setUsername(user);
	sendMessage(clientFd, std::string(CYAN) + "Your username has been seted.\n" + std::string(RESET));
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

void	Server::privateMessage(int clientFd, const std::string& line) {
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
				sendMessage(it->second.fd, std::string(CYAN) + "PRIVMSG : [ " + _clients[clientFd].nickname +" -> " + client + " ]: " + message + "\n" + std::string(RESET));
				return;
			}
		}
		sendMessage(clientFd, std::string(RED) + "Error: Nickname not found.\n" + std::string(RESET));
	}
	else
		sendMessage(clientFd, std::string(RED) + "Error: No message provided.\n" + std::string(RESET));
}

void	Server::kick(int clientFd, const std::string& channel, const std::string& nick) {
	(void)nick;
	for (size_t i = 0; i < channelsIRC.size(); i++) {
		if (channelsIRC[i].channelName == channel) {
			break;
		}
	}
	for (size_t i = 0; i < _clients[clientFd].channels.size(); ++i) {
		if (_clients[clientFd].channels[i].channelName == channel) {
			_clients[clientFd].channels.erase(_clients[clientFd].channels.begin() + i);
			std::cout << _clients[clientFd].nickname << " Left channel: " << channel << std::endl;
			if (_clients[clientFd].curchannel == channel) {
				_clients[clientFd].curchannel = _clients[clientFd].channels.empty() ? "" : _clients[clientFd].channels.back().channelName;
			}
			return;
		}
	}
	if (!_clients[clientFd].isOperator) {
		sendMessage(clientFd, std::string(RED) + "Error: Permission denied.\n" + std::string(RESET));
		return ;
	}

}


/****************************************************************************************************/
/****************************************************************************************************/
/****************************************************************************************************/
/****************************************************************************************************/
/****************************************************************************************************/

/*******************************************************
 *                        B O T                        *
 *                                                     *
 *   ____   _____   _____    ____    _____    _____    *
 *  | __ ) | ____| | ____|  | __ )  | ____|  | ____|   *
 *  |  _ \ |  _|   |  _|    |  _ \  |  _|    |  _|     *
 *  | |_) || |___  | |___   | |_) | | |___   | |___    *
 *  |____/ |_____| |_____|  |____/  |_____|  |_____|   *
 *                                                     *
 ******************************************************/


void Server::botHelp(int clientFd, const std::string& line) {
	std::stringstream ss(line);
	std::string cmd;
	ss >> cmd;

	std::string restOfLine;
	std::getline(ss, restOfLine);
	if (!restOfLine.empty()) {
		sendMessage(clientFd, std::string(RED) + "Error: The HELP command must be used without additional arguments.\n" + std::string(RESET));
		return;
	}

	std::string helpMessage = "Inforamtion about Commands:\n";
	helpMessage += "Note: <> means required, [] means optional.\n";
	helpMessage += "HELP - Show this help message\n";
	helpMessage += "JOIN <channel> [password] - Join a channel\n";
	helpMessage += "KICK <channel> <nickname> - Kick a user from a channel\n";
	helpMessage += "LEAVE <channel> - Leave a channel\n";
	helpMessage += "MOTD - Get the message of the day \n";
	helpMessage += "NICK <nickname> - Set your nickname\n";
	helpMessage += "PRIVMSG <nickname> <message> - Send private message\n";
	helpMessage += "TIME - Get the current server time\n";
	helpMessage += "USER <username> - Set your username\n";

	sendMessage(clientFd, std::string(YELLOW) + helpMessage + std::string(RESET));
}

void Server::sendMotd(int clientFd, const std::string& line) {
	std::stringstream ss(line);
	std::string cmd;
	ss >> cmd;

	std::string restOfLine;
	std::getline(ss, restOfLine);
	if (!restOfLine.empty()) {
		sendMessage(clientFd, std::string(RED) + "Error: The MOTD command must be used without additional arguments.\n" + std::string(RESET));
		return;
	}
    time_t now = time(0);
    struct tm *timeInfo = localtime(&now);
    int hour = timeInfo->tm_hour;

    std::string greeting;
    if (hour < 12) {
        greeting = "Good morning! Have a great day ahead!";
    } else if (hour < 18) {
        greeting = "Good afternoon! Keep up the good work!";
    } else {
        greeting = "Good evening! Hope you’re having a good time!";
    }

    std::vector<std::string> quotes;
    quotes.push_back("The only way to do great work is to love what you do. - Steve Jobs");
    quotes.push_back("Success is not final, failure is not fatal: It is the courage to continue that counts. - Winston Churchill");
    quotes.push_back("Life is what happens when you're busy making other plans. - John Lennon");
    quotes.push_back("To be yourself in a world that is constantly trying to make you something else is the greatest accomplishment. - Ralph Waldo Emerson");

    srand(time(0));
    int randomIndex = rand() % quotes.size();

    std::string motd = greeting + "\n\n";
    motd += "Here's a random quote for you:\n";
    motd += "\"" + quotes[randomIndex] + "\"";

    sendMessage(clientFd, std::string(PURPLE) + motd + "\n" + std::string(RESET));
}

void Server::sendTime(int clientFd, const std::string& line) {
	std::stringstream ss(line);
	std::string cmd;
	ss >> cmd;

	std::string restOfLine;
	std::getline(ss, restOfLine);
	if (!restOfLine.empty()) {
		sendMessage(clientFd, std::string(RED) + "Error: The TIME command must be used without additional arguments.\n" + std::string(RESET));
		return;
	}

    time_t now = time(0);
    std::string timeStr = ctime(&now);
    sendMessage(clientFd, std::string(GREEN) + "Current time: " + timeStr + std::string(RESET));
}

static std::string getRandomWeather() {
    std::vector<std::string> weatherTypes;
	weatherTypes.push_back("Clear");
	weatherTypes.push_back("Clouds");
	weatherTypes.push_back("Mist");
	weatherTypes.push_back("Snow");
	weatherTypes.push_back("Rain");

	srand(time(0));
    int randomIndex = rand() % weatherTypes.size();
    return weatherTypes[randomIndex];
}

static std::vector<std::string> getWeatherArt(const std::string& weather) {
    std::vector<std::string> res(5, "");

    if (weather == "Clear") {
        res[0] = "     \\   /     ";
        res[1] = "      .-.      ";
        res[2] = "   ― (   ) ―   ";
        res[3] = "      `-’      ";
        res[4] = "     /   \\     ";
    }
    else if (weather == "Clouds") {
        res[0] = "               ";
        res[1] = "      .--.     ";
        res[2] = "   .-(    ).   ";
        res[3] = "  (  .  )  )   ";
        res[4] = "   ‾‾ ‾‾ ‾‾    ";
    }
    else if (weather == "Mist") {
        res[0] = "               ";
        res[1] = "    -   -   -  ";
        res[2] = "  ‾  -‾  -‾    ";
        res[3] = "   ‾-  ‾-  ‾-  ";
        res[4] = "  ‾   ‾   ‾    ";
    }
    else if (weather == "Snow") {
        res[0] = "      .-.      ";
        res[1] = "     (   ).    ";
        res[2] = "    (__(__)    ";
        res[3] = "    * * * *    ";
        res[4] = "   * * * *     ";
    }
    else if (weather == "Rain") {
        res[0] = "      .-.      ";
        res[1] = "     (   ).    ";
        res[2] = "    (___(__)   ";
        res[3] = "   ‚‘‚‘‚‘‚‘    ";
        res[4] = "   ‚’‚’‚’‚’    ";
    }
    else {
        res[0] = "               ";
        res[1] = "               ";
        res[2] = "               ";
        res[3] = "               ";
        res[4] = "               ";
    }

    return res;
}

void Server::sendWeather(int clientFd, const std::string& line) {
	std::stringstream ss(line);
	std::string cmd, location;
	ss >> cmd >> location;

	if (location.empty()) {
		sendMessage(clientFd, std::string(RED) + "Error: The WEATHER command must be used with location.\n" + std::string(RESET));
		return;
	}
	std::string restOfLine;
	std::getline(ss, restOfLine);
	if (!restOfLine.empty()) {
		sendMessage(clientFd, std::string(RED) + "Error: The WEATHER command must be used only with one argument.\n" + std::string(RESET));
		return;
	}

    std::string randomWeather = getRandomWeather();

    std::vector<std::string> weatherArt = getWeatherArt(randomWeather);

    std::string weatherInfo = "Weather Forecast at " + location + " : " + randomWeather + "\n";
    for (std::vector<std::string>::iterator it = weatherArt.begin(); it != weatherArt.end(); ++it) {
		weatherInfo += *it + "\n";
	}
    sendMessage(clientFd, weatherInfo);
}

/*******************************************************
 *                   B O T  E N D                      *
 *                                                     *
 *   ____   _____   _____    ____    _____    _____    *
 *  | __ ) | ____| | ____|  | __ )  | ____|  | ____|   *
 *  |  _ \ |  _|   |  _|    |  _ \  |  _|    |  _|     *
 *  | |_) || |___  | |___   | |_) | | |___   | |___    *
 *  |____/ |_____| |_____|  |____/  |_____|  |_____|   *
 *                                                     *
 ******************************************************/

/****************************************************************************************************/
/****************************************************************************************************/
/****************************************************************************************************/
/****************************************************************************************************/
/****************************************************************************************************/

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
	} else if (cmd == "PASS") {
		ss >> password;
		_clients[clientFd].changeChannelPassword(channelName, password);
	}  else if (cmd == "LEAVE") {
		_clients[clientFd].leaveChannel(channelName);
	} else if (cmd == "PRIVMSG") {
		privateMessage(clientFd, line);
	} else if (cmd == "KICK") {
		ss >> nick;
		kick(clientFd, channelName, nick);
	} else if (cmd == "HELP") {
		botHelp(clientFd, line);
	} else if (cmd == "MOTD") {
		sendMotd(clientFd, line);
	} else if (cmd == "TIME") {
		sendTime(clientFd, line);
	} else if (cmd == "WEATHER") {
		sendWeather(clientFd, line);
	} else if (!_clients[clientFd].curchannel.empty()) {
		Message(clientFd, line);
	} else {
		sendMessage(clientFd, std::string(RED) + "Unknown command: " + std::string(RESET) + cmd + "\n");
		sendMessage(clientFd, std::string(PURPLE) + "Use HELP to know more about commands" + std::string(RESET) + "\n");
	}
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

void Server::broadcastMessage(const std::string& channelName, int senderFd, const std::string& message) {
	for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
		if (it->second.curchannel == channelName && it->first != senderFd) {
			sendMessage(it->first, std::string(CYAN) + "[ " + _clients[senderFd].nickname + " ]: " + message + "\n" + std::string(RESET));
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

	sendMessage(clientFd,  std::string(CYAN) + "Your nickname has been changed to " + newNick + "\n" + std::string(RESET));

	for (size_t i = 0; i < _clients[clientFd].channels.size(); i++) {
		std::string channel = _clients[clientFd].channels[i].channelName;
		broadcastMessage(channel , clientFd, "User " + oldNick + " is now known as " + newNick);
	}
}
