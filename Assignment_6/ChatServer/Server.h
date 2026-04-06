#pragma once
#include "RequestParser.h"
#include "ThreadPool.h"
#include "CommandHandler.h"
#include "ClientInfo.h"
#include "Room.h"
#include "Protocol_asmt6.h"
#include <WinSock2.h>
#include <unordered_map>

#pragma comment(lib, "ws2_32.lib")

class Server
{
private:
	int port = 12345;
	SOCKET server_socket;
	sockaddr_in server_addr;

	const int CLIENTS_NUM = 20;
	//do i need thid? maybe i can switch to std::vector<ClientInfo> or even be without it all
	std::unordered_map<SOCKET, ClientInfo*> clients;
	std::unordered_map<uint8_t, Room> rooms;

	ThreadPool thread_pool;
	RequestParser request_parser;
	CommandHandler command_handler;

	//bool is_running
	void initialize();
	void createSocket();
	void bindSocket();
	void startListening();
	void acceptConnection();
	void processRequest(SOCKET client_socket);
	//void console_loop();
	void cleanUp();
public:
	Server() : command_handler(clients, rooms){}
	void run();
};

