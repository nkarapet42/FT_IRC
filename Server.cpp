#include "Server.hpp"

Server::Server(int port, const std::string& password) {
	this->_password = password;
	_serverSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (_serverSocket < 0) {
		std::cerr << "Error: Cannot create socket\n";
		std::exit(EXIT_FAILURE);
	}

	sockaddr_in serverAddr;
	std::memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = INADDR_ANY;
	serverAddr.sin_port = htons(port);

	if (bind(_serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
		std::cerr << "Error: Cannot bind socket\n";
		std::exit(EXIT_FAILURE);
	}

	if (listen(_serverSocket, 10) < 0) {
		std::cerr << "Error: Cannot listen on socket\n";
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
			std::cerr << "Error: poll() failed\n";
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
		;
		// handleClientCommands(clientFd, message);
		/*
			Nayi inch es anum Cliet mej inch vor commentaca et es grum, channels eti nranqa te vor chatum Client ka
			Info-vectora dra mej pahvuma anun@ channeli, password datark dir demic nuyne operator@ dir false demic, heto global mej main-uma std::vector<Channel> channelsIRC; 
			Nayum es ete et nuyn anunov channel ka ete goyutyun uni prosto Join es anum et chat u parol es uzum ete parol ka ete chka miangamic kpcnum es parol@ datark toxum
			Ete goyutyun chuni tenc anunov chat stexcum es avelacnum es globali mej demic channelPass = "" u havepass = false kdnes,u et kpnoxin et chati hamar miangamic operator ksarqes
			Ete et client pasword dni ira channeli vra kpnox@ pti et parol@ gri,u vorves user kpni,ete parol@ poxvi userin dursa qcelu,vor noric parol@ gri,bayc ete et userin operator en tve u urish mek@ parol@ poxuma inq@ pti chatum mna u parol@ update lini ira mot
		*/
	} else {
		authenticateClient(clientFd, message);
	}
}

