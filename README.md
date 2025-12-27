# ft_irc (mandatory part)

This is a minimal IRC server implementation in **C++98**.

## Build

```sh
make
```

## Run

```sh
./ircserv <port> <password>
```

Example:

```sh
./ircserv 6667 secret
```

## Supported (mandatory) features

- Multiple clients with a **single `poll()`** loop
- **Non-blocking** sockets (no fork)
- Authentication and registration: `PASS`, `NICK`, `USER`
- Channels: `JOIN`
- Messaging: `PRIVMSG` (to user or `#channel`)
- Channel operators and operator commands:
  - `KICK`
  - `INVITE`
  - `TOPIC`
  - `MODE` with flags: `i`, `t`, `k`, `o`, `l`

## Manual fragmentation test (from the subject)

In a separate terminal:

```sh
nc -C 127.0.0.1 6667
```

Type commands in parts (use Ctrl-D to flush partial sends), for example:

- `PA` Ctrl-D `SS secret\r\n`
- `NI` Ctrl-D `CK a\r\n`

The server aggregates `recv()` fragments until a full line is received.

## Notes

- This is *not* a full RFC-complete IRC server.
- The goal is to be compatible enough to use a real IRC client for evaluation.
