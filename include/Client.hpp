
#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "Channel.hpp"
#include <iostream>

class Channel;

enum ClientState
{
    HANDSHAKE,
	UNAUTHENTICATED,
	AUTHENTICATED,
	REGISTERED,
    DISCONECTED
};

class Client
{
public:
	explicit Client(int fd, std::string& ip, std::string& hostname);
	~Client();

	void write(const std::string &msg) const;

public: // Setters
    void setState(AuthState state) { _state = state; }
    void setNickname(const std::string& nickname);
    void setUsername(const std::string& username) { username_ = username; }
    void setRealname(const std::string& realname) { realname_ = realname; }
    void incrementChannelCount() { channelCount_++; }
    void decrementChannelCount() { channelCount_--; }

public: // Getters
    int getPort() const { return port_ };
	int getFd() const { return fd_; }
	std::string getHostname() const { return hostname_; }
	std::string getUsername() const { return username_; }
	std::string getRealname() const { return realname_; }
	std::string getNickname() const { return nickname_; }
	ClientState getState() const { return state_; }
	int getChannelCount() const { return channelCount_; }
	std::string getPrefix() const;

private:
    Client();
	Client(const Client& other);
	Client& operator=(const Client& rhs);

private:
	int fd_;
	std::string ip_;
	ClientState state_;
	int channelCount_;
	
	std::string hostname_;
	std::string username_;
	std::string realname_;
	std::string nickname_;

};

#endif /* CLIENT_HPP */