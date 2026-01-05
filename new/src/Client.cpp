#include "Client.hpp"

Client::Client(int fd) : fd_(fd), authenticated_(false), registered_(false), passOk_(false)
{
    nickname_ = "*";
    username_ = "";
    realname_ = "";
    hostname_ = "";
    buffer_ = "";
}

Client::~Client()
{
}

int Client::getFd() const
{
    return fd_;
}

std::string Client::getNickname() const
{
    return nickname_;
}

std::string Client::getUsername() const
{
    return username_;
}

std::string Client::getRealname() const
{
    return realname_;
}

std::string Client::getHostname() const
{
    return hostname_;
}

std::string Client::getBuffer() const
{
    return buffer_;
}

bool Client::isAuthenticated() const
{
    return authenticated_;
}

bool Client::isRegistered() const
{
    return registered_;
}

bool Client::hasPassOk() const
{
    return passOk_;
}

std::string Client::getPrefix() const
{
    return nickname_ + "!" + username_ + "@" + hostname_;
}

void Client::setNickname(const std::string& nickname)
{
    nickname_ = nickname;
}

void Client::setUsername(const std::string& username)
{
    username_ = username;
}

void Client::setRealname(const std::string& realname)
{
    realname_ = realname;
}

void Client::setHostname(const std::string& hostname)
{
    hostname_ = hostname;
}

void Client::setAuthenticated(bool value)
{
    authenticated_ = value;
}

void Client::setRegistered(bool value)
{
    registered_ = value;
}

void Client::setPassOk(bool value)
{
    passOk_ = value;
}

void Client::appendToBuffer(const std::string& data)
{
    buffer_ += data;
}

void Client::clearBuffer()
{
    buffer_.clear();
}

bool Client::hasCompleteMessage() const
{
    return buffer_.find("\r\n") != std::string::npos || buffer_.find("\n") != std::string::npos;
}

std::string Client::extractMessage()
{
    size_t pos = buffer_.find("\r\n");
    if (pos == std::string::npos)
    {
        pos = buffer_.find("\n");
    }
    
    if (pos != std::string::npos)
    {
        std::string message = buffer_.substr(0, pos);
        if (pos + 1 < buffer_.length() && buffer_[pos] == '\r')
            buffer_ = buffer_.substr(pos + 2);
        else
            buffer_ = buffer_.substr(pos + 1);
        return message;
    }
    return "";
}

void Client::addChannel(const std::string& channelName)
{
    channels_.insert(channelName);
}

void Client::removeChannel(const std::string& channelName)
{
    channels_.erase(channelName);
}

bool Client::isInChannel(const std::string& channelName) const
{
    return channels_.find(channelName) != channels_.end();
}

std::set<std::string> Client::getChannels() const
{
    return channels_;
}

void Client::addInvite(const std::string& channelName)
{
    invitedChannels_.insert(channelName);
}

void Client::removeInvite(const std::string& channelName)
{
    invitedChannels_.erase(channelName);
}

bool Client::isInvited(const std::string& channelName) const
{
    return invitedChannels_.find(channelName) != invitedChannels_.end();
}
