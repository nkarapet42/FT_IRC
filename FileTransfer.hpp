#pragma once

#include <string>

class Server;

class FileTransfer {
public:
	int			senderFd;
	int			receiverFd;
	std::string	fileName;
	size_t		fileSize;

	FileTransfer();
	FileTransfer(int sender, int receiver, const std::string& file, size_t size);
	FileTransfer(const FileTransfer& other);
    FileTransfer& operator=(const FileTransfer& other);
	~FileTransfer();

	void	dccSend(int senderFd, Server& server, const std::string& line);
	void	dccGet(int receiverFd, Server& server, const std::string& line);
	void	connectToSender(int receiverFd, Server& server, const FileTransfer& transfer);
	void	startFileTransfer(int receiverFd, Server& server, const std::string& filename, size_t fileSize);
	
};
