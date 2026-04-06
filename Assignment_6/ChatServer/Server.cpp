#include "Server.h"
#include "Protocol_asmt6.h"
#include <iostream>

void Server::initialize() {
	WSADATA wsa_data;
	if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
		std::cerr << "WSAStartup failed" << std::endl;
		return;
	}
}

void Server::createSocket() {
	server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (server_socket == INVALID_SOCKET) {
		std::cerr << "Error creating socket: " << WSAGetLastError() << std::endl;
		WSACleanup();
		return;
	}
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(port);
}

void Server::bindSocket() {
	if (bind(server_socket, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr))) {
		std::cerr << "Bind failed with error: " << WSAGetLastError() << std::endl;
		closesocket(server_socket);
		WSACleanup();
		return;
	}
}

void Server::startListening() {
	thread_pool.initialize(CLIENTS_NUM);

	if (listen(server_socket, SOMAXCONN) == SOCKET_ERROR) {
		std::cerr << "Listen failed with error: " << WSAGetLastError() << std::endl;
		closesocket(server_socket);
		WSACleanup();
		return;
	}
	std::cout << "Server is listening on a port: " << port << std::endl;
}

void Server::acceptConnection() {
	SOCKET client_socket = accept(server_socket, nullptr, nullptr);
	if (client_socket == SOCKET_ERROR) {
		std::cerr << "Accept failed with error: " << WSAGetLastError() << std::endl;
		return;
	}

	thread_pool.add_task([client_socket, this]() {
		processRequest(client_socket);
		});

}

void Server::processRequest(SOCKET client_socket) {
	ClientInfo clientInfo;
	clientInfo.socket = client_socket;

	while (true) {
		uint16_t header_size = 0;
		if (!recvAll(client_socket, reinterpret_cast<char*>(&header_size), REQUEST_HEADER_SIZE_LEN)) {
			return;
			if (clientInfo.room_id != 0) {
				command_handler.handleLeave(clientInfo);
			}
			closesocket(client_socket);
			return;
		}
		header_size = ntohs(header_size);

		std::vector<char> buffer(header_size);
		if (!recvAll(client_socket, buffer.data(), header_size)) {
			return;
		}

		Request request = request_parser.parseRequest(buffer);
		
		switch (request.command) {
			case Command::join:
				command_handler.handleJoin(clientInfo, request.room_id);
				break;
			case Command::leave:
				command_handler.handleLeave(clientInfo);
				break;
			case Command::list:
				command_handler.handleList(clientInfo);
				break;
			case Command::who:
				command_handler.handleWho(clientInfo);
				break;
			case Command::quit:
				command_handler.handleQuit(clientInfo);
				clients.erase(client_socket);
				break;
			case Command::message:
				command_handler.handleMessage(clientInfo, request.message);
				break;
			case Command::upload:
				std::cout << "Upload: " << request.file_name << " size: " << request.file_size << std::endl;
				command_handler.handleUpload(clientInfo, request.file_name, request.file_size);
				break;
			case Command::file_accept:
				command_handler.handleFileAccept(clientInfo);
				break;
			case Command::file_reject:
				command_handler.handleFileReject(clientInfo);
				break;
			case Command::download:
				command_handler.handleDownload(clientInfo, request.file_name);
				break;
		}
	}
}

void Server::cleanUp() {
	thread_pool.terminate();
	closesocket(server_socket);
	WSACleanup();
}

void Server::run() {
	initialize();
	createSocket();
	bindSocket();
	startListening();

	while (true)
	{
		acceptConnection();
	}

	cleanUp();
}