#ifndef CHANEL_HPP
# define CHANEL_HPP

# include <iostream>
# include <vector>

struct Channel {
	std::string					channelName;
	std::string					channelPass;
	std::vector<std::string>	members;
	bool						havePass;
};

#endif