#pragma once
#include <WinSock2.h>
#include <cstdint>

#pragma comment(lib, "ws2_32.lib")

struct ClientInfo {
	SOCKET socket;
	uint8_t room_id = 0;
};

