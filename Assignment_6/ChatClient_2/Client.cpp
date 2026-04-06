#include "Client.h"
#include <iostream>
#include <Ws2tcpip.h>
#include <fstream>
#include <algorithm>
#include <filesystem>
#include <sstream>

void Client::initialize() {
	WSADATA wsa_data;
	if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
		std::cerr << "WSAStartup failed" << std::endl;
		return;
	}
}

void Client::createSocket() {
	client_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (client_socket == INVALID_SOCKET) {
		std::cerr << "Error creating socket " << WSAGetLastError() << std::endl;
		WSACleanup();
		return;
	}
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	InetPton(AF_INET, server_ip, &server_addr.sin_addr);
}

void Client::connectToServer() {
	if (connect(client_socket, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) == SOCKET_ERROR) {
		std::cerr << "Connect failed with error: " << WSAGetLastError() << std::endl;
		closesocket(client_socket);
		WSACleanup();
		return;
	}
}

bool Client::sendRequest(std::vector<char>& buffer) {
	uint16_t header_size = static_cast<uint16_t>(buffer.size());
	uint16_t net_header_size = htons(header_size);
	if (!sendAll(client_socket, reinterpret_cast<char*>(&net_header_size), REQUEST_HEADER_SIZE_LEN)) {
		return false;
	}

	if (!sendAll(client_socket, buffer.data(), header_size)) {
		return false;
	}
	return true;
}

void Client::cleanUp() {
	closesocket(client_socket);
	WSACleanup();
}

Command Client::parseCommand(const std::string& str) {
	if (str == "join") {
		return Command::join;
	}
	else if (str == "leave") {
		return Command::leave;
	}
	else if (str == "list") {
		return Command::list;
	}
	else if (str == "who") {
		return Command::who;
	}
	else if (str == "upload") {
		return Command::upload;
	}
	else if (str == "download") {
		return Command::download;
	}
	else if (str == "accept") {
		return Command::file_accept;
	}
	else if (str == "reject") {
		return Command::file_reject;
	}
	else {
		return Command::unknown;
	}
}

void Client::cliLoop() {
	std::string line;
	while (running) {
		{
			std::lock_guard<std::mutex> lock(cout_mutex);
			std::cout << "> " << std::flush;
		}
		std::getline(std::cin, line);
		if (line.empty()) {
			continue;
		}

		std::istringstream iss(line);
		std::string command_or_message;
		iss >> command_or_message;

		if (command_or_message.front() == '/') {
			std::string command = command_or_message.erase(0, 1);
			std::vector<std::string> args;
			std::string arg;
			while (iss >> arg) args.push_back(arg);

			const int FIRST_ARG_INDEX = 0;
			Command cmd = parseCommand(command);
			switch (cmd) {
			case Command::join:
				if (args.empty()) {
					print("Usage: /join <room_number>");
					break;
				}
				try {
					join(static_cast<uint8_t>(std::stoi(args[FIRST_ARG_INDEX])));
				}
				catch (const std::invalid_argument&) {
					print("Invalid room number");
				}
				catch (const std::out_of_range&) {
					print("Room number out of range");
				}
				break;
			case Command::leave:
				leave();
				break;
			case Command::list:
				list();
				break;
			case Command::who:
				who();
				break;
			case Command::upload:
				if (args.empty()) {
					print("Usage: /upload <file_name>");
					break;
				}
				upload(args[FIRST_ARG_INDEX]);
				break;
			case Command::file_accept:
				fileAccept();
				break;
			case Command::file_reject:
				fileReject();
				break;
			case Command::download:
				if (args.empty()) {
					print("Usage: /download <file_name>");
					break;
				}
				download(args[FIRST_ARG_INDEX]);
				break;
			}
		}
		else {
			message(line);
		}
	}
}

void Client::print(const std::string& msg) {
	std::lock_guard<std::mutex> lock(cout_mutex);
	std::cout << msg << std::endl;
}

void Client::receiveStatus()
{
	uint8_t byte;
	if (!recvAll(client_socket, reinterpret_cast<char*>(&byte), STATUS_LEN)) {
		return;
	}

	{
		std::lock_guard<std::mutex> lock(mutex);
		status = static_cast<Status>(byte);
	}
	cv.notify_one();
}

Status Client::waitForStatus(){
	std::unique_lock<std::mutex> lock(mutex);
	cv.wait(lock, [this] { return status.has_value(); });

	Status stat = status.value();
	status.reset();

	return stat;
}

uint8_t Client::receiveRoomsNum(){
	uint8_t rooms_num;
	recvAll(client_socket, reinterpret_cast<char*>(&rooms_num), ROOMS_NUM_LEN);
	return ntohl(rooms_num);
}

uint8_t Client::receiveClientsNum(){
	uint8_t clients_num;
	recvAll(client_socket, reinterpret_cast<char*>(&clients_num), CLIENTS_NUM_LEN);
	return ntohl(clients_num);
}


Client::~Client() {
	running = false;
	closesocket(client_socket);
	if (receive_thread.joinable()) {
		receive_thread.join();
	}
	if (cli_thread.joinable()) {
		cli_thread.join();
	}
	WSACleanup();
}

void Client::run() {
	initialize();
	createSocket();
	connectToServer();

	receive_thread = std::thread(&Client::receiveLoop, this);
	cli_thread = std::thread(&Client::cliLoop, this);
	cli_thread.join();
}

void Client::receiveLoop() {
	while (running) {
		Tag tag;
		if (!recvAll(client_socket, reinterpret_cast<char*>(&tag), TAG_LEN)) {
			return;
		}

		switch (tag) {
			case Tag::status:
				receiveStatus();
				break;
			case Tag::message: {
				SOCKET sender_socket;
				if (!recvAll(client_socket, reinterpret_cast<char*>(&sender_socket), sizeof(sender_socket))) {
					break;
				}
				sender_socket = ntohll(sender_socket);

				uint32_t msg_size;
				if (!recvAll(client_socket, reinterpret_cast<char*>(&msg_size), MESSAGE_SIZE_LEN)) {
					break;
				}
				msg_size = ntohl(msg_size);

				std::vector<char> buffer(msg_size);
				if (!recvAll(client_socket, buffer.data(), msg_size)) {
					break;
				}

				std::string msg(buffer.begin(), buffer.end());
				print("[Client " + std::to_string(sender_socket) + "] " + msg);
				break;
			}
			case Tag::who_response: {
				uint8_t clients_num;
				if (!recvAll(client_socket, reinterpret_cast<char*>(&clients_num), CLIENTS_NUM_LEN)) {
					break;
				}

				std::vector<SOCKET> clients;

				for (int i = 0; i < clients_num; i++) {
					SOCKET socket;
					if (!recvAll(client_socket, reinterpret_cast<char*>(&socket), sizeof(socket))) {
						break;
					}
					socket = ntohll(socket);
					clients.push_back(socket);
				}

				{
					std::lock_guard<std::mutex> lock(mutex);
					who_result = clients;
				}
				cv.notify_one();
				break;
			}
			case Tag::list_response: {
				uint8_t rooms_num;
				if (!recvAll(client_socket, reinterpret_cast<char*>(&rooms_num), ROOMS_NUM_LEN)) {
					break;
				}

				std::vector<uint8_t> rooms;
				for (int i = 0; i < rooms_num; i++) {
					uint8_t room_id;
					if (!recvAll(client_socket, reinterpret_cast<char*>(&room_id), sizeof(room_id))) {
						break;
					}
					rooms.push_back(room_id);
				}

				{
					std::lock_guard<std::mutex> lock(mutex);
					list_result = rooms;
				}
				cv.notify_one();
				break;
			}
			case Tag::file_offer: {
				SOCKET sender_socket;
				if (!recvAll(client_socket, reinterpret_cast<char*>(&sender_socket), sizeof(sender_socket))) break;
				sender_socket = ntohll(sender_socket);

				uint16_t name_len;
				if (!recvAll(client_socket, reinterpret_cast<char*>(&name_len), sizeof(name_len))) break;
				name_len = ntohs(name_len);

				std::string file_name(name_len, '\0');
				if (!recvAll(client_socket, file_name.data(), name_len)) break;

				uint32_t file_size;
				if (!recvAll(client_socket, reinterpret_cast<char*>(&file_size), sizeof(file_size))) break;
				file_size = ntohl(file_size);

				std::lock_guard<std::mutex> lock(offers_mutex);
				pending_offers.push({ sender_socket, file_name, file_size });
				print("\n[Client " + std::to_string(sender_socket) + "] wants to send " +
					file_name + " (" + std::to_string(file_size) + " bytes). Type /accept or /reject");
				break;
			}
			case Tag::file_data: {
				uint16_t name_len;
				if (!recvAll(client_socket, reinterpret_cast<char*>(&name_len), sizeof(name_len))) {
					break;
				}
				name_len = ntohs(name_len);

				std::string file_name(name_len, '\0');
				if (!recvAll(client_socket, file_name.data(), name_len)) break;

				uint32_t file_size;
				if (!recvAll(client_socket, reinterpret_cast<char*>(&file_size), sizeof(file_size))) break;
				file_size = ntohl(file_size);

				std::filesystem::create_directories(file_directory);
				std::ofstream file(file_directory + "/" + file_name, std::ios::binary);
				if (!file.is_open()) {
					print("Failed to save file " + file_name);
					break;
				}

				std::vector<char> buffer(CHUNK_SIZE);
				uint32_t total_received = 0;
				while (total_received < file_size) {
					uint32_t bytes_to_read = std::min<uint32_t>(CHUNK_SIZE, file_size - total_received);
					if (!recvAll(client_socket, buffer.data(), bytes_to_read)) {
						break;
					}
					file.write(buffer.data(), bytes_to_read);
					total_received += bytes_to_read;
				}
				file.close();

				print("File " + file_name + " has been downloaded");
				break;
			}
		}
	}
}

void Client::join(uint8_t room_id){
	std::vector<char> request = request_builder.buildJoinRequest(room_id);
	sendRequest(request);

	Status stat = waitForStatus();

	if (stat == Status::SUCCESS) {
		print("Joined room successfully");
	}
	else if (stat == Status::ALREADY_IN_THIS_ROOM) {
		print("You are already in a room " + std::to_string(room_id));
	}
	else if (stat == Status::INVALID_ROOM_NUM) {
		print("Invalid room number (Must be: 1 <= room number <= 255)");
	}
	else if (stat == Status::MUST_LEAVE_ROOM_BEFORE_JOIN) {
		print("You must leave the current room before joining the new one");
	}
}

void Client::leave(){
	std::vector<char> request = request_builder.buildLeaveRequest();
	sendRequest(request);

	Status stat = waitForStatus();

	if (stat == Status::SUCCESS) {
		print("You left the room");
	}
	else if (stat == Status::CLIENT_HAS_NO_ROOM) {
		print("You are not in the room");
	}
}

void Client::list() {
    std::vector<char> request = request_builder.buildListRequest();
    sendRequest(request);

    Status stat = waitForStatus();

    if (stat == Status::SUCCESS) {
        std::unique_lock<std::mutex> lock(mutex);
        cv.wait(lock, [this] { return list_result.has_value(); });

        std::vector<uint8_t> rooms = list_result.value();
        list_result.reset();

        print("Available rooms:");
        for (auto r : rooms) {
            print(std::to_string(r));
        }
    }
}

void Client::who(){
	std::vector<char> request = request_builder.buildWhoRequest();
	sendRequest(request);

	Status stat = waitForStatus();

	if (stat == Status::CLIENT_HAS_NO_ROOM) {
		print("You are not in a room");
	}
	else if (stat == Status::SUCCESS) {
		std::unique_lock<std::mutex> lock(mutex);
		cv.wait(lock, [this] { return who_result.has_value(); });

		std::vector<SOCKET> clients = who_result.value();
		who_result.reset();

		if (clients.size() == 0) {
			print("You are alone in this room");
			return;
		}

		print("Clients in the room: ");
		for (auto c : clients) {
			print("Client " + std::to_string(c));
		}
	}
}

void Client::message(const std::string& message){
	std::vector<char> request = request_builder.buildMessage(message);
	sendRequest(request);

	Status stat = waitForStatus();

	if (stat == Status::CLIENT_HAS_NO_ROOM) {
		print("You are not in a room");
	}
}

void Client::upload(const std::string& file_name){
	std::ifstream file(file_directory + "/" + file_name, std::ios::binary);
	if (!file.is_open()) {
		print("File not found");
		return;
	}

	file.seekg(0, std::ios::end);
	uint32_t file_size = static_cast<uint32_t>(file.tellg());
	file.seekg(0, std::ios::beg);

	std::vector<char> header = request_builder.buildUploadRequest(file_name, file_size);
	if (!sendRequest(header)) {
		print("Failed to send request");
		return;
	}

	std::vector<char> buffer(CHUNK_SIZE);
	int total_sent = 0;
	while (total_sent < file_size) {
		int to_sent = std::min<int>(CHUNK_SIZE, file_size - total_sent);
		file.read(buffer.data(), to_sent);
		if (!sendAll(client_socket, buffer.data(), to_sent)) {
			return;
		}
		total_sent += to_sent;
	}

	Status stat = waitForStatus();

	if (stat == Status::SUCCESS) {
		print("File was successfully uploaded to the server");
	}
	else {
		print("Upload failed");
	}
	print("Upload status: " + std::to_string(static_cast<int>(stat)));
}

void Client::fileAccept() {
	std::vector<char> request = request_builder.buildFileAcceptRequest();
	sendRequest(request);
}

void Client::fileReject() {
	std::vector<char> request = request_builder.buildFileRejectRequest();
	sendRequest(request);
}

void Client::download(const std::string& file_name){
	std::vector<char> request = request_builder.buildDownloadRequest(file_name);
	sendRequest(request);

	Status stat = waitForStatus();
	if (stat == Status::FILE_NOT_FOUND) {
		print("File not found on server");
	}
}
