#pragma once

# include <iostream>
# include <sstream>
# include <vector>
# include <ctime>
# include <cstdlib>

class Server;

class Bot {
public:
	Bot();
	~Bot();
	
	void	botHelp(int clientFd, Server& server, const std::string& line);
	void	sendMotd(int clientFd, Server& server, const std::string& line);
	void	sendTime(int clientFd, Server& server, const std::string& line);
	void	sendWeather(int clientFd, Server& server, const std::string& line);
};