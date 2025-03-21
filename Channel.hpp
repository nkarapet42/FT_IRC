#pragma once

# include <iostream>
# include <algorithm>
# include <vector>

struct Channel {
	std::string					channelName;
	std::string					channelPass;
	std::vector<std::string>	members;
	std::vector<std::string>	invited;
	std::string					topic;
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

	bool	isUserMember(const std::string& nickname) {
		for (size_t i = 0; i < members.size(); i++) {
			if (members[i] == nickname) {
				return true;
			}
		}
		return false;
	}
};

