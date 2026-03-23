#include "Room.h"
#include "Protocol_asmt6.h"

void Room::sendMessageToEveryone(RoomMessage roomMessage){
	for (auto& client : clients) {
		if (client->socket != roomMessage.sender_socket) {
			if (!sendAll(client->socket, roomMessage.message.data(), roomMessage.message.size())) {
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
	clients.push_back(&client);
	client.room_id = id;
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
}

std::vector<ClientInfo*>& Room::getClients(){
	return clients;
}
