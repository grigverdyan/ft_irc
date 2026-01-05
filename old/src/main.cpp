#include "Server.hpp"

#include <iostream>
#include <stdexcept>

static bool isValidPort(const std::string& s)
{
    if (s.empty())
        return false;
    for (size_t i = 0; i < s.size(); ++i)
        if (s[i] < '0' || s[i] > '9')
            return false;
    int p = 0;
    for (size_t i = 0; i < s.size(); ++i)
        p = p * 10 + (s[i] - '0');
    return p > 0 && p <= 65535;
}

int main(int ac, char** av)
{
    try
    {
        if (ac != 3)
        {
            std::cerr << "Usage: " << av[0] << " <port> <password>" << std::endl;
            return 1;
        }
        std::string port(av[1]);
        std::string pass(av[2]);
        if (!isValidPort(port))
        {
            std::cerr << "Invalid port" << std::endl;
            return 1;
        }
        Server s(port, pass);
        s.run();
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Fatal: " << e.what() << std::endl;
        return 1;
    }
}
