#include "RequestParser.h"
#include <WinSock2.h>

Request RequestParser::parseRequest(const std::vector<char>& buffer) {
	Request request;
	size_t offset = 0;
	
	char prefix;
	memcpy(&prefix, buffer.data() + offset, PREFIX_LEN);
	if (prefix == '/') {
		offset += PREFIX_LEN;

		request.command = static_cast<Command>(buffer[offset]);
		offset += COMMAND_LEN;

		switch (request.command) {
			case Command::join: {
				request.room_id = buffer[offset];
				break;
			}
			case Command::upload:
			case Command::download: {
				char file_name_size = buffer[offset];
				offset += FILE_NAME_SIZE_LEN;

				std::string file_name(buffer.data() + offset, file_name_size);
				offset += file_name_size;

				uint32_t file_size;
				memcpy(&file_size, buffer.data() + offset, FILE_SIZE_LEN);
				file_size = htonl(file_size);
				request.file_size = file_size;
			}
		}
		
	}
	else {
		uint32_t message_size = 0;
		memcpy(&message_size, buffer.data() + offset, MESSAGE_SIZE_LEN);
		message_size = htonl(message_size);
		request.message_size = message_size;
	}

	return request;
}
