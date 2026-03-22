#pragma once
#include <string>
#include <WinSock2.h>

#pragma comment(lib, "ws2_32.lib")

const int REQUEST_HEADER_SIZE_LEN = 2;
const int MESSAGE_SIZE_LEN = 4;
const int PREFIX_LEN = 1;
const int COMMAND_LEN = 1;
const int ROOM_ID_LEN = 1;
const int FILE_NAME_SIZE_LEN = 1;
const int FILE_SIZE_LEN = 4;
const int STATUS_LEN = 1;

const uint8_t min_room_num = 1;
const uint8_t max_room_num = 255;

enum class Command : uint8_t {
	message,
	join,
	leave,
	list,
	who,
	upload,
	download,
	quit,
	unknown
};

enum class Status : uint8_t {
	SUCCESS,
	FILE_NOT_FOUND,
	INVALID_ROOM_NUM,
	CLIENT_HAS_NO_ROOM,
	ALREADY_IN_THIS_ROOM,
	MUST_LEAVE_ROOM_BEFORE_JOIN
};


struct Request {
	//std::string client_name;
	Command command;
	char room_id = 0;
	std::string file_name = "";
	uint32_t file_size = 0;
	uint32_t message_size = 0;
};

bool recvAll(SOCKET socket, char* buffer, int size);
bool sendAll(SOCKET socket, const char* buffer, int size);

