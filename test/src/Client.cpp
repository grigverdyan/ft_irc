#include "Client.hpp"

Client::Client(int fd) : _fd(fd), _authenticated(false), _registered(false), _passOk(false)
{
    _nickname = "*";
    _username = "";
    _realname = "";
    _hostname = "";
    _buffer = "";
}

Client::~Client()
{
}

// Getters
int Client::getFd() const
{
    return _fd;
}

std::string Client::getNickname() const
{
    return _nickname;
}

std::string Client::getUsername() const
{
    return _username;
}

std::string Client::getRealname() const
{
    return _realname;
}

std::string Client::getHostname() const
{
    return _hostname;
}

std::string Client::getBuffer() const
{
    return _buffer;
}

bool Client::isAuthenticated() const
{
    return _authenticated;
}

bool Client::isRegistered() const
{
    return _registered;
}

bool Client::hasPassOk() const
{
    return _passOk;
}

std::string Client::getPrefix() const
{
    return _nickname + "!" + _username + "@" + _hostname;
}

// Setters
void Client::setNickname(const std::string& nickname)
{
    _nickname = nickname;
}

void Client::setUsername(const std::string& username)
{
    _username = username;
}

void Client::setRealname(const std::string& realname)
{
    _realname = realname;
}

void Client::setHostname(const std::string& hostname)
{
    _hostname = hostname;
}

void Client::setAuthenticated(bool value)
{
    _authenticated = value;
}

void Client::setRegistered(bool value)
{
    _registered = value;
}

void Client::setPassOk(bool value)
{
    _passOk = value;
}

// Buffer management
void Client::appendToBuffer(const std::string& data)
{
    _buffer += data;
}

void Client::clearBuffer()
{
    _buffer.clear();
}

bool Client::hasCompleteMessage() const
{
    return _buffer.find("\r\n") != std::string::npos || _buffer.find("\n") != std::string::npos;
}

std::string Client::extractMessage()
{
    size_t pos = _buffer.find("\r\n");
    if (pos == std::string::npos)
    {
        pos = _buffer.find("\n");
    }
    
    if (pos != std::string::npos)
    {
        std::string message = _buffer.substr(0, pos);
        // Skip \r\n or just \n
        if (pos + 1 < _buffer.length() && _buffer[pos] == '\r')
            _buffer = _buffer.substr(pos + 2);
        else
            _buffer = _buffer.substr(pos + 1);
        return message;
    }
    return "";
}

// Channel management
void Client::addChannel(const std::string& channelName)
{
    _channels.insert(channelName);
}

void Client::removeChannel(const std::string& channelName)
{
    _channels.erase(channelName);
}

bool Client::isInChannel(const std::string& channelName) const
{
    return _channels.find(channelName) != _channels.end();
}

std::set<std::string> Client::getChannels() const
{
    return _channels;
}

// Invite management
void Client::addInvite(const std::string& channelName)
{
    _invitedChannels.insert(channelName);
}

void Client::removeInvite(const std::string& channelName)
{
    _invitedChannels.erase(channelName);
}

bool Client::isInvited(const std::string& channelName) const
{
    return _invitedChannels.find(channelName) != _invitedChannels.end();
}
