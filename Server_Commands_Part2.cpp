#include "Server.hpp"

void Server::whoCommand(int clientFd, const std::string& line) {
	std::stringstream ss(line);
	std::string cmd, channelName;
	ss >> cmd >> channelName;

    endErase(cmd);
    endErase(channelName);
	std::string restOfLine;
	std::getline(ss, restOfLine);
    endErase(restOfLine);
	if (!restOfLine.empty()) {
        sendErrorMessage(clientFd, "Error: Wrong Syntax.", 461);
		sendMessage(clientFd, std::string(RED) + "Usage: WHO [channel].\n" + std::string(RESET), "WHO", 461);
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
                        response += (infoIt->isOperator ? "(operator) " : "(normal) ");
                        break;
                    }
                }
            }

            sendMessage(clientFd, std::string(BLUE) + response + "\n" + std::string(RESET), "WHO", 352);
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
            sendErrorMessage(clientFd, "Error: Channel not found.", 403);
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
        sendMessage(clientFd, std::string(BLUE) + response + "\n" + std::string(RESET), "WHO", 352);
    }
}

void Server::partChannel(int clientFd, const std::string& line) {
	std::stringstream ss(line);
	std::string cmd, channelName;
	ss >> cmd >> channelName;

    endErase(cmd);
    endErase(channelName);
	if (channelName.empty()) {
        sendErrorMessage(clientFd, "Error: Wrong Syntax.", 461);
		sendMessage(clientFd, std::string(RED) + "Usage: PART <channel> [message].\n" + std::string(RESET), "PART", 461);
		return;
	}
	std::string message;
	std::getline(ss, message);
    endErase(message);
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
        sendErrorMessage(clientFd, "Error: Channel not found.", 403);
		return;
	}
	if (_clients[clientFd].leaveChannel(channelName)){
		sendMessage(clientFd, std::string(GREEN) + "You left the channel.\n" + std::string(RESET), "PART", 3);
	} else {
        sendErrorMessage(clientFd, "Error: You are not in channel.", 442);
	}
	broadcastMessage(channelName, clientFd, std::string(CYAN) + "Last words : " + message + "\n" + std::string(RESET), "PART", 3);
}

void Server::topicCommand(int clientFd, const std::string& line) {
	std::stringstream ss(line);
	std::string cmd, channelName;
	ss >> cmd >> channelName;

    endErase(cmd);
    endErase(channelName);
	if (channelName.empty()) {
        sendErrorMessage(clientFd, "Error: Wrong Syntax.", 461);
		sendMessage(clientFd, std::string(RED) + "Usage: TOPIC <channel> [topic].\n" + std::string(RESET), "TOPIC", 461);
		return;
	}
	std::string topic;
	std::getline(ss, topic);
    endErase(topic);

    Channel* channel = NULL;
    for (std::vector<Channel>::iterator it = channelsIRC.begin(); it != channelsIRC.end(); ++it) {
        if (it->channelName == channelName) {
            channel = &(*it);
            break;
        }
    }

    if (!channel) {
        sendErrorMessage(clientFd, "Error: Channel not found.", 403);
        return;
    }

    if (topic.empty()) {
        std::string response = "TOPIC " + channelName + " :" + (channel->topic.empty() ? "No topic set." : channel->topic);
        sendMessage(clientFd, std::string(CYAN) + response + "\n" + std::string(RESET), "TOPIC", (channel->topic.empty() ? 331 : 332));
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
            sendMessage(clientFd, std::string(CYAN) + response + std::string(RESET), "TOPIC", 750);
            broadcastMessage(channelName, clientFd, std::string(CYAN) + response + std::string(RESET), "TOPIC", 750);
        } else {
            sendErrorMessage(clientFd, "Error: You are not an operator of this channel.", 482);
        }
    }
}