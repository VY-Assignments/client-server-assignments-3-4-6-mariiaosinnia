#pragma once
#include "Protocol_asmt6.h"
#include <WinSock2.h>
#include <unordered_map>
#include "ClientInfo.h"
#include "Room.h"
#include <future>
#include <optional>

#pragma comment(lib, "ws2_32.lib")
class CommandHandler
{
private:
	std::string file_directory = "server_files";

	std::unordered_map<SOCKET, ClientInfo*>& clients;
	std::unordered_map<uint8_t, Room>& rooms;

	std::mutex pending_mutex;
	std::condition_variable pending_cv;
	std::unordered_map<SOCKET, std::optional<bool>> pending_file_responses;

	void sendStatus(SOCKET socket, Status status);
	void sendFileOffer(SOCKET receiver, SOCKET sender, const std::string& file_name, uint32_t file_size);
	void sendFileData(SOCKET receiver, const std::string& file_name, uint32_t file_size);

public:
	CommandHandler(std::unordered_map<SOCKET, ClientInfo*>& c,
		std::unordered_map<uint8_t, Room>& r) : clients(c), rooms(r) {}
	void handleJoin(ClientInfo& client, uint8_t room_id);
	void handleLeave(ClientInfo& client);
	void handleList(ClientInfo& client);
	void handleWho(ClientInfo& client);
	void handleQuit(ClientInfo& client);
	void handleMessage(ClientInfo& client, std::string& message);
	void handleUpload(ClientInfo& client, const std::string& file_name, const uint32_t file_size);
	void handleFileAccept(ClientInfo& client);
	void handleFileReject(ClientInfo& client);
	void handleDownload(ClientInfo& client, const std::string& file_name);
};

