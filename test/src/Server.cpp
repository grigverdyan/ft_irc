#include "Server.hpp"
#include "Utils.hpp"

bool Server::_signal = false;

Server::Server(int port, const std::string& password) : _port(port), _password(password), _serverSocket(-1)
{
    initServer();
}

Server::~Server()
{
    // Clean up clients
    for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it)
    {
        close(it->first);
        delete it->second;
    }
    _clients.clear();

    // Clean up channels
    for (std::map<std::string, Channel*>::iterator it = _channels.begin(); it != _channels.end(); ++it)
    {
        delete it->second;
    }
    _channels.clear();

    // Close server socket
    if (_serverSocket != -1)
    {
        close(_serverSocket);
    }
}

void Server::signalHandler(int sig)
{
    (void)sig;
    std::cout << "\nSignal received, shutting down server..." << std::endl;
    _signal = true;
}

void Server::initServer()
{
    // Create socket
    _serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (_serverSocket == -1)
    {
        throw std::runtime_error("Failed to create socket");
    }

    // Set socket options
    int opt = 1;
    if (setsockopt(_serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
    {
        close(_serverSocket);
        throw std::runtime_error("Failed to set socket options");
    }

    // Set socket to non-blocking
    if (fcntl(_serverSocket, F_SETFL, O_NONBLOCK) == -1)
    {
        close(_serverSocket);
        throw std::runtime_error("Failed to set socket to non-blocking");
    }

    // Bind socket
    struct sockaddr_in serverAddr;
    std::memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(_port);

    if (bind(_serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1)
    {
        close(_serverSocket);
        throw std::runtime_error("Failed to bind socket");
    }

    // Listen for connections
    if (listen(_serverSocket, SOMAXCONN) == -1)
    {
        close(_serverSocket);
        throw std::runtime_error("Failed to listen on socket");
    }

    // Add server socket to poll
    struct pollfd serverPollFd;
    serverPollFd.fd = _serverSocket;
    serverPollFd.events = POLLIN;
    serverPollFd.revents = 0;
    _pollFds.push_back(serverPollFd);

    std::cout << "Server started on port " << _port << std::endl;
}

void Server::run()
{
    while (!_signal)
    {
        int pollResult = poll(&_pollFds[0], _pollFds.size(), -1);
        
        if (pollResult == -1)
        {
            if (_signal)
                break;
            throw std::runtime_error("Poll failed");
        }

        for (size_t i = 0; i < _pollFds.size(); ++i)
        {
            if (_pollFds[i].revents & POLLIN)
            {
                if (_pollFds[i].fd == _serverSocket)
                {
                    acceptClient();
                }
                else
                {
                    receiveData(_pollFds[i].fd);
                }
            }
        }
    }
}

void Server::acceptClient()
{
    struct sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);
    
    int clientFd = accept(_serverSocket, (struct sockaddr*)&clientAddr, &clientLen);
    if (clientFd == -1)
    {
        std::cerr << "Failed to accept client connection" << std::endl;
        return;
    }

    // Set client socket to non-blocking
    if (fcntl(clientFd, F_SETFL, O_NONBLOCK) == -1)
    {
        std::cerr << "Failed to set client socket to non-blocking" << std::endl;
        close(clientFd);
        return;
    }

    // Add client to poll
    struct pollfd clientPollFd;
    clientPollFd.fd = clientFd;
    clientPollFd.events = POLLIN;
    clientPollFd.revents = 0;
    _pollFds.push_back(clientPollFd);

    // Create client object
    Client* newClient = new Client(clientFd);
    newClient->setHostname(inet_ntoa(clientAddr.sin_addr));
    _clients[clientFd] = newClient;

    std::cout << "New client connected: " << clientFd << " from " << inet_ntoa(clientAddr.sin_addr) << std::endl;
}

void Server::receiveData(int fd)
{
    char buffer[1024];
    std::memset(buffer, 0, sizeof(buffer));
    
    ssize_t bytesReceived = recv(fd, buffer, sizeof(buffer) - 1, 0);
    
    if (bytesReceived <= 0)
    {
        if (bytesReceived == 0)
        {
            std::cout << "Client " << fd << " disconnected" << std::endl;
        }
        else
        {
            std::cerr << "Error receiving data from client " << fd << std::endl;
        }
        removeClient(fd);
        return;
    }

    Client* client = _clients[fd];
    client->appendToBuffer(std::string(buffer, bytesReceived));

    while (client->hasCompleteMessage())
    {
        std::string message = client->extractMessage();
        if (!message.empty())
        {
            handleClientMessage(fd, message);
        }
    }
}

void Server::handleClientMessage(int fd, const std::string& message)
{
    std::cout << "Received from " << fd << ": " << message << std::endl;
    parseCommand(fd, message);
}

void Server::removeClient(int fd)
{
    // Remove from channels
    Client* client = _clients[fd];
    if (client)
    {
        std::set<std::string> channels = client->getChannels();
        for (std::set<std::string>::iterator it = channels.begin(); it != channels.end(); ++it)
        {
            Channel* channel = getChannel(*it);
            if (channel)
            {
                std::string quitMsg = ":" + client->getPrefix() + " QUIT :Client disconnected\r\n";
                sendToChannel(channel, quitMsg, fd);
                channel->removeClient(fd);
                if (channel->isEmpty())
                {
                    removeChannel(*it);
                }
            }
        }
        delete client;
        _clients.erase(fd);
    }

    // Remove from poll
    for (std::vector<struct pollfd>::iterator it = _pollFds.begin(); it != _pollFds.end(); ++it)
    {
        if (it->fd == fd)
        {
            _pollFds.erase(it);
            break;
        }
    }

    close(fd);
}

void Server::sendToClient(int fd, const std::string& message)
{
    std::cout << "Sending to " << fd << ": " << message;
    if (send(fd, message.c_str(), message.length(), 0) == -1)
    {
        std::cerr << "Failed to send message to client " << fd << std::endl;
    }
}

void Server::sendToChannel(Channel* channel, const std::string& message, int excludeFd)
{
    std::set<int> clients = channel->getClients();
    for (std::set<int>::iterator it = clients.begin(); it != clients.end(); ++it)
    {
        if (*it != excludeFd)
        {
            sendToClient(*it, message);
        }
    }
}

void Server::broadcastToAll(const std::string& message, int excludeFd)
{
    for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it)
    {
        if (it->first != excludeFd)
        {
            sendToClient(it->first, message);
        }
    }
}

Client* Server::getClientByNick(const std::string& nick)
{
    for (std::map<int, Client*>::iterator it = _clients.begin(); it != _clients.end(); ++it)
    {
        if (Utils::toLower(it->second->getNickname()) == Utils::toLower(nick))
        {
            return it->second;
        }
    }
    return NULL;
}

Client* Server::getClientByFd(int fd)
{
    std::map<int, Client*>::iterator it = _clients.find(fd);
    if (it != _clients.end())
    {
        return it->second;
    }
    return NULL;
}

Channel* Server::getChannel(const std::string& name)
{
    std::string lowerName = Utils::toLower(name);
    std::map<std::string, Channel*>::iterator it = _channels.find(lowerName);
    if (it != _channels.end())
    {
        return it->second;
    }
    return NULL;
}

Channel* Server::createChannel(const std::string& name, Client* creator)
{
    std::string lowerName = Utils::toLower(name);
    Channel* channel = new Channel(name);
    channel->addClient(creator->getFd());
    channel->addOperator(creator->getFd());
    _channels[lowerName] = channel;
    creator->addChannel(lowerName);
    return channel;
}

void Server::removeChannel(const std::string& name)
{
    std::string lowerName = Utils::toLower(name);
    std::map<std::string, Channel*>::iterator it = _channels.find(lowerName);
    if (it != _channels.end())
    {
        delete it->second;
        _channels.erase(it);
    }
}

bool Server::isNickInUse(const std::string& nick)
{
    return getClientByNick(nick) != NULL;
}

std::string Server::getPassword() const
{
    return _password;
}
