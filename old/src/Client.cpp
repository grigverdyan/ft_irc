#include "Client.hpp"
#include <sys/socket.h>
#include <errno.h>

Client::Client(int fd, std::string &ip, std::string &hostname) 
    : fd_(fd)
    , ip_(ip)
    , state_(UNAUTHENTICATED)
    , channelCount_(0)
    , hasPassed_(false)
    , hostname_(hostname)
    , recvBuffer_()
    , sendBuffer_()
{}

Client::Client(const Client& other) 
    : fd_(other.fd_)
    , ip_(other.ip_)
    , state_(other.state_)
    , channelCount_(other.channelCount_)
    , hasPassed_(other.hasPassed_)
    , hostname_(other.hostname_)
    , username_(other.username_)
    , realname_(other.realname_)
    , nickname_(other.nickname_)
    , recvBuffer_(other.recvBuffer_)
    , sendBuffer_(other.sendBuffer_)
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

void Client::queue(const std::string& line)
{
    sendBuffer_ += line;
    sendBuffer_ += "\r\n";
}

bool Client::hasPendingSend() const
{
    return !sendBuffer_.empty();
}

bool Client::flushSend()
{
    while (!sendBuffer_.empty())
    {
        ssize_t n = ::send(fd_, sendBuffer_.c_str(), sendBuffer_.size(), 0);
        if (n > 0)
        {
            sendBuffer_.erase(0, static_cast<size_t>(n));
            continue;
        }
        if (n == 0)
            return false;
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return true;
        return false;
    }
    return true;
}

void Client::appendRecv(const std::string& data)
{
    recvBuffer_ += data;
}

bool Client::popLine(std::string& outLine)
{
    std::string::size_type pos = recvBuffer_.find("\n");
    if (pos == std::string::npos)
        return false;
    outLine = recvBuffer_.substr(0, pos);
    recvBuffer_.erase(0, pos + 1);
    if (!outLine.empty() && outLine[outLine.size() - 1] == '\r')
        outLine.erase(outLine.size() - 1);
    return true;
}

std::string Client::getPrefix() const
{
    std::string u = username_.empty() ? "" : "!" + username_;
    std::string h = hostname_.empty() ? "" : "@" + hostname_;
    return nickname_ + u + h;
}

void Client::setNickname(const std::string& nickname) 
{
    // IRCv3 Specification Client nickname constraints
    // https://modern.ircdocs.horse/#clients
    if (std::string::npos != nickname.find_first_of(" ,*?!@.")) {
        throw std::runtime_error("Nickname should not contain none of these chacarcters: \" ,*?!@.\"");
    }
        if (nickname.empty()) {
		throw std::runtime_error("Nickname cannot be empty");
	}
        if (nickname[0] == '$' || nickname[0] == ':') {
        throw std::runtime_error("Nickname should not start with: $ or :");
    }
    nickname_ = nickname; 
}
