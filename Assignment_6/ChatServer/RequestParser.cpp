#include "RequestParser.h"
#include <WinSock2.h>

Request RequestParser::parseRequest(const std::vector<char>& buffer) {
	Request request;
	size_t offset = 0;

	Tag tag;
	memcpy(&tag, buffer.data() + offset, TAG_LEN);
	if (tag == Tag::command) {
		offset += TAG_LEN;

		request.command = static_cast<Command>(buffer[offset]);
		offset += COMMAND_LEN;

		switch (request.command) {
			case Command::join: {
				request.room_id = buffer[offset];
				break;
			}
			case Command::upload: {
				uint16_t file_name_size;
				memcpy(&file_name_size, buffer.data() + offset, sizeof(uint16_t));
				file_name_size = ntohs(file_name_size);
				offset += sizeof(uint16_t);
				request.file_name = std::string(buffer.data() + offset, file_name_size);
				offset += file_name_size;
				uint32_t file_size;
				memcpy(&file_size, buffer.data() + offset, FILE_SIZE_LEN);
				request.file_size = ntohl(file_size);
				break;
			}
			case Command::download: {
				uint16_t file_name_size;
				memcpy(&file_name_size, buffer.data() + offset, sizeof(uint16_t));
				file_name_size = ntohs(file_name_size);
				offset += sizeof(uint16_t);
				request.file_name = std::string(buffer.data() + offset, file_name_size);
				break;
			}
		}
		
	}
	else if(tag == Tag::message) {
		offset += TAG_LEN;
		uint32_t message_size = 0;
		memcpy(&message_size, buffer.data() + offset, MESSAGE_SIZE_LEN);
		message_size = ntohl(message_size);
		request.message_size = message_size;
		offset += MESSAGE_SIZE_LEN;
		request.message = std::string(buffer.data() + offset, message_size);
		request.command = Command::message;
	}

	return request;
}
