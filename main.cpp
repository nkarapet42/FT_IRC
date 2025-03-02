#include "Server.hpp"

std::vector<Channel> channelsIRC;

bool isValidPort(const char* str) {
	for (int i = 0; str[i] != '\0'; i++) {
		if (!std::isdigit(str[i])) {
			return false;
		}
	}
	long port = std::strtol(str, NULL, 10);
	return (port > 0 && port <= 65535);
}

int main(int argc, char* argv[]) {
	if (argc != 3) {
		std::cerr << "Usage: ./ircserv <port> <password>\n";
		return (1);
	}
	if (!isValidPort(argv[1])) {
		std::cerr << "Error: Invalid port. Port must be a number between 1 and 65535.\n";
		return (1);
	}
	int port = std::atoi(argv[1]);
	std::string password = argv[2];

	Server server(port, password);
	server.run();

	return (0);
}