#include "Room.h"
#include "Protocol_asmt6.h"

void Room::sendMessageToEveryone(RoomMessage roomMessage){
	for (auto& client : clients) {
		if (client->socket != roomMessage.sender_socket) {
			std::vector<char> message;

			uint8_t tag = static_cast<uint8_t>(Tag::message);
			message.push_back(static_cast<char>(tag));

			SOCKET sender_socket = htonll(roomMessage.sender_socket);
			message.insert(message.end(), reinterpret_cast<char*>(&sender_socket),
				reinterpret_cast<char*>(&sender_socket) + sizeof(sender_socket));

			uint32_t size = htonl(static_cast<uint32_t>(roomMessage.message.size()));
			message.insert(message.end(), reinterpret_cast<char*>(&size),
				reinterpret_cast<char*>(&size) + MESSAGE_SIZE_LEN);

			message.insert(message.end(), roomMessage.message.begin(), roomMessage.message.end());

			if (!sendAll(client->socket, message.data(), message.size())) {
				return;
			}
		}
	}
}

void Room::initialize() {
	broadcast_thread = std::thread(&Room::broadcastMessage, this);
}

void Room::broadcastMessage(){
	while (true) {
		std::unique_lock<std::mutex> lock(mutex);
		cv.wait(lock, [this]{
			return !message_queue.empty();
		});

		RoomMessage roomMessage = message_queue.front();
		message_queue.pop();

		sendMessageToEveryone(roomMessage);
	}
}

void Room::addClient(ClientInfo& client){
	client.room_id = id;
	clients.push_back(&client);
}

void Room::removeClient(ClientInfo& client){
	for (auto it = clients.begin(); it != clients.end(); ) {
		if ((**it).socket == client.socket) {
			it = clients.erase(it);
		}
		else {
			++it;
		}
	}
	client.room_id = 0;
}

void Room::addMessage(SOCKET socket, std::string& message){
	RoomMessage roomMessage;
	roomMessage.sender_socket = socket;
	roomMessage.message = message;

	std::lock_guard<std::mutex> lock(mutex);
	message_queue.push(roomMessage);

	cv.notify_one();
}

std::vector<ClientInfo*>& Room::getClients(){
	return clients;
}

void Room::setID(uint8_t room_id){
	id = room_id;
}
