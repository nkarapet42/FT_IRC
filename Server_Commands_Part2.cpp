#include "Server.hpp"

void Server::whoCommand(int clientFd, const std::string& line) {
	std::stringstream ss(line);
	std::string cmd, channelName;
	ss >> cmd >> channelName;

	std::string restOfLine;
	std::getline(ss, restOfLine);
	if (!restOfLine.empty()) {
        sendErrorMessage(clientFd, "Error: Wrong Syntax.", 401);
		sendMessage(clientFd, std::string(RED) + "Usage: WHO [channel].\n" + std::string(RESET));
		return;
	}
    if (channelName.empty()) {
        for (std::vector<Channel>::iterator it = channelsIRC.begin(); it != channelsIRC.end(); ++it) {
            Channel& channel = *it;
            std::string response = "WHO " + channel.channelName + " :";
            for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
                Client& client = it->second;
                for (std::vector<Info>::iterator infoIt = client.channels.begin(); infoIt != client.channels.end(); ++infoIt) {
                    if (infoIt->channelName == channel.channelName) {
                        response += client.nickname + " ";
                        response += (client.isOperator ? "(operator) " : "(normal) ");
                        break;
                    }
                }
            }

            sendMessage(clientFd, std::string(BLUE) + response + "\n" + std::string(RESET));
        }
    } else {
        Channel* channel = NULL;
        for (std::vector<Channel>::iterator it = channelsIRC.begin(); it != channelsIRC.end(); ++it) {
            if (it->channelName == channelName) {
                channel = &(*it);
                break;
            }
        }
        if (!channel) {
            sendErrorMessage(clientFd, "Error: Channel not found.", 401);
            return;
        }
        std::string response = "WHO " + channelName + " :";
        for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
            Client& client = it->second;
            for (std::vector<Info>::iterator infoIt = client.channels.begin(); infoIt != client.channels.end(); ++infoIt) {
                if (infoIt->channelName == channelName) {
                    response += client.nickname + " ";
                    response += (client.isOperator ? "(operator) " : "(normal) ");
                    break;
                }
            }
        }
        sendMessage(clientFd, std::string(BLUE) + response + "\n" + std::string(RESET));
    }
}

void Server::partChannel(int clientFd, const std::string& line) {
	std::stringstream ss(line);
	std::string cmd, channelName;
	ss >> cmd >> channelName;

	if (channelName.empty()) {
        sendErrorMessage(clientFd, "Error: Wrong Syntax.", 401);
		sendMessage(clientFd, std::string(RED) + "Usage: PART <channel> [message].\n" + std::string(RESET));
		return;
	}
	std::string message;
	std::getline(ss, message);
	if (message.empty())
		message += "GoodBye my Friend's";
	Channel* channel = NULL;
	for (std::vector<Channel>::iterator it = channelsIRC.begin(); it != channelsIRC.end(); ++it) {
		if (it->channelName == channelName) {
			channel = &(*it);
			break;
		}
	}
	if (!channel) {
        sendErrorMessage(clientFd, "Error: Channel not found.", 401);
		return;
	}
	if (_clients[clientFd].leaveChannel(channelName)){
		sendMessage(clientFd, std::string(GREEN) + "You left the channel.\n" + std::string(RESET));
	} else {
		sendMessage(clientFd, std::string(RED) + "You are not in channel.\n" + std::string(RESET));
	}
	broadcastMessage(channelName, clientFd, std::string(CYAN) + "Last words : " + message + "\n" + std::string(RESET));
}

void Server::topicCommand(int clientFd, const std::string& line) {
	std::stringstream ss(line);
	std::string cmd, channelName;
	ss >> cmd >> channelName;

	if (channelName.empty()) {
        sendErrorMessage(clientFd, "Error: Wrong Syntax.", 401);
		sendMessage(clientFd, std::string(RED) + "Usage: TOPIC <channel> [topic].\n" + std::string(RESET));
		return;
	}
	std::string topic;
	std::getline(ss, topic);

    Channel* channel = NULL;
    for (std::vector<Channel>::iterator it = channelsIRC.begin(); it != channelsIRC.end(); ++it) {
        if (it->channelName == channelName) {
            channel = &(*it);
            break;
        }
    }

    if (!channel) {
        sendErrorMessage(clientFd, "Error: Channel not found.", 401);
        return;
    }

    if (topic.empty()) {
        std::string response = "TOPIC " + channelName + " :" + (channel->topic.empty() ? "No topic set." : channel->topic);
        sendMessage(clientFd, std::string(CYAN) + response + "\n" + std::string(RESET));
    } else {
        Client& client = _clients[clientFd];
        bool isOperator = false;
        for (std::vector<Info>::iterator it = client.channels.begin(); it != client.channels.end(); ++it) {
            if (it->channelName == channelName && it->isOperator) {
                isOperator = true;
                break;
            }
        }

        if (!channel->isTopic || isOperator) {
            channel->topic = topic;
            std::string response = "TOPIC " + channelName + " :" + topic + "\n";
            sendMessage(clientFd, std::string(CYAN) + response + std::string(RESET));
            broadcastMessage(channelName, clientFd, std::string(CYAN) + response + std::string(RESET));
        } else {
            sendErrorMessage(clientFd, "Error: You are not an operator of this channel.", 482);
        }
    }
}