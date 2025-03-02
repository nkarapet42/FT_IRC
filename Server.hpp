#ifndef SERVER_HPP
# define SERVER_HPP

# include <sys/socket.h>
# include <netinet/in.h>
# include <arpa/inet.h>
# include <unistd.h>
# include <iostream>
# include <cstring>
# include <cstdlib>
# include <string>
# include <vector>
# include <string>
# include <poll.h>
# include <set>
# include <map>
# include "Client.hpp"
# include "Channel.hpp"

class Server {
private:
	int						_serverSocket;
	std::vector<pollfd>		_pollFds;
	std::map<int, Client>	_clients;
	std::string				_password;

	Server(const Server& other);
	Server& operator=(const Server& other);

	void	authenticateClient(int clientFd, const std::string& password);
	void	sendMessage(int clientFd, const std::string& message);
	void	broadcastMessage(const std::string& message, int senderFd);
	void	handleClientCommands(int clientFd, const std::string& command);

public:
	Server(int port, const std::string& password);
	~Server();

	void		run();
	void		acceptNewClient();
	void		handleClientMessage(int clientFd);

};

#endif
