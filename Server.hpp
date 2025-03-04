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
#include <sstream>
# include <poll.h>
# include <set>
# include <map>
# include "Client.hpp"
# include "Channel.hpp"

using std::cout;
using std::cin;
using std::endl;

# define GREEN "\e[1;32m"
# define RESET "\e[0m"
# define RED "\e[1;91m"
# define CYAN "\e[1;36m"
# define YELLOW "\e[1;33m"
# define PURPLE "\e[1;35m"
# define BLUE "\e[1;34m"

extern std::vector<Channel> channelsIRC;

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
	void	broadcastMessage(const std::string& channelName, int senderFd, const std::string& message);
	void	handleClientCommands(int clientFd, const std::string& command);
	void	changeNickname(int clientFd, const std::string& newNick);
	void	setUsername(int clientFd, const std::string& user);
	void	privateMessage(int clientFd, const std::string& line);

public:
	Server(int port, const std::string& password);
	~Server();

	void		run();
	void		join(int clientFd, const std::string& channelNAme, const std::string& password);
	void		acceptNewClient();
	void		handleClientMessage(int clientFd);

};

#endif
