#ifndef SERVER_HPP
# define SERVER_HPP

# include <sys/socket.h>
# include <netinet/in.h>
# include <arpa/inet.h>
# include <unistd.h>
# include <iostream>
# include <cstring>
# include <iomanip>
# include <cstdlib>
# include <fstream>
# include <sstream>
# include <string>
# include <vector>
# include <string>
# include <poll.h>
# include <ctime>
# include <set>
# include <map>
# include "Client.hpp"
# include "Channel.hpp"
# include "FileTransfer.hpp"

using std::cout;
using std::cin;
using std::endl;

# define GREEN "\e[1;32m"
# define RESET "\e[0m"
# define RED "\e[1;91m"
# define CYAN "\e[1;36m"
# define YELLOW "\e[33m"
# define PURPLE "\e[1;35m"
# define BLUE "\e[1;34m"

extern std::vector<Channel> channelsIRC;

class Server {
private:
	int						_serverSocket;
	std::vector<pollfd>		_pollFds;
	std::map<int, Client>	_clients;
	std::string				_password;

	//Filetransfer
	std::map<std::string, FileTransfer> activeTransfers;

	//Constructors
	Server(const Server& other);
	Server& operator=(const Server& other);

	//Commands
	void	authenticateClient(int clientFd, const std::string& password);
	void	sendMessage(int clientFd, const std::string& message);
	void	broadcastMessage(const std::string& channelName, int senderFd, const std::string& message);
	void	handleClientCommands(int clientFd, const std::string& command);
	void	changeNickname(int clientFd, const std::string& newNick);
	void	setUsername(int clientFd, const std::string& user);
	void	Message(int clientFd, const std::string& line);
	void	privateMessage(int clientFd, const std::string& line);
	void	kick(int clientFd, const std::string& line, const std::string& nick);
	void	quitClient(int clientFd, const std::string& line);
	void	whoCommand(int clientFd, const std::string& line);
	void	topicCommand(int clientFd, const std::string& line);
	void	partChannel(int clientFd, const std::string& line);

	//BOT
	void	botHelp(int clientFd, const std::string& line);
	void	sendMotd(int clientFd, const std::string& line);
	void	sendTime(int clientFd, const std::string& line);
	void	sendWeather(int clientFd, const std::string& line);

	//FILE_TRANSFER
	void	dccSend(int senderFd, const std::string& line);
	void	dccGet(int receiverFd, const std::string& line);
	void	connectToSender(int receiverFd, const FileTransfer& transfer);
	void	startFileTransfer(int receiverFd, const std::string& filename, size_t fileSize);

public:
	Server(int port, const std::string& password);
	~Server();

	//ServerSideCommands
	void		run();
	void		join(int clientFd, const std::string& channelNAme, const std::string& password);
	void		acceptNewClient();
	void		handleClientMessage(int clientFd);

};

#endif
