#ifndef IRC_HPP
#define IRC_HPP

#include <string>
#include <vector>

struct IrcMessage
{
    std::string prefix;
    std::string command;
    std::vector<std::string> params;
};

class Irc
{
public:
    static IrcMessage parseLine(const std::string& line);
    static std::string toUpper(const std::string& s);
};

#endif
