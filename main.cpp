#include "ft_irc.hpp"

std::vector<Client> clients;

void handleClientMessage(int client_fd, const std::string &message);

int main(int argc, char **argv) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <port> <password>" << std::endl;
        return 1;
    }

    int port = std::atoi(argv[1]);
    std::string password = argv[2];

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        std::cerr << "Error creating socket" << std::endl;
        return 1;
    }

    struct sockaddr_in address;
    int addrlen = sizeof(address);
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        std::cerr << "Error binding socket\n";
        return 1;
    }

    if (listen(server_fd, MAX_CLIENTS) < 0) {
        std::cerr << "Error listening on socket\n";
        return 1;
    }

    std::cout << "Server listening on port " << port << "\n";

    struct pollfd fds[MAX_CLIENTS + 1];
    int client_count = 0;

    fds[0].fd = server_fd;
    fds[0].events = POLLIN;

    while (true) {
        int activity = poll(fds, client_count + 1, 0);
        if (activity < 0) {
            std::cerr << "Error in poll\n";
            return 1;
        }

        if (fds[0].revents & POLLIN) {
            int new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
            if (new_socket < 0) {
                std::cerr << "Error accepting connection\n";
                return 1;
            }

            for (int i = 1; i <= MAX_CLIENTS; i++) {
                if (fds[i].fd == 0) {
                    fds[i].fd = new_socket;
                    fds[i].events = POLLIN;
                    clients.push_back(Client(new_socket, "", "", "", false));
                    client_count++;
                    break;
                }
            }
        }

        for (int i = 1; i <= MAX_CLIENTS; i++) {
            if (fds[i].fd != 0 && fds[i].revents & POLLIN) {
                char buffer[1024] = {0};
                int valread = recv(fds[i].fd, buffer, 1024, 0);
                if (valread == 0) {
                    close(fds[i].fd);
                    fds[i].fd = 0;
                    client_count--;
                    continue;
                } else if (valread > 0) {
                    std::string message(buffer, valread);
                    handleClientMessage(fds[i].fd, message);
                } else {
                    std::cerr << "Error reading from socket\n";
                    close(fds[i].fd);
                    fds[i].fd = 0;
                    client_count--;
                }
            }
        }
    }

    return 0;
}