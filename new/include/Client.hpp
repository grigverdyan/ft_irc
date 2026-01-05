#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <vector>
#include <set>

class Channel;

class Client
{
private:
    int                     fd_;
    std::string             nickname_;
    std::string             username_;
    std::string             realname_;
    std::string             hostname_;
    std::string             buffer_;
    bool                    authenticated_;
    bool                    registered_;
    bool                    passOk_;
    std::set<std::string>   channels_;
    std::set<std::string>   invitedChannels_;

public:
    Client(int fd);
    ~Client();

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

    void                setNickname(const std::string& nickname);
    void                setUsername(const std::string& username);
    void                setRealname(const std::string& realname);
    void                setHostname(const std::string& hostname);
    void                setAuthenticated(bool value);
    void                setRegistered(bool value);
    void                setPassOk(bool value);

    void                appendToBuffer(const std::string& data);
    void                clearBuffer();
    bool                hasCompleteMessage() const;
    std::string         extractMessage();

    void                addChannel(const std::string& channelName);
    void                removeChannel(const std::string& channelName);
    bool                isInChannel(const std::string& channelName) const;
    std::set<std::string> getChannels() const;

    void                addInvite(const std::string& channelName);
    void                removeInvite(const std::string& channelName);
    bool                isInvited(const std::string& channelName) const;
};

#endif
