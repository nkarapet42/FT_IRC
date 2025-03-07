#include "Server.hpp"

void Server::startFileTransfer(int receiverFd, const std::string& filename, size_t fileSize) {
    std::ifstream file(filename.c_str(), std::ios::binary);
    if (!file.is_open()) {
        sendErrorMessage(receiverFd, "Error: Unable to open file for transfer.", 401);
        return;
    }

    const size_t bufferSize = 1024;
    char buffer[bufferSize];

    size_t bytesSent = 0;
    while (bytesSent < fileSize) {
        size_t bytesToRead = std::min(bufferSize, fileSize - bytesSent);
        file.read(buffer, bytesToRead);

        if (file.gcount() > 0) {
            ssize_t bytesWritten = send(receiverFd, buffer, file.gcount(), 0);
            if (bytesWritten == -1) {
                sendErrorMessage(receiverFd, "Error: Unable to send file.", 401);
                break;
            }
            bytesSent += bytesWritten;
        }
    }

    file.close();
    sendMessage(receiverFd, std::string(YELLOW) + "Capacity of File\n" + std::string(RESET));
    sendMessage(receiverFd, std::string(GREEN) + "File transfer completed.\n" + std::string(RESET));
}


 void Server::dccSend(int senderFd, const std::string& line) {
	std::stringstream ss(line);
    std::string cmd, type, receiverNick, filename;
    ss >> cmd >> type >> receiverNick >> filename;
	
	if (type.empty() || type != "SEND") {
		sendMessage(senderFd, std::string(RED) + "Usage: DCC SEND <nickname> <filename>\n" + std::string(RESET));
		return;
	}
    if (receiverNick.empty() || filename.empty()) {
        sendMessage(senderFd, std::string(RED) + "Usage: DCC SEND <nickname> <filename>\n" + std::string(RESET));
        return;
    }
	if (_clients[senderFd].nickname == receiverNick) {
        sendErrorMessage(senderFd, "Error: You cannot send a file to yourself.", 401);
		return;
	}

    int receiverFd = -1;
    for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
        if (it->second.nickname == receiverNick) {
            receiverFd = it->first;
            break;
        }
    }

    if (receiverFd == -1) {
        sendErrorMessage(senderFd, "Error: User not found.", 401);
        return;
    }

	if (activeTransfers.find(filename) != activeTransfers.end()
		&& activeTransfers.find(filename)->second.senderFd == senderFd
		&& activeTransfers.find(filename)->second.receiverFd == receiverFd) {
        sendErrorMessage(senderFd, "Error: File transfer for '" + filename + "' is already in progress.\n", 401);
        return;
    }

    std::ifstream file(filename.c_str(), std::ios::binary | std::ios::ate);
    if (!file) {
        sendErrorMessage(senderFd, std::string(RED) + "Error: File not found.\n" + std::string(RESET), 401);
        return;
    }
    int fileSize = file.tellg();
    file.close();

    std::stringstream sizeStream;
    sizeStream << fileSize;
    std::string fileSizeStr = sizeStream.str();
	
	activeTransfers[filename] = FileTransfer(senderFd, receiverFd, filename, fileSize);

    std::string dccMessage = "DCC SEND " + filename + " " + _clients[senderFd].nickname + " " + fileSizeStr + "\n"
							+ "USE: DCC GET <filename> for accepting";
    sendMessage(receiverFd, std::string(GREEN) + dccMessage + "\n" + std::string(RESET));

    startFileTransfer(receiverFd, filename, fileSize);
}

static std::string generateUniqueFileName(const std::string& filename) {
    std::time_t now = std::time(0);
    std::tm* 	timeInfo = std::localtime(&now);

    char timeBuffer[31];
    std::sprintf(timeBuffer, "%04d%02d%02d%02d%02d%02d", 
                 1900 + timeInfo->tm_year, timeInfo->tm_mon + 1, timeInfo->tm_mday,
                 timeInfo->tm_hour, timeInfo->tm_min, timeInfo->tm_sec);

    std::stringstream ss;
    ss << "dcc_" << timeBuffer << "_" << filename;
    return ss.str();
}

void Server::connectToSender(int receiverFd, const FileTransfer& transfer) {
    int senderFd = transfer.senderFd;
    const std::string& filename = transfer.fileName;
    size_t fileSize = transfer.fileSize;

    if (_clients.find(senderFd) == _clients.end()) {
        sendErrorMessage(receiverFd, "Error: Sender is no longer connected.", 401);
        return;
    }

    std::string dccFilePath = generateUniqueFileName(filename);

    std::ofstream dccFile(dccFilePath.c_str(), std::ios::binary);
    if (!dccFile.is_open()) {
        sendErrorMessage(receiverFd, std::string(RED) + "Error: Unable to open file for transfer.\n" + std::string(RESET), 999);
        return;
    }

    std::ifstream file(filename.c_str(), std::ios::binary);
    if (!file.is_open()) {
        sendErrorMessage(receiverFd, std::string(RED) + "Error: Unable to open file for transfer.\n" + std::string(RESET), 999);
        return;
    }

    char buffer[1024];
    while (file.read(buffer, sizeof(buffer)) || file.gcount() > 0) {
        dccFile.write(buffer, file.gcount());
    }
    file.close();
    dccFile.close();

    std::stringstream info;
    info << "DCC GET " << filename << " " << fileSize << " " << dccFilePath;
    sendMessage(receiverFd, std::string(YELLOW) + info.str() + "\n" + std::string(RESET));

    sendMessage(receiverFd, std::string(GREEN) + "File transfer completed." + std::string(RESET));
}


void Server::dccGet(int receiverFd, const std::string& line) {
    std::stringstream ss(line);
    std::string cmd, type ,filename;
    ss >> cmd >> type >> filename;

	if (type.empty() || type != "GET") {
		sendMessage(receiverFd, std::string(RED) + "Usage: DCC GET <filename>\n" + std::string(RESET));
		return;
	}
    if (filename.empty()) {
        sendMessage(receiverFd, std::string(RED) + "Usage: DCC GET <filename>\n" + std::string(RESET));
        return;
    }

	std::map<std::string, FileTransfer>::iterator it = activeTransfers.find(filename);
    if (it == activeTransfers.end()) {
        sendErrorMessage(receiverFd, "Error: No file transfer found for '" + filename + "'.\n", 401);
        return;
    }

	FileTransfer& transfer = it->second;

    if (transfer.receiverFd != receiverFd) {
        sendErrorMessage(receiverFd, "Error: No file transfer found for '" + filename + "'.\n", 401);
        return;
    }

    sendMessage(transfer.senderFd, std::string(YELLOW) + "User " + _clients[receiverFd].nickname + " has accepted your file: " + filename + "\n" + std::string(RESET));
    sendMessage(receiverFd, std::string(YELLOW) + "You accepted the file: " + filename + " from " + _clients[transfer.senderFd].nickname + ". Waiting for transfer...\n" + std::string(RESET));

    connectToSender(receiverFd, transfer);
    activeTransfers.erase(it);
}