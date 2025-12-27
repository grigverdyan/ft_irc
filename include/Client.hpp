
#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <stdexcept>

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
	Client(const Client& other);
	Client& operator=(const Client& rhs);
	~Client();

	// Non-blocking I/O helpers
	void queue(const std::string& line);
	bool hasPendingSend() const;
	bool flushSend();
	void appendRecv(const std::string& data);
	bool popLine(std::string& outLine);

public: // Setters
	void setState(ClientState state) { state_ = state; }
    void setNickname(const std::string& nickname);
    void setUsername(const std::string& username) { username_ = username; }
    void setRealname(const std::string& realname) { realname_ = realname; }
    void incrementChannelCount() { channelCount_++; }
    void decrementChannelCount() { channelCount_--; }

public: // Getters
	int getFd() const { return fd_; }
	std::string getHostname() const { return hostname_; }
	std::string getUsername() const { return username_; }
	std::string getRealname() const { return realname_; }
	std::string getNickname() const { return nickname_; }
	ClientState getState() const { return state_; }
	int getChannelCount() const { return channelCount_; }
	std::string getPrefix() const;
	bool hasPassed() const { return hasPassed_; }
	void setPassed(bool v) { hasPassed_ = v; }

private:
    Client();
	// copy operations are public (needed for std::map value)

private:
	int fd_;
	std::string ip_;
	ClientState state_;
	int channelCount_;
	bool hasPassed_;
	
	std::string hostname_;
	std::string username_;
	std::string realname_;
	std::string nickname_;

	std::string recvBuffer_;
	std::string sendBuffer_;

};

#endif /* CLIENT_HPP */