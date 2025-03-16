#pragma once

# include <sys/socket.h>
# include <netinet/in.h>
# include <arpa/inet.h>
# include <algorithm>
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
# include "Bot.hpp"

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

# define BUFFER_SIZE 512

extern std::vector<Channel> channelsIRC;

class Server {
private:
	int						_serverSocket;
	std::vector<pollfd>		_pollFds;
	std::map<int, Client>	_clients;
	std::string				_password;
	Bot						_bot;

	/***************Server_Transfer****************/
	std::map<std::string, FileTransfer> activeTransfers;

	void	dccSend(int senderFd, const std::string& line);
	void	dccGet(int receiverFd, const std::string& line);
	void	connectToSender(int receiverFd, const FileTransfer& transfer);
	void	startFileTransfer(int receiverFd, const std::string& filename, size_t fileSize);
	
	/****************Constructors*****************/
	Server(const Server& other);
	Server& operator=(const Server& other);

	/********************BOT**********************/
	void	botCommandsCall(int clientFd, const std::string& line);

	/******************ServerSideCommands*************/
	void	authenticateClient(int clientFd, const std::string& password);
	void	broadcastMessage(const std::string& channelName, int senderFd, const std::string& message, const std::string& cmd, int errorNumber = 0);
	bool	isChannelName(const std::string& channelName);
	void	handleClientCommands(int clientFd, const std::string& command);
	void	Message(int clientFd, const std::string& line);

	/****************Server_Commands_Part1*****************/
	void	quitClient(int clientFd, const std::string& line);
	void	setUsername(int clientFd, const std::string& user);
	void	changeNickname(int clientFd, const std::string& newNick);
	void	privateNoticeMessage(int clientFd, const std::string& line);
	void	kick(int clientFd, const std::string& line, const std::string& nick);
	
	/****************Server_Commands_Part2*****************/
	void	whoCommand(int clientFd, const std::string& line);
	void	partChannel(int clientFd, const std::string& line);
	void	topicCommand(int clientFd, const std::string& line);
	
	/****************Server_Commands_Part3*****************/
	void	invite(int clientFd, const std::string& channel, const std::string& nick);
	void	join(int clientFd, const std::string& channelNAme, const std::string& password);
	void	modeCommand(int clientFd, const std::string& channel, const std::string& mode, const std::string& param);
	
	public:
	
	/****************Constructors*****************/
	Server(int port, const std::string& password);
	~Server();
	
	/******************ServerSideCommands*************/
	void		run();
	void		acceptNewClient();
	void		handleClientMessage(int clientFd);
	void		sendMessage(int clientFd, const std::string& message, const std::string& cmd, int errorNumber = 0);
	void		sendErrorMessage(int clientFd, const std::string& message, int errorNumber);
	
};
