#ifndef CHANEL_HPP
# define CHANEL_HPP

# include <iostream>
# include <vector>

struct Channel {
	std::string					channelName;
	std::string					channelPass;
	std::vector<std::string>	members;
	std::vector<std::string>	invited;
	std::string					topic;;
	int							limit;
	bool						havePass;
	bool						haveLimit;
	bool						isTopic;
	bool						isInviteOnly;

	bool	isUserInvited(const std::string& nickname) {
		for (size_t i = 0; i < invited.size(); i++) {
			if (invited[i] == nickname) {
				return true;
			}
		}
		return false;
	}
};

#endif