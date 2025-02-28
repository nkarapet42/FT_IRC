#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <cstring>
#include <cstdlib>
#include <bits/basic_string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <poll.h>

#define PORT 6667
#define MAX_CLIENTS 10

struct Client {
    int fd;
    std::string nickname;
    std::string username;
    std::string realname;
    bool authenticated;

    // Constructor for C++98 compatibility
    Client(int fd, const std::string &nickname, const std::string &username, const std::string &realname, bool authenticated)
        : fd(fd), nickname(nickname), username(username), realname(realname), authenticated(authenticated) {}
};