#pragma once
#include <string>
#include <WinSock2.h>

#pragma comment(lib, "ws2_32.lib")

const int REQUEST_HEADER_SIZE_LEN = 2;
const int MESSAGE_SIZE_LEN = 4;
const int TAG_LEN = 1;
const int CLIENT_NAME_SIZE_LEN = 4;
const int COMMAND_LEN = 1;
const int ROOM_ID_LEN = 1;
const int FILE_NAME_SIZE_LEN = 2;
const int FILE_SIZE_LEN = 4;

const int CLIENTS_NUM_LEN = 1;
const int ROOMS_NUM_LEN = 1;
const int STATUS_LEN = 1;

const uint8_t min_room_num = 1;
const uint8_t max_room_num = 255;

const int CHUNK_SIZE = 1024;

enum class Tag : uint8_t {
	message,
	command,
	status,
	who_response,
	list_response,
	file_offer,
	file_data
};

enum class Command : uint8_t {
	message,
	join,
	leave,
	list,
	who,
	upload,
	download,
	quit,
	file_accept,
	file_reject,
	unknown
};

enum class Status : uint8_t {
	SUCCESS,
	FILE_NOT_FOUND,
	INVALID_REQUEST,
	INTERNAL_ERROR,
	INVALID_ROOM_NUM,
	CLIENT_HAS_NO_ROOM,
	ALREADY_IN_THIS_ROOM,
	MUST_LEAVE_ROOM_BEFORE_JOIN
};

struct Request {
	SOCKET socket;
	Command command;
	char room_id = 0;
	std::string file_name = "";
	uint32_t file_size = 0;
	uint32_t message_size = 0;
	std::string message;
};

bool recvAll(SOCKET socket, char* buffer, int size);
bool sendAll(SOCKET socket, const char* buffer, int size);

