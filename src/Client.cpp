#include "Client.hpp"

#include <iostream>
#include <unistd.h>

Client::Client(int fd, int port)
    : fd_(fd)
    , port_(port)
    , authenticated_(false)
    , state_(DISCONNECTED)
    , channel_(NULL)
{
}

Client::~Client()
{
}

bool Client::checkRegistration() const
{
    return authenticated_ && !username_.empty() && !nickname_.empty() && !realname_.empty();
}

void Client::send(const std::string& message)
{
    std::string msg = message + "\r\n";
    if (write(fd_, msg.c_str(), msg.size()) == -1)
    {
        std::cerr << "Error to send message to client " << fd_ << std::endl;
    }
}