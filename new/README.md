# ft_irc - Internet Relay Chat Server

A lightweight IRC (Internet Relay Chat) server implementation in C++98, supporting multiple simultaneous connections with non-blocking I/O.

## Table of Contents
- [Features](#features)
- [Requirements](#requirements)
- [Installation](#installation)
- [Usage](#usage)
- [Makefile Commands](#makefile-commands)
- [Testing with nc](#testing-with-nc)
- [IRC Commands Reference](#irc-commands-reference)
- [Channel Modes](#channel-modes)
- [Architecture](#architecture)

---

## Features

✅ Non-blocking I/O with `poll()`  
✅ Multiple simultaneous client connections  
✅ Password-protected server  
✅ Channel management (create, join, part)  
✅ Private messaging (user-to-user and channel)  
✅ Channel operator privileges  
✅ Channel modes (invite-only, topic protection, password, user limit, operators)  
✅ User authentication system  
✅ IRC protocol compliant  

---

## Requirements

- C++ compiler with C++98 standard support (g++, clang++)
- Make
- Unix-like operating system (Linux, macOS)
- netcat (nc) for testing (optional)
- IRC client (irssi, WeeChat, HexChat, etc.) for full experience

---

## Installation

### Clone and Build

```bash
# Navigate to project directory
cd ft_irc

# Compile the project
make

# The executable 'ircserv' will be created
```

---

## Usage

### Starting the Server

```bash
./ircserv <port> <password>
```

**Parameters:**
- `port`: Port number (1-65535) on which the server will listen
- `password`: Connection password required for all clients

**Example:**
```bash
./ircserv 6667 mypassword123
```

**Output:**
```
Server started on port 6667
IRC Server starting...
Port: 6667
Password: mypassword123
Press Ctrl+C to stop the server.
```

### Stopping the Server

Press `Ctrl+C` to gracefully shut down the server.

---

## Makefile Commands

| Command | Description |
|---------|-------------|
| `make` or `make all` | Compiles the project and creates the `ircserv` executable |
| `make clean` | Removes object files (`obj/` directory) |
| `make fclean` | Removes object files and the executable |
| `make re` | Performs `fclean` then `all` (full recompilation) |

### Compilation Details

- **Compiler:** `c++`
- **Flags:** `-Wall -Wextra -Werror -std=c++98`
- **Source Files:** `src/*.cpp`
- **Header Files:** `include/*.hpp`
- **Object Files:** `obj/*.o`

---

## Testing with nc

`nc` (netcat) is a simple tool to test your IRC server without an IRC client.

### Basic Connection Test

**Terminal 1: Start the server**
```bash
./ircserv 6667 test123
```

**Terminal 2: Connect with nc**
```bash
nc localhost 6667
```

Then type commands (press Enter after each):
```irc
PASS test123
NICK alice
USER alice 0 * :Alice Smith
JOIN #general
PRIVMSG #general :Hello from nc!
QUIT
```

### Expected Response Flow

```
# After PASS, NICK, and USER:
:ft_irc 001 alice :Welcome to the Internet Relay Network alice!alice@hostname
:ft_irc 002 alice :Your host is ft_irc, running version 1.0
:ft_irc 003 alice :This server was created today
:ft_irc 004 alice ft_irc 1.0 o itkol

# After JOIN #general:
:alice!alice@hostname JOIN #general
:ft_irc 331 alice #general :No topic is set
:ft_irc 353 alice = #general :@alice
:ft_irc 366 alice #general :End of /NAMES list

# After PRIVMSG (seen by other users in channel):
:alice!alice@hostname PRIVMSG #general :Hello from nc!
```

### Multi-Client Test with nc

**Terminal 1: Server**
```bash
./ircserv 6667 pass123
```

**Terminal 2: First client (Alice)**
```bash
nc localhost 6667
PASS pass123
NICK alice
USER alice 0 * :Alice
JOIN #test
```

**Terminal 3: Second client (Bob)**
```bash
nc localhost 6667
PASS pass123
NICK bob
USER bob 0 * :Bob
JOIN #test
```

**Terminal 2 (Alice): Send message**
```
PRIVMSG #test :Hi Bob!
```

**Terminal 3 (Bob): Will receive**
```
:alice!alice@hostname PRIVMSG #test :Hi Bob!
```

---

## IRC Commands Reference

### Authentication Commands (Must be sent in order)

#### 1. PASS - Server Password
Authenticates with the server. **Must be the first command.**

**Syntax:**
```
PASS <password>
```

**Example:**
```
PASS mypassword123
```

**Notes:**
- Must match the server password
- Required before NICK and USER
- Cannot be changed after registration

---

#### 2. NICK - Set Nickname
Sets or changes your nickname.

**Syntax:**
```
NICK <nickname>
```

**Example:**
```
NICK alice
```

**Rules:**
- Must be unique on the server
- Valid characters: letters, numbers, `_`, `-`, `[`, `]`, `{`, `}`, `|`, `\`
- Cannot start with a number
- Can be changed after registration (notifies all shared channels)

---

#### 3. USER - Set User Information
Sets username and real name. Required for registration.

**Syntax:**
```
USER <username> <mode> <unused> :<realname>
```

**Example:**
```
USER alice 0 * :Alice Smith
```

**Parameters:**
- `username`: Your username
- `mode`: User mode (use `0`)
- `unused`: Placeholder (use `*`)
- `realname`: Your full name (must start with `:`)

**Notes:**
- Cannot be changed after registration
- After both NICK and USER are set, registration completes

---

### Channel Commands

#### 4. JOIN - Join Channel
Joins one or more channels. Creates channel if it doesn't exist.

**Syntax:**
```
JOIN <channel>[,<channel2>,...] [<key>[,<key2>,...]]
```

**Examples:**
```
JOIN #general
JOIN #private secretkey
JOIN #chan1,#chan2,#chan3
JOIN #secure secret123
```

**Channel Restrictions:**
- `+i`: Invite-only (must be invited)
- `+k`: Key-protected (must provide password)
- `+l`: User limit (channel must not be full)

**Notes:**
- Channel names must start with `#` or `&`
- First user to join becomes channel operator (`@`)
- All channel members are notified

---

#### 5. PART - Leave Channel
Leaves one or more channels.

**Syntax:**
```
PART <channel>[,<channel2>,...] [:<reason>]
```

**Examples:**
```
PART #general
PART #test :Going to sleep
PART #chan1,#chan2 :Leaving
```

---

#### 6. TOPIC - View/Set Channel Topic
Views or changes the channel topic.

**Syntax:**
```
TOPIC <channel>              (view)
TOPIC <channel> :<new topic> (set)
```

**Examples:**
```
TOPIC #general
TOPIC #general :Welcome to General Chat!
```

**Requirements:**
- Must be in the channel
- Setting topic requires operator if `+t` mode is set

---

#### 7. MODE - Channel Modes
Views or changes channel modes.

**Syntax:**
```
MODE <channel>                (view modes)
MODE <channel> <+/-modes> [params]
```

**Examples:**
```
MODE #general
MODE #private +i
MODE #secure +k secretpass
MODE #chat +o alice
MODE #chat -o bob
MODE #test +l 50
MODE #room +it
MODE #secure -k
```

**Requirements:**
- Must be channel operator to change modes

---

#### 8. KICK - Remove User from Channel
Kicks a user from a channel. **Operator only.**

**Syntax:**
```
KICK <channel> <nickname> [:<reason>]
```

**Examples:**
```
KICK #general bob
KICK #test alice :Violating rules
```

**Requirements:**
- Must be channel operator
- Target must be in channel

---

#### 9. INVITE - Invite User to Channel
Invites a user to join a channel.

**Syntax:**
```
INVITE <nickname> <channel>
```

**Examples:**
```
INVITE alice #private
INVITE bob #vip
```

**Requirements:**
- Must be in the channel
- For `+i` channels, must be operator
- Target bypasses invite-only restriction

---

### Messaging Commands

#### 10. PRIVMSG - Send Message
Sends a message to a user or channel.

**Syntax:**
```
PRIVMSG <target> :<message>
```

**Examples:**
```
PRIVMSG #general :Hello everyone!
PRIVMSG alice :Hi Alice!
```

**Target Types:**
- `#channel`: Sends to all channel users (except you)
- `nickname`: Private message to user

---

### Information Commands

#### 11. WHO - List Channel Members
Lists users in a channel.

**Syntax:**
```
WHO <channel>
```

**Example:**
```
WHO #general
```

**Response includes:**
- Username, hostname
- Nickname
- Flags (H=here, @=operator)
- Real name

---

### Connection Commands

#### 12. QUIT - Disconnect
Disconnects from the server.

**Syntax:**
```
QUIT [:<reason>]
```

**Examples:**
```
QUIT
QUIT :Goodbye!
```

**Notes:**
- Notifies all channels you're in
- Cleanly closes connection

---

## Channel Modes

| Mode | Symbol | Name | Description | Parameter |
|------|--------|------|-------------|-----------|
| **i** | +i | Invite-only | Only invited users can join | None |
| **t** | +t | Topic protection | Only operators can change topic | None |
| **k** | +k | Channel key | Password required to join | Password |
| **o** | +o | Operator | Give/remove operator privilege | Nickname |
| **l** | +l | User limit | Maximum number of users | Number |

### Mode Examples

```irc
MODE #private +i           # Make invite-only
MODE #secure +k pass123    # Set password
MODE #chat +o alice        # Make alice operator
MODE #chat -o bob          # Remove bob's operator
MODE #test +l 50           # Set user limit to 50
MODE #room +it             # Combine modes
MODE #secure -k            # Remove password
MODE #test -l              # Remove user limit
```

---

## Architecture

### Design Pattern
- **Non-blocking I/O**: All sockets use `O_NONBLOCK` flag
- **Event-driven**: Single `poll()` call monitors all file descriptors
- **No forking**: All clients handled in a single process

### Key Components

**Server Class**
- Manages all connections
- Handles incoming/outgoing data
- Routes commands to handlers

**Client Class**
- Stores client state
- Manages authentication
- Tracks joined channels

**Channel Class**
- Manages channel members
- Enforces channel modes
- Handles operator privileges

### Protocol Flow

```
Client Connect → Accept → Non-blocking socket
                           ↓
                    Add to poll() list
                           ↓
            PASS → NICK → USER (Registration)
                           ↓
                   Registered Client
                           ↓
          JOIN, PRIVMSG, MODE, etc. (Commands)
                           ↓
                    QUIT → Close
```

---

## Complete Connection Example

```bash
# Start server
./ircserv 6667 pass123

# Connect with nc
nc localhost 6667

# Authentication sequence
PASS pass123
NICK alice
USER alice 0 * :Alice Smith

# Join channel
JOIN #general

# Send message
PRIVMSG #general :Hello everyone!

# Set topic (as operator)
TOPIC #general :Welcome to the channel!

# Make channel private
MODE #general +i

# Invite someone
INVITE bob #general

# Check users
WHO #general

# Leave
PART #general :Goodbye!

# Disconnect
QUIT :See you later!
```

---

## Command Summary

| Command | Purpose | Auth Required |
|---------|---------|---------------|
| PASS | Authenticate | No |
| NICK | Set nickname | No |
| USER | Set user info | No |
| JOIN | Join channel | Yes |
| PART | Leave channel | Yes |
| PRIVMSG | Send message | Yes |
| TOPIC | View/set topic | Yes |
| MODE | Channel modes | Yes |
| KICK | Remove user | Yes (operator) |
| INVITE | Invite user | Yes |
| WHO | List users | Yes |
| QUIT | Disconnect | Yes |

---

## Error Handling

The server implements standard IRC numeric replies for errors:

- `401`: No such nick/channel
- `403`: No such channel
- `404`: Cannot send to channel
- `421`: Unknown command
- `431`: No nickname given
- `432`: Erroneous nickname
- `433`: Nickname in use
- `441`: User not in channel
- `442`: Not on channel
- `451`: Not registered
- `461`: Not enough parameters
- `462`: Already registered
- `464`: Password mismatch
- `471`: Channel is full
- `473`: Invite only
- `475`: Bad channel key
- `482`: You're not channel operator

---

## Testing with IRC Clients

### Using irssi

```bash
irssi
/connect localhost 6667 pass123
/nick alice
/join #general
/msg #general Hello!
/quit
```

### Using WeeChat

```bash
weechat
/server add local localhost/6667 -password=pass123
/connect local
/nick alice
/join #general
/msg #general Hello!
/quit
```

---

## Project Structure

```
.
├── Makefile              # Build configuration
├── README.md             # This file
├── include/              # Header files
│   ├── Channel.hpp       # Channel class
│   ├── Client.hpp        # Client class
│   ├── Server.hpp        # Server class
│   └── Utils.hpp         # Utility functions & IRC codes
├── src/                  # Source files
│   ├── Channel.cpp       # Channel implementation
│   ├── Client.cpp        # Client implementation
│   ├── Commands.cpp      # Command handlers
│   ├── Server.cpp        # Server implementation
│   ├── Utils.cpp         # Utility implementations
│   └── main.cpp          # Entry point
└── obj/                  # Object files (generated)
```

---

## License

This project is part of the 42 School curriculum.

---

## Authors

Developed as part of the ft_irc project at 42 School.

---

## Additional Notes

- The server listens on all network interfaces (0.0.0.0)
- Maximum connections limited by system resources
- Commands are case-insensitive
- Channel names are case-insensitive
- Nicknames are case-insensitive
- The server implements a subset of the IRC protocol (RFC 1459/2812)
- UTF-8 characters are supported in messages and topics

---

**Happy IRCing! 🚀**
