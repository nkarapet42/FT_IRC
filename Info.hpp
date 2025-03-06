#pragma once

# include <iostream>
# include <vector>

struct Info {
	std::string					channelName;
	std::string					password;
	std::vector<std::string>	members;
	bool						isOperator;
};