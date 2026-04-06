#include "CommandHandler.h"
#include <filesystem>
#include <fstream>

void CommandHandler::sendStatus(SOCKET socket, Status st) {
	uint8_t tag = static_cast<uint8_t>(Tag::status);
	uint8_t status = static_cast<uint8_t>(st);

	std::vector<char> status_buf;
	status_buf.push_back(static_cast<char>(tag));
	status_buf.push_back(static_cast<char>(status));
	if (!sendAll(socket, status_buf.data(), TAG_LEN + STATUS_LEN)) {
		return;
	}
}

void CommandHandler::handleJoin(ClientInfo& client, uint8_t room_id) {
	if (room_id < min_room_num || room_id > max_room_num) {
		sendStatus(client.socket, Status::INVALID_ROOM_NUM);
		return;
	}
	if (client.room_id == room_id) {
		sendStatus(client.socket, Status::ALREADY_IN_THIS_ROOM);
		return;
	}
	if (client.room_id != 0) {
		sendStatus(client.socket, Status::MUST_LEAVE_ROOM_BEFORE_JOIN);
		return;
	}

	if (rooms[room_id].getClients().size() == 0) {
		rooms[room_id].initialize();
		rooms[room_id].setID(room_id);
	}
	rooms[room_id].addClient(client);
	sendStatus(client.socket, Status::SUCCESS);
}

void CommandHandler::handleLeave(ClientInfo& client) {
	if (client.room_id == 0) {
		sendStatus(client.socket, Status::CLIENT_HAS_NO_ROOM);
		return;
	}
	rooms[client.room_id].removeClient(client);
	sendStatus(client.socket, Status::SUCCESS);
}

void CommandHandler::handleList(ClientInfo& client) {
	sendStatus(client.socket, Status::SUCCESS);

    uint8_t tag = static_cast<uint8_t>(Tag::list_response);
    if (!sendAll(client.socket, reinterpret_cast<char*>(&tag), TAG_LEN)) {
        return;
    }

	uint8_t rooms_num = static_cast<uint8_t>(rooms.size());
	if (!sendAll(client.socket, reinterpret_cast<char*>(&rooms_num), ROOMS_NUM_LEN)) {
		return;
	}

	for (auto& r : rooms) {
		uint8_t room_id = r.first;
		if (!sendAll(client.socket, reinterpret_cast<char*>(&room_id), sizeof(r.first))) {
			return;
		}
	}
}

void CommandHandler::handleWho(ClientInfo& client) {
	if (client.room_id == 0) {
		sendStatus(client.socket, Status::CLIENT_HAS_NO_ROOM);
		return;
	}

	sendStatus(client.socket, Status::SUCCESS);

	uint8_t tag = static_cast<uint8_t>(Tag::who_response);
	sendAll(client.socket, reinterpret_cast<char*>(&tag), TAG_LEN);

	Room& room = rooms[client.room_id];
	std::vector<ClientInfo*> clients = room.getClients();
	//number of clients in the room without client himself
	uint8_t client_num = static_cast<uint8_t>(clients.size()) - 1;
	if (!sendAll(client.socket, reinterpret_cast<char*>(&client_num), CLIENTS_NUM_LEN)) {
		return;
	}

	for (auto& cl : clients) {
		if (cl->socket != client.socket) {
			SOCKET client_socket = htonll(cl->socket);
			if (!sendAll(client.socket, reinterpret_cast<char*>(&client_socket), sizeof(client_socket))) {
				return;
			}
		}
	}
}

void CommandHandler::handleQuit(ClientInfo& client) {
	if (client.room_id != 0) {
		rooms[client.room_id].removeClient(client);
	}
	closesocket(client.socket);

	sendStatus(client.socket, Status::SUCCESS);
}

void CommandHandler::handleMessage(ClientInfo& client, std::string& message) {
	if (client.room_id == 0) {
		sendStatus(client.socket, Status::CLIENT_HAS_NO_ROOM);
		return;
	}
	rooms[client.room_id].addMessage(client.socket, message);
	sendStatus(client.socket, Status::SUCCESS);
}

void CommandHandler::sendFileOffer(SOCKET receiver, SOCKET sender, const std::string& file_name, uint32_t file_size) {
    std::vector<char> offer;

    uint8_t tag = static_cast<uint8_t>(Tag::file_offer);
    offer.push_back(static_cast<char>(tag));

    SOCKET net_sender = htonll(sender);
    offer.insert(offer.end(), reinterpret_cast<char*>(&net_sender),
        reinterpret_cast<char*>(&net_sender) + sizeof(net_sender));

    uint16_t name_len = htons(static_cast<uint16_t>(file_name.size()));
    offer.insert(offer.end(), reinterpret_cast<char*>(&name_len),
        reinterpret_cast<char*>(&name_len) + sizeof(name_len));
    offer.insert(offer.end(), file_name.begin(), file_name.end());

    uint32_t net_size = htonl(file_size);
    offer.insert(offer.end(), reinterpret_cast<char*>(&net_size),
        reinterpret_cast<char*>(&net_size) + sizeof(net_size));

    if (!sendAll(receiver, offer.data(), offer.size())) {
        return;
    }
}

void CommandHandler::sendFileData(SOCKET receiver, const std::string& file_name, uint32_t file_size) {
    std::vector<char> header;

    uint8_t tag = static_cast<uint8_t>(Tag::file_data);
    header.push_back(static_cast<char>(tag));

    uint16_t name_len = htons(static_cast<uint16_t>(file_name.size()));
    header.insert(header.end(), reinterpret_cast<char*>(&name_len),
        reinterpret_cast<char*>(&name_len) + sizeof(name_len));
    header.insert(header.end(), file_name.begin(), file_name.end());

    uint32_t net_size = htonl(file_size);
    header.insert(header.end(), reinterpret_cast<char*>(&net_size),
        reinterpret_cast<char*>(&net_size) + sizeof(net_size));

    sendAll(receiver, header.data(), header.size());

    std::ifstream file(file_directory + "/" + file_name, std::ios::binary);
    std::vector<char> buffer(CHUNK_SIZE);
    uint32_t total_sent = 0;
    while (total_sent < file_size) {
        uint32_t bytes_to_send = std::min<uint32_t>(CHUNK_SIZE, file_size - total_sent);
        file.read(buffer.data(), bytes_to_send);
        if (!sendAll(receiver, buffer.data(), bytes_to_send)) {
            return;
        }
        total_sent += bytes_to_send;
    }
}

void CommandHandler::handleUpload(ClientInfo& client, const std::string& file_name, const uint32_t file_size) {
    if (client.room_id == 0) {
        sendStatus(client.socket, Status::CLIENT_HAS_NO_ROOM);
        return;
    }

    std::filesystem::create_directories(file_directory);
    std::ofstream file(file_directory + "/" + file_name, std::ios::binary);
    if (!file.is_open()) {
        sendStatus(client.socket, Status::INTERNAL_ERROR);
        return;
    }

    std::vector<char> buffer(CHUNK_SIZE);
    uint32_t total_received = 0;
    while (total_received < file_size) {
        uint32_t bytes_to_read = std::min<uint32_t>(CHUNK_SIZE, file_size - total_received);
        if (!recvAll(client.socket, buffer.data(), bytes_to_read)) {
            return;
        }
        file.write(buffer.data(), bytes_to_read);
        total_received += bytes_to_read;
    }
    file.close();

    std::vector<ClientInfo*> clients = rooms[client.room_id].getClients();

    {
        std::lock_guard<std::mutex> lock(pending_mutex);
        for (auto& cl : clients) {
            if (cl->socket != client.socket) {
                pending_file_responses[cl->socket] = std::nullopt;
                sendFileOffer(cl->socket, client.socket, file_name, file_size);
            }
        }
    }

    for (auto& cl : clients) {
        if (cl->socket != client.socket) {
            std::unique_lock<std::mutex> lock(pending_mutex);
            pending_cv.wait(lock, [&] {
                return pending_file_responses[cl->socket].has_value();
                });
            bool accepted = pending_file_responses[cl->socket].value();
            pending_file_responses.erase(cl->socket);
            lock.unlock();

            if (accepted) {
                sendFileData(cl->socket, file_name, file_size);
            }
        }
    }

    sendStatus(client.socket, Status::SUCCESS);
}

void CommandHandler::handleFileAccept(ClientInfo& client) {
    std::lock_guard<std::mutex> lock(pending_mutex);
    pending_file_responses[client.socket] = true;
    pending_cv.notify_all();
}

void CommandHandler::handleFileReject(ClientInfo& client) {
    std::lock_guard<std::mutex> lock(pending_mutex);
    pending_file_responses[client.socket] = false;
    pending_cv.notify_all();
}

void CommandHandler::handleDownload(ClientInfo& client, const std::string& file_name) {
    std::string path = file_directory + "/" + file_name;
    if (!std::filesystem::exists(path)) {
        sendStatus(client.socket, Status::FILE_NOT_FOUND);
        return;
    }

    uint32_t file_size = static_cast<uint32_t>(std::filesystem::file_size(path));
    sendStatus(client.socket, Status::SUCCESS);
    sendFileData(client.socket, file_name, file_size);
}
