#include "Client.hpp"
#include <iostream>
#include <sys/socket.h>
#include <fstream>

Client::Client(int fd, std::string &ip, std::string &hostname) 
    : fd_(fd)
    , ip_(ip)
    , hostname_(hostname)
    , state_(UNAUTHENTICATED)
    , channelCount_(0)
{}

Client::Client(const Client& other) 
    : fd_(other.fd_)
    , ip_(other.ip_)
    , hostname_(other.hostname_)
    , state_(other.state_)
    , username_(other.username_)
    , realname_(other.realname_)
    , nickname_(other.nickname_)
    , channelCount_(other.channelCount_)
{}

Client::~Client()
{}

Client &Client::operator=(const Client &rhs)
{
	if (this != &rhs)
	{
		fd_ = rhs.fd_;
		ip_ = rhs.ip_;
		hostname_ = rhs.hostname_;
		state_ = rhs.state_;
		nickname_ = rhs.nickname_;
		username_ = rhs.username_;
		realname_= rhs.realname_;
		channelCount_ = rhs.channelCount_;
	}

	return *this;
}

void Client::write(const std::string& msg) const
{
    std::string buffer = msg + "\r\n";
	if (send(fd_, buffer.c_str(), buffer.size(), 0) < 0) 
    {
		throw std::runtime_error("Failed to send message to client");
    }
}

std::string Client::getPrefix() const
{
	std::string username_ = username_.empty() ? "" : "!" + username_;
	std::string hostname_ = hostname_.empty() ? "" : "@" + hostname_;

	return nickname_ + username + hostname;
}

void Client::setNickname(const std::string& nickname) 
{
    // IRCv3 Specification Client nickname constraints
    // https://modern.ircdocs.horse/#clients
    if (std::string::npos != nickname.find_first_of(" ,*?!@.")) {
        throw std::runtime_error("Nickname should not contain none of these chacarcters: \" ,*?!@.\"");
    }
    if (nickname[0] == '$' || nickname[0] == ':') {
        throw std::runtime_error("Nickname should not start with: $ or :");
    }
    nickname_ = nickname; 
}
