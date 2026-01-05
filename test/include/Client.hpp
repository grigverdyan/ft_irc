#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <vector>
#include <set>

class Channel;

class Client
{
private:
    int                     _fd;
    std::string             _nickname;
    std::string             _username;
    std::string             _realname;
    std::string             _hostname;
    std::string             _buffer;
    bool                    _authenticated;
    bool                    _registered;
    bool                    _passOk;
    std::set<std::string>   _channels;
    std::set<std::string>   _invitedChannels;

public:
    Client(int fd);
    ~Client();

    // Getters
    int                 getFd() const;
    std::string         getNickname() const;
    std::string         getUsername() const;
    std::string         getRealname() const;
    std::string         getHostname() const;
    std::string         getBuffer() const;
    bool                isAuthenticated() const;
    bool                isRegistered() const;
    bool                hasPassOk() const;
    std::string         getPrefix() const;

    // Setters
    void                setNickname(const std::string& nickname);
    void                setUsername(const std::string& username);
    void                setRealname(const std::string& realname);
    void                setHostname(const std::string& hostname);
    void                setAuthenticated(bool value);
    void                setRegistered(bool value);
    void                setPassOk(bool value);

    // Buffer management
    void                appendToBuffer(const std::string& data);
    void                clearBuffer();
    bool                hasCompleteMessage() const;
    std::string         extractMessage();

    // Channel management
    void                addChannel(const std::string& channelName);
    void                removeChannel(const std::string& channelName);
    bool                isInChannel(const std::string& channelName) const;
    std::set<std::string> getChannels() const;

    // Invite management
    void                addInvite(const std::string& channelName);
    void                removeInvite(const std::string& channelName);
    bool                isInvited(const std::string& channelName) const;
};

#endif
