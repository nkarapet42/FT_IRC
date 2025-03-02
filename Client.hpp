#ifndef CLIENT_HPP
#define CLIENT_HPP

# include "Info.hpp"
# include <string>
# include <set>

class Client {
public:
	int								fd;
	std::string						nickname;
	std::string						username;
	std::set<Info>	channels;
	std::string						curchannel;
	bool							isAuthenticated;
	bool							isOperator;

	Client();
	Client(int fd);
	Client(const Client& other);
	Client& operator=(const Client& other);
	~Client();
	
	void	setNickname(const std::string& nick);
	void	setUsername(const std::string& user);
	// void	joinChannel(const std::string& channel);
	// void	leaveChannel(const std::string& channel);
};

#endif
