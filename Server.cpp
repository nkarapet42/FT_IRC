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


	sendMessage(clientFd, std::string(PURPLE) + "Please authenticate by sending the password: PASS <password>\n" + std::string(RESET));
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
		if (!_clients[clientFd].isOperator) {
			sendMessage(clientFd, std::string(RED) + "Error: Permission denied.\n" + std::string(RESET));
			return;
		}
		for (std::vector<std::string>::iterator memberIt = it->members.begin(); memberIt != it->members.end(); ++memberIt) {
			if (*memberIt == nick) {
				it->members.erase(memberIt);
				sendMessage(clientFd, std::string(GREEN) + "User " + nick + " has been kicked from the channel " + channel + ".\n" + std::string(RESET));
				return;
			}
		}
		break;
	}
}
sendMessage(clientFd, std::string(RED) + "Error: The channel doesn't exist.\n" + std::string(RESET));
}


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

    close(clientFd);
    _clients.erase(it);
}

void Server::whoCommand(int clientFd, const std::string& line) {
	std::stringstream ss(line);
	std::string cmd, channelName;
	ss >> cmd >> channelName;

	std::string restOfLine;
	std::getline(ss, restOfLine);
	if (!restOfLine.empty()) {
		sendMessage(clientFd, std::string(RED) + "Error: Wrong Syntax.\n" + std::string(RESET));
		sendMessage(clientFd, std::string(RED) + "Usage: WHO [channel].\n" + std::string(RESET));
		return;
	}
    if (channelName.empty()) {
        for (std::vector<Channel>::iterator it = channelsIRC.begin(); it != channelsIRC.end(); ++it) {
            Channel& channel = *it;
            std::string response = "WHO " + channel.channelName + " :";
            for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
                Client& client = it->second;
                for (std::vector<Info>::iterator infoIt = client.channels.begin(); infoIt != client.channels.end(); ++infoIt) {
                    if (infoIt->channelName == channel.channelName) {
                        response += client.nickname + " ";
                        response += (client.isOperator ? "(operator) " : "(normal) ");
                        break;
                    }
                }
            }

            sendMessage(clientFd, std::string(BLUE) + response + "\n" + std::string(RESET));
        }
    } else {
        Channel* channel = NULL;
        for (std::vector<Channel>::iterator it = channelsIRC.begin(); it != channelsIRC.end(); ++it) {
            if (it->channelName == channelName) {
                channel = &(*it);
                break;
            }
        }
        if (!channel) {
            sendMessage(clientFd, std::string(RED) + "ERROR: Channel not found.\n" + std::string(RESET));
            return;
        }
        std::string response = "WHO " + channelName + " :";
        for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
            Client& client = it->second;
            for (std::vector<Info>::iterator infoIt = client.channels.begin(); infoIt != client.channels.end(); ++infoIt) {
                if (infoIt->channelName == channelName) {
                    response += client.nickname + " ";
                    response += (client.isOperator ? "(operator) " : "(normal) ");
                    break;
                }
            }
        }
        sendMessage(clientFd, std::string(BLUE) + response + "\n" + std::string(RESET));
    }
}

void Server::topicCommand(int clientFd, const std::string& line) {
	std::stringstream ss(line);
	std::string cmd, channelName;
	ss >> cmd >> channelName;

	if (channelName.empty()) {
		sendMessage(clientFd, std::string(RED) + "ERROR: No channel specified.\n" + std::string(RESET));
		sendMessage(clientFd, std::string(RED) + "Usage: TOPIC <channel> [topic].\n" + std::string(RESET));
		return;
	}
	std::string topic;
	std::getline(ss, topic);

    Channel* channel = NULL;
    for (std::vector<Channel>::iterator it = channelsIRC.begin(); it != channelsIRC.end(); ++it) {
        if (it->channelName == channelName) {
            channel = &(*it);
            break;
        }
    }

    if (!channel) {
        sendMessage(clientFd, std::string(RED) + "ERROR: Channel not found.\n" + std::string(RESET));
        return;
    }

    if (topic.empty()) {
        std::string response = "TOPIC " + channelName + " :" + (channel->topic.empty() ? "No topic set." : channel->topic);
        sendMessage(clientFd, std::string(CYAN) + response + "\n" + std::string(RESET));
    } else {
        Client& client = _clients[clientFd];
        bool isOperator = false;
        for (std::vector<Info>::iterator it = client.channels.begin(); it != client.channels.end(); ++it) {
            if (it->channelName == channelName && it->isOperator) {
                isOperator = true;
                break;
            }
        }

        if (!channel->isTopic || isOperator) {
            channel->topic = topic;
            std::string response = "TOPIC " + channelName + " :" + topic + "\n";
            sendMessage(clientFd, std::string(CYAN) + response + std::string(RESET));
            broadcastMessage(channelName, clientFd, std::string(CYAN) + response + std::string(RESET));
        } else {
            sendMessage(clientFd, std::string(RED) + "ERROR: You are not an operator of this channel.\n" + std::string(RESET));
        }
    }
}

void Server::partChannel(int clientFd, const std::string& line) {
	std::stringstream ss(line);
	std::string cmd, channelName;
	ss >> cmd >> channelName;

	if (channelName.empty()) {
		sendMessage(clientFd, std::string(RED) + "ERROR: Wrong Syntax .\n" + std::string(RESET));
		sendMessage(clientFd, std::string(RED) + "Usage: PART <channel> [message].\n" + std::string(RESET));
        return;
	}
	std::string message;
	std::getline(ss, message);
	if (message.empty())
		message += "GoodBye my Friend's";
	Channel* channel = NULL;
    for (std::vector<Channel>::iterator it = channelsIRC.begin(); it != channelsIRC.end(); ++it) {
        if (it->channelName == channelName) {
            channel = &(*it);
            break;
        }
    }
    if (!channel) {
        sendMessage(clientFd, std::string(RED) + "ERROR: Channel not found.\n" + std::string(RESET));
        return;
    }
	if (_clients[clientFd].leaveChannel(channelName)){
		sendMessage(clientFd, std::string(GREEN) + "You left the channel.\n" + std::string(RESET));
	} else {
		sendMessage(clientFd, std::string(RED) + "You are not in channel.\n" + std::string(RESET));
	}
	broadcastMessage(channelName, clientFd, std::string(CYAN) + "Last words : " + message + "\n" + std::string(RESET));
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
	helpMessage += "JOIN <channel> [password] - Join a channel\n";
	helpMessage += "KICK <channel> <nickname> - Kick a user from a channel\n";
	helpMessage += "LEAVE <channel> - Leave a channel\n";
	helpMessage += "NICK <nickname> - Set your nickname\n";
	helpMessage += "PRIVMSG <nickname> <message> - Send private message\n";
	helpMessage += "USER <username> - Set your username\n";
	helpMessage += "Bot Commands Information:\n";
	helpMessage += "HELP - Show this help message\n";
	helpMessage += "MOTD - Get the message of the day \n";
	helpMessage += "TIME - Get the current server time\n";
	helpMessage += "WEATHER - Get a random weather forecast\n";

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
 * 													   *
 * 			    F I L E    T R A N S F E R			   *
 *                                                     *
 ******************************************************/

 void Server::startFileTransfer(int receiverFd, const std::string& filename, size_t fileSize) {
    std::ifstream file(filename.c_str(), std::ios::binary);
    if (!file.is_open()) {
        sendMessage(receiverFd, std::string(RED) + "Error: Unable to open file for transfer.\n" + std::string(RESET));
        return;
    }

    const size_t bufferSize = 1024;
    char buffer[bufferSize];

    size_t bytesSent = 0;
    while (bytesSent < fileSize) {
        size_t bytesToRead = std::min(bufferSize, fileSize - bytesSent);
        file.read(buffer, bytesToRead);

        if (file.gcount() > 0) {
            ssize_t bytesWritten = send(receiverFd, buffer, file.gcount(), 0);
            if (bytesWritten == -1) {
                sendMessage(receiverFd, std::string(RED) + "Error: File transfer failed.\n" + std::string(RESET));
                break;
            }
            bytesSent += bytesWritten;
        }
    }

    file.close();
    sendMessage(receiverFd, std::string(YELLOW) + "Capacity of File\n" + std::string(RESET));
    sendMessage(receiverFd, std::string(GREEN) + "File transfer completed.\n" + std::string(RESET));
}


 void Server::dccSend(int senderFd, const std::string& line) {
	std::stringstream ss(line);
    std::string cmd, type, receiverNick, filename;
    ss >> cmd >> type >> receiverNick >> filename;
	
	if (type.empty() || type != "SEND") {
		sendMessage(senderFd, std::string(RED) + "Usage: DCC SEND <nickname> <filename>\n" + std::string(RESET));
		return;
	}
    if (receiverNick.empty() || filename.empty()) {
        sendMessage(senderFd, std::string(RED) + "Usage: DCC SEND <nickname> <filename>\n" + std::string(RESET));
        return;
    }
	if (_clients[senderFd].nickname == receiverNick) {
		sendMessage(senderFd, std::string(RED) + "Error: Cant send file to yourself.\n" + std::string(RESET));
		return;
	}

    int receiverFd = -1;
    for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
        if (it->second.nickname == receiverNick) {
            receiverFd = it->first;
            break;
        }
    }

    if (receiverFd == -1) {
        sendMessage(senderFd, std::string(RED) + "Error: User " + receiverNick + " not found.\n" + std::string(RESET));
        return;
    }

	if (activeTransfers.find(filename) != activeTransfers.end()
		&& activeTransfers.find(filename)->second.senderFd == senderFd
		&& activeTransfers.find(filename)->second.receiverFd == receiverFd) {
        sendMessage(senderFd, std::string(RED) + "Error: File transfer for '" + filename + "' is already in progress.\n" + std::string(RESET));
        return;
    }

    std::ifstream file(filename.c_str(), std::ios::binary | std::ios::ate);
    if (!file) {
        sendMessage(senderFd, std::string(RED) + "Error: File not found.\n" + std::string(RESET));
        return;
    }
    int fileSize = file.tellg();
    file.close();

    std::stringstream sizeStream;
    sizeStream << fileSize;
    std::string fileSizeStr = sizeStream.str();
	
	activeTransfers[filename] = FileTransfer(senderFd, receiverFd, filename, fileSize);

    std::string dccMessage = "DCC SEND " + filename + " " + _clients[senderFd].nickname + " " + fileSizeStr + "\n"
							+ "USE: DCC GET <filename> for accepting";
    sendMessage(receiverFd, std::string(GREEN) + dccMessage + "\n" + std::string(RESET));

    startFileTransfer(receiverFd, filename, fileSize);
}

static std::string generateUniqueFileName(const std::string& filename) {
    std::time_t now = std::time(0);
    std::tm* 	timeInfo = std::localtime(&now);

    char timeBuffer[31];
    std::sprintf(timeBuffer, "%04d%02d%02d%02d%02d%02d", 
                 1900 + timeInfo->tm_year, timeInfo->tm_mon + 1, timeInfo->tm_mday,
                 timeInfo->tm_hour, timeInfo->tm_min, timeInfo->tm_sec);

    std::stringstream ss;
    ss << "dcc_" << timeBuffer << "_" << filename;
    return ss.str();
}

void Server::connectToSender(int receiverFd, const FileTransfer& transfer) {
    int senderFd = transfer.senderFd;
    const std::string& filename = transfer.fileName;
    size_t fileSize = transfer.fileSize;

    if (_clients.find(senderFd) == _clients.end()) {
        sendMessage(receiverFd, std::string(RED) + "Error: Sender is no longer available.\n" + std::string(RESET));
        return;
    }

    std::string dccFilePath = generateUniqueFileName(filename);

    std::ofstream dccFile(dccFilePath.c_str(), std::ios::binary);
    if (!dccFile.is_open()) {
        sendMessage(receiverFd, std::string(RED) + "Error: Unable to create file for transfer.\n" + std::string(RESET));
        return;
    }

    std::ifstream file(filename.c_str(), std::ios::binary);
    if (!file.is_open()) {
        sendMessage(receiverFd, std::string(RED) + "Error: Unable to open file for transfer.\n" + std::string(RESET));
        return;
    }

    char buffer[1024];
    while (file.read(buffer, sizeof(buffer)) || file.gcount() > 0) {
        dccFile.write(buffer, file.gcount());
    }
    file.close();
    dccFile.close();

    std::stringstream info;
    info << "DCC GET " << filename << " " << fileSize << " " << dccFilePath;
    sendMessage(receiverFd, std::string(YELLOW) + info.str() + "\n" + std::string(RESET));

    sendMessage(receiverFd, std::string(GREEN) + "File transfer completed." + std::string(RESET));
}


void Server::dccGet(int receiverFd, const std::string& line) {
    std::stringstream ss(line);
    std::string cmd, type ,filename;
    ss >> cmd >> type >> filename;

	if (type.empty() || type != "GET") {
		sendMessage(receiverFd, std::string(RED) + "Usage: DCC GET <filename>\n" + std::string(RESET));
		return;
	}
    if (filename.empty()) {
        sendMessage(receiverFd, std::string(RED) + "Usage: DCC GET <filename>\n" + std::string(RESET));
        return;
    }

	std::map<std::string, FileTransfer>::iterator it = activeTransfers.find(filename);
    if (it == activeTransfers.end()) {
        sendMessage(receiverFd, std::string(RED) + "Error: No active DCC SEND for this file.\n" + std::string(RESET));
        return;
    }

	FileTransfer& transfer = it->second;

    if (transfer.receiverFd != receiverFd) {
        sendMessage(receiverFd, std::string(RED) + "Error: You are not the intended recipient of this file.\n" + std::string(RESET));
        return;
    }

    sendMessage(transfer.senderFd, std::string(YELLOW) + "User " + _clients[receiverFd].nickname + " has accepted your file: " + filename + "\n" + std::string(RESET));
    sendMessage(receiverFd, std::string(YELLOW) + "You accepted the file: " + filename + " from " + _clients[transfer.senderFd].nickname + ". Waiting for transfer...\n" + std::string(RESET));

    connectToSender(receiverFd, transfer);
    activeTransfers.erase(it);
}


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
	} else if (cmd == "QUIT") {
		quitClient(clientFd, line);
	} else if (cmd == "PASS") {
		ss >> password;
		_clients[clientFd].changeChannelPassword(channelName, password);
	}  else if (cmd == "PART") {
		partChannel(clientFd, line);
	} else if (cmd == "PRIVMSG") {
		privateMessage(clientFd, line);
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
