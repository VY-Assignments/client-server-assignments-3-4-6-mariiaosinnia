#include "RequestBuilder.h"
#include <WinSock2.h>

std::vector<char> RequestBuilder::serializeClientSocket(SOCKET socket)
{
    std::vector<char> client_socket_buf;
    client_socket_buf.insert(client_socket_buf.end(), reinterpret_cast<char*>(&socket),
        reinterpret_cast<char*>(&socket) + sizeof(socket));
    return client_socket_buf;
}

std::vector<char> RequestBuilder::serializeTag(Tag tag)
{
    std::vector<char> tag_buf;
    tag_buf.push_back(static_cast<char>(tag));
    return tag_buf;
}


std::vector<char> RequestBuilder::serializeCommand(Command command)
{
    std::vector<char> command_buf;
    command_buf.push_back(static_cast<char>(command));
    return command_buf;
}

std::vector<char> RequestBuilder::serializeRoomID(uint8_t room_id)
{
    std::vector<char> room_id_buf;
    room_id_buf.push_back(room_id);
    return room_id_buf;
}

std::vector<char> RequestBuilder::serializeFileName(const std::string& file_name)
{
    std::vector<char> file_name_buf;
    uint16_t file_name_length = static_cast<uint16_t>(file_name.length());
    uint16_t net_file_name_length = htons(file_name_length);
    file_name_buf.insert(file_name_buf.end(), reinterpret_cast<char*>(&net_file_name_length),
        reinterpret_cast<char*>(&net_file_name_length) + FILE_NAME_SIZE_LEN);

    file_name_buf.insert(file_name_buf.end(), file_name.begin(), file_name.end());
    return file_name_buf;
}

std::vector<char> RequestBuilder::serializeFileSize(uint32_t file_size)
{
    std::vector<char> file_size_buf;
    uint32_t net_file_size = htonl(file_size);
    file_size_buf.insert(file_size_buf.end(), reinterpret_cast<char*>(&net_file_size),
        reinterpret_cast<char*>(&net_file_size) + FILE_SIZE_LEN);
    return file_size_buf;
}

std::vector<char> RequestBuilder::buildJoinRequest(const uint8_t room_id)
{
    std::vector<char> request;

    std::vector<char> tag = serializeTag(Tag::command);
    request.insert(request.end(), tag.begin(), tag.end());

    std::vector<char> command = serializeCommand(Command::join);
    request.insert(request.end(), command.begin(), command.end());

    request.push_back(room_id);

    return request;
}

std::vector<char> RequestBuilder::buildLeaveRequest()
{
    std::vector<char> request;

    std::vector<char> tag = serializeTag(Tag::command);
    request.insert(request.end(), tag.begin(), tag.end());

    std::vector<char> command = serializeCommand(Command::leave);
    request.insert(request.end(), command.begin(), command.end());

    return request;
}

std::vector<char> RequestBuilder::buildListRequest()
{
    std::vector<char> request;

    std::vector<char> tag = serializeTag(Tag::command);
    request.insert(request.end(), tag.begin(), tag.end());

    std::vector<char> command = serializeCommand(Command::list);
    request.insert(request.end(), command.begin(), command.end());

    return request;
}

std::vector<char> RequestBuilder::buildWhoRequest()
{
    std::vector<char> request;

    std::vector<char> tag = serializeTag(Tag::command);
    request.insert(request.end(), tag.begin(), tag.end());

    std::vector<char> command = serializeCommand(Command::who);
    request.insert(request.end(), command.begin(), command.end());

    return request;
}

std::vector<char> RequestBuilder::buildMessage(const std::string& message)
{
    std::vector<char> request;

    std::vector<char> tag = serializeTag(Tag::message);
    request.insert(request.end(), tag.begin(), tag.end());

    uint32_t message_size = static_cast<uint32_t>(message.size());
    message_size = htonl(message_size);
    request.insert(request.end(), reinterpret_cast<char*>(&message_size),
        reinterpret_cast<char*>(&message_size) + MESSAGE_SIZE_LEN);

    request.insert(request.end(), message.begin(), message.end());

    return request;
}

std::vector<char> RequestBuilder::buildUploadRequest(const std::string& file_name, const uint32_t file_size)
{
    std::vector<char> request;

    std::vector<char> tag = serializeTag(Tag::command);
    request.insert(request.end(), tag.begin(), tag.end());

    std::vector<char> command = serializeCommand(Command::upload);
    request.insert(request.end(), command.begin(), command.end());

    std::vector<char> file_name_serialized = serializeFileName(file_name);
    request.insert(request.end(), file_name_serialized.begin(), file_name_serialized.end());

    std::vector<char> file_size_serialized = serializeFileSize(file_size);
    request.insert(request.end(), file_size_serialized.begin(), file_size_serialized.end());

    return request;
}

std::vector<char> RequestBuilder::buildQuitRequest()
{
    std::vector<char> request;

    std::vector<char> tag = serializeTag(Tag::command);
    request.insert(request.end(), tag.begin(), tag.end());

    std::vector<char> command = serializeCommand(Command::quit);
    request.insert(request.end(), command.begin(), command.end());

    return request;
}

std::vector<char> RequestBuilder::buildFileAcceptRequest() {
    std::vector<char> request;

    std::vector<char> tag = serializeTag(Tag::command);
    request.insert(request.end(), tag.begin(), tag.end());

    std::vector<char> command = serializeCommand(Command::file_accept);
    request.insert(request.end(), command.begin(), command.end());

    return request;
}

std::vector<char> RequestBuilder::buildFileRejectRequest() {
    std::vector<char> request;

    std::vector<char> tag = serializeTag(Tag::command);
    request.insert(request.end(), tag.begin(), tag.end());

    std::vector<char> command = serializeCommand(Command::file_reject);
    request.insert(request.end(), command.begin(), command.end());

    return request;
}

std::vector<char> RequestBuilder::buildDownloadRequest(const std::string& file_name) {
    std::vector<char> request;

    std::vector<char> tag = serializeTag(Tag::command);
    request.insert(request.end(), tag.begin(), tag.end());

    std::vector<char> command = serializeCommand(Command::download);
    request.insert(request.end(), command.begin(), command.end());

    std::vector<char> name = serializeFileName(file_name);
    request.insert(request.end(), name.begin(), name.end());

    return request;
}
