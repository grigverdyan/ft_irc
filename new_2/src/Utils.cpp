#include "Utils.hpp"
#include <cctype>
#include <algorithm>

namespace Utils
{

std::vector<std::string> split(const std::string& str, char delimiter)
{
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    
    while (std::getline(ss, token, delimiter))
    {
        if (!token.empty())
        {
            tokens.push_back(token);
        }
    }
    return tokens;
}

std::vector<std::string> splitCommand(const std::string& message)
{
    std::vector<std::string> tokens;
    std::string current;
    bool inTrailing = false;

    for (size_t i = 0; i < message.length(); ++i)
    {
        char c = message[i];
        
        if (inTrailing)
        {
            current += c;
        }
        else if (c == ':' && current.empty() && !tokens.empty())
        {
            inTrailing = true;
        }
        else if (c == ' ')
        {
            if (!current.empty())
            {
                tokens.push_back(current);
                current.clear();
            }
        }
        else
        {
            current += c;
        }
    }
    
    if (!current.empty())
    {
        tokens.push_back(current);
    }
    
    return tokens;
}

std::string toUpper(const std::string& str)
{
    std::string result = str;
    for (size_t i = 0; i < result.length(); ++i)
    {
        result[i] = std::toupper(static_cast<unsigned char>(result[i]));
    }
    return result;
}

std::string toLower(const std::string& str)
{
    std::string result = str;
    for (size_t i = 0; i < result.length(); ++i)
    {
        result[i] = std::tolower(static_cast<unsigned char>(result[i]));
    }
    return result;
}

bool isValidNickname(const std::string& nick)
{
    if (nick.empty() || nick.length() > 9)
        return false;
    
    char first = nick[0];
    if (!std::isalpha(static_cast<unsigned char>(first)) && 
        first != '[' && first != ']' && first != '\\' && 
        first != '`' && first != '_' && first != '^' && 
        first != '{' && first != '|' && first != '}')
    {
        return false;
    }
    
    for (size_t i = 1; i < nick.length(); ++i)
    {
        char c = nick[i];
        if (!std::isalnum(static_cast<unsigned char>(c)) && 
            c != '[' && c != ']' && c != '\\' && 
            c != '`' && c != '_' && c != '^' && 
            c != '{' && c != '|' && c != '}' && c != '-')
        {
            return false;
        }
    }
    
    return true;
}

bool isValidChannelName(const std::string& name)
{
    if (name.empty() || name.length() > 50)
        return false;
    
    if (name[0] != '#' && name[0] != '&')
        return false;
    
    for (size_t i = 1; i < name.length(); ++i)
    {
        char c = name[i];
        if (c == ' ' || c == ',' || c == '\x07' || c == '\0')
            return false;
    }
    
    return true;
}

std::string intToString(int num)
{
    std::stringstream ss;
    ss << num;
    return ss.str();
}

int stringToInt(const std::string& str)
{
    std::stringstream ss(str);
    int num = 0;
    ss >> num;
    return num;
}

std::string trim(const std::string& str)
{
    size_t start = 0;
    size_t end = str.length();
    
    while (start < end && std::isspace(static_cast<unsigned char>(str[start])))
        ++start;
    
    while (end > start && std::isspace(static_cast<unsigned char>(str[end - 1])))
        --end;
    
    return str.substr(start, end - start);
}

}
