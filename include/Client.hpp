#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>

class Channel;

class Client
{
public:
    enum ClientState
    {
        DISCONNECTED,
        CONNECTING
        REGISTERED
    };

public: // Public Constructors and Destructor
    Client(int fd, int port);
    ~Client();

private: // Private Copy Constructor and Assignment Operator
    Client(const Client&) {}
    Client& operator=(const Client&) { return *this; }

public: // Getters and Setters
    bool    isAuthenticated() const { return authenticated_; }
    void    setAuthenticated(bool auth) { authenticated_ = auth; }

    const std::string& getUsername() const { return username_; }
    void    setUsername(const std::string& username) { username_ = username; }
    
    const std::string& getNickname() const { return nickname_; }
    void    setNickname(const std::string& nickname) { nickname_ = nickname; }

    const std::string& getRealname() const { return realname_; }
    void    setRealname(const std::string& realname) { realname_ = realname; }

    int     getPort() const { return port_; }
    int     getFd() const { return fd_; }


    bool    checkRegistration() const;

public: // Client actions
    void    send(const std::string& message);
    void    reply(const std::string& message);

private:
    int     fd_;
    int     port_;
    bool    authenticated_;

    std::string username_;
    std::string nickname_;
    std::string realname_;
    std::string hostname_;

    ClientState state_;
    Channel*    channel_;

};

#endif // CLIENT_HPP