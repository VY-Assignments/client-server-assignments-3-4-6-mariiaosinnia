#pragma once
#include "ClientInfo.h"
#include <queue>
#include <string>
#include <memory>
#include <WinSock2.h>
#include <thread>
#include <mutex>

#pragma comment(lib, "ws2_32.lib")

struct RoomMessage {
	SOCKET sender_socket;
	std::string message;
};

class Room
{
private:
	uint8_t id;
	std::queue<RoomMessage> message_queue;
	std::vector<ClientInfo*> clients;
	std::thread broadcast_thread;
	std::mutex mutex;
	std::condition_variable cv;

	void sendMessageToEveryone(RoomMessage message);
	void broadcastMessage();

public:
	void initialize();

	void addClient(ClientInfo& client);
	void removeClient(ClientInfo& client);
	void addMessage(SOCKET socket, std::string& message);
	std::vector<ClientInfo*>& getClients();

	void setID(uint8_t room_id);
};

