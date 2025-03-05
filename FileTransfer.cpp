#include "FileTransfer.hpp"

FileTransfer::FileTransfer()
    : senderFd(-1), receiverFd(-1), fileSize(0) {}

FileTransfer::FileTransfer(int sender, int receiver, const std::string& file, size_t size)
    : senderFd(sender), receiverFd(receiver), fileName(file), fileSize(size) {}

FileTransfer::~FileTransfer() {}

FileTransfer::FileTransfer(const FileTransfer& other)
    : senderFd(other.senderFd), receiverFd(other.receiverFd),
      fileName(other.fileName), fileSize(other.fileSize) {}

FileTransfer& FileTransfer::operator=(const FileTransfer& other) {
    if (this == &other) {
        return *this;
    }
    
    senderFd = other.senderFd;
    receiverFd = other.receiverFd;
    fileName = other.fileName;
    fileSize = other.fileSize;

    return *this;
}
