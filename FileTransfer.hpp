#ifndef FILETRANSFER_HPP
#define FILETRANSFER_HPP

#include <string>

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
};

#endif
