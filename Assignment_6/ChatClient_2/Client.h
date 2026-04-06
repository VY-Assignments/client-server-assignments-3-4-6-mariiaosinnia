#pragma once
#include <WinSock2.h>
#include <vector>
#include "Protocol_asmt6.h"
#include "RequestBuilder.h"
#include <string>
#include <mutex>
#include <optional>
#include <queue>

#pragma comment(lib, "ws2_32.lib")

struct FileOffer {
	SOCKET sender_socket;
	std::string file_name;
	uint32_t file_size;
};

class Client {
private:
	int port = 12345;
	SOCKET client_socket;
	PCWSTR server_ip = L"127.0.0.1";
	sockaddr_in server_addr;

	std::atomic<bool> running = true;

	std::thread receive_thread;
	std::thread cli_thread;
	std::mutex cout_mutex;

	RequestBuilder request_builder;

	std::string file_directory = "client_files";

	std::mutex mutex;
	std::condition_variable cv;

	std::queue<FileOffer> pending_offers;
	std::mutex offers_mutex;
	
	std::optional<Status> status;
	std::optional <std::vector<SOCKET>> who_result;
	std::optional<std::vector<uint8_t>> list_result;

	void initialize();
	void createSocket();
	void connectToServer();
	bool sendRequest(std::vector<char>& buffer);
	void cleanUp();

	Command parseCommand(const std::string& str);
	void cliLoop();
	void receiveLoop();

	void print(const std::string& msg);
	void receiveStatus();
	Status waitForStatus();
	uint8_t receiveRoomsNum();
	uint8_t receiveClientsNum();
public:
	~Client();
	void run();

	void join(uint8_t room_id);
	void leave();
	void list();
	void who();
	void message(const std::string& message);
	void upload(const std::string& file_name);
	void fileAccept();
	void fileReject();
	void download(const std::string& file_name);
};

