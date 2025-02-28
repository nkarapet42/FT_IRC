#include "ft_irc.hpp"
#include <bits/basic_string.h>

extern std::vector<Client> clients;

void handleNickCommand(int client_fd, const std::string &message);
void handleUserCommand(int client_fd, const std::string &message);
void handleJoinCommand(int client_fd, const std::string &message);
void handlePrivmsgCommand(int client_fd, const std::string &message);
void handleQuitCommand(int client_fd, const std::string &message);

std::string trim(const std::string &str) {
    size_t first = str.find_first_not_of(' ');
    if (first == std::string::npos)
        return str;
    size_t last = str.find_last_not_of(' ');
    return str.substr(first, (last - first + 1));
}

std::string trimNewline(const std::string &str) {
    size_t pos = str.find_last_not_of("\n\r");
    if (pos == std::string::npos) {
        return str;
    }
    return str.substr(0, pos + 1);
}

void handleClientMessage(int client_fd, const std::string &message) {
    std::string trimmed_message = trim(trimNewline(message));
    std::istringstream iss(trimmed_message);
    std::string command;
    iss >> command;

    if (command == "NICK") {
        handleNickCommand(client_fd, trimmed_message);
    } else if (command == "USER") {
        handleUserCommand(client_fd, trimmed_message);
    } else if (command == "JOIN") {
        handleJoinCommand(client_fd, trimmed_message);
    } else if (command == "PRIVMSG") {
        handlePrivmsgCommand(client_fd, trimmed_message);
    } else if (command == "QUIT") {
        handleQuitCommand(client_fd, trimmed_message);
    } else {
        std::string response = "421 " + command + " :Unknown command\n";
        send(client_fd, response.c_str(), response.size(), 0);
    }
}

void handleNickCommand(int client_fd, const std::string &message) {
    std::istringstream iss(message);
    std::string command, nickname;
    iss >> command >> nickname;

    if (nickname.empty()) {
        std::string response = "431 :No nickname given\n";
        send(client_fd, response.c_str(), response.size(), 0);
        return;
    }

    for (size_t i = 0; i < clients.size(); ++i) {
        if (clients[i].fd == client_fd) {
            clients[i].nickname = nickname;
            std::string response = "NICK " + nickname + "\n";
            send(client_fd, response.c_str(), response.size(), 0);
            break;
        }
    }
}

void handleUserCommand(int client_fd, const std::string &message) {
    std::istringstream iss(message);
    std::string command, username, mode, realname;
    iss >> command >> username >> mode >> realname;

    if (username.empty() || mode.empty() || realname.empty()) {
        std::string response = "461 " + command + " :Not enough parameters\n";
        send(client_fd, response.c_str(), response.size(), 0);
        return;
    }

    for (size_t i = 0; i < clients.size(); ++i) {
        if (clients[i].fd == client_fd) {
            clients[i].username = username;
            clients[i].realname = realname;
            clients[i].authenticated = true;
            std::string response = "USER " + username + " " + mode + " :" + realname + "\n";
            send(client_fd, response.c_str(), response.size(), 0);
            break;
        }
    }
}

void handleJoinCommand(int client_fd, const std::string &message) {
    std::istringstream iss(message);
    std::string command, channel;
    iss >> command >> channel;

    if (channel.empty()) {
        std::string response = "461 " + command + " :Not enough parameters\n";
        send(client_fd, response.c_str(), response.size(), 0);
        return;
    }

    std::string response = "JOIN " + channel + "\n";
    send(client_fd, response.c_str(), response.size(), 0);
}

void handlePrivmsgCommand(int client_fd, const std::string &message) {
    std::istringstream iss(message);
    std::string command, target, msg;
    iss >> command >> target;
    std::getline(iss, msg);

    if (target.empty() || msg.empty()) {
        std::string response = "461 " + command + " :Not enough parameters\n";
        send(client_fd, response.c_str(), response.size(), 0);
        return;
    }

    std::string response = "PRIVMSG " + target + " :" + msg + "\n";
    send(client_fd, response.c_str(), response.size(), 0);
}

void handleQuitCommand(int client_fd, const std::string &message) {
    (void)message;
    std::string response = "QUIT\n";
    send(client_fd, response.c_str(), response.size(), 0);

    close(client_fd);

    for (size_t i = 0; i < clients.size(); ++i) {
        if (clients[i].fd == client_fd) {
            clients.erase(clients.begin() + i);
            break;
        }
    }
    std::cerr << "Client " << client_fd << " disconnected\n";
}