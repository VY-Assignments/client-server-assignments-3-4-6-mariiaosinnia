#pragma once
#include <vector>
#include <string>
#include "Protocol_asmt6.h"
#include <WinSock2.h>

#pragma comment(lib, "ws2_32.lib")

class RequestBuilder {
private:
    std::vector<char> serializeClientSocket(SOCKET socket);
    std::vector<char> serializeTag(Tag tag);
    std::vector<char> serializeCommand(Command command);
    std::vector<char> serializeRoomID(uint8_t room_id);
    std::vector<char> serializeFileName(const std::string& file_name);
    std::vector<char> serializeFileSize(uint32_t file_size);

public:
    std::vector<char> buildJoinRequest(const uint8_t room_id); 
    std::vector<char> buildLeaveRequest();
    std::vector<char> buildListRequest();
    std::vector<char> buildWhoRequest();
    std::vector<char> buildMessage(const std::string& message);
    std::vector<char> buildUploadRequest(const std::string& file_name, const uint32_t file_size);
    std::vector<char> buildFileAcceptRequest();
    std::vector<char> buildFileRejectRequest();
    std::vector<char> buildDownloadRequest(const std::string& file_name);
    std::vector<char> buildQuitRequest();
};

