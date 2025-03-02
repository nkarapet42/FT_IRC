#ifndef CLIENT_HPP
#define CLIENT_HPP

# include "Info.hpp"
# include <string>
# include <vector>
# include "Channel.hpp"

extern std::vector<Channel> channelsIRC;

class Client {
public:
	int					fd;
	std::string			nickname;
	std::string			username;
	std::vector<Info>	channels;
	std::string			curchannel;
	bool				isAuthenticated;
	bool				isOperator;

	Client();
	Client(int fd);
	Client(const Client& other);
	Client& operator=(const Client& other);
	~Client();
	
	const std::string& 	getNickname();
	void				setNickname(const std::string& nick);
	void				setUsername(const std::string& user);
	void   				joinChannel(const std::string& channelName, const std::string& password);
	void    			changeChannelPassword(const std::string& channelName, const std::string& newPassword);
	void				leaveChannel(const std::string& channel);
};

#endif
