#include "Server.hpp"
#include <cstdlib>
#include <iostream>

static bool isValidPort(const std::string& portStr)
{
    if (portStr.empty())
        return false;
    
    for (size_t i = 0; i < portStr.length(); ++i)
    {
        if (!std::isdigit(portStr[i]))
            return false;
    }
    
    int port = std::atoi(portStr.c_str());
    return port >= 1 && port <= 65535;
}

int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        std::cerr << "Usage: " << argv[0] << " <port> <password>" << std::endl;
        return 1;
    }
    
    std::string portStr = argv[1];
    std::string password = argv[2];
    
    if (!isValidPort(portStr))
    {
        std::cerr << "Error: Invalid port number. Port must be between 1 and 65535." << std::endl;
        return 1;
    }
    
    if (password.empty())
    {
        std::cerr << "Error: Password cannot be empty." << std::endl;
        return 1;
    }
    
    int port = std::atoi(portStr.c_str());
    
    try
    {
        Server server(port, password);
        
        signal(SIGINT, Server::signalHandler);
        signal(SIGQUIT, Server::signalHandler);
        
        std::cout << "IRC Server starting..." << std::endl;
        std::cout << "Port: " << port << std::endl;
        std::cout << "Password: " << password << std::endl;
        std::cout << "Press Ctrl+C to stop the server." << std::endl;
        
        server.run();
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "Server stopped." << std::endl;
    return 0;
}
