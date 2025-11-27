# ft_irc

ft_irc is a small, educational IRC server implemented in C++98. It provides basic IRC features (registration, channels, JOIN/PART, PRIVMSG, MODE, KICK, INVITE, QUIT) and is designed for learning and interoperability testing with simple IRC clients.

**Features**
- PASS / NICK / USER registration flow
- Channel management: create channels on JOIN, channel keys (+k), invite-only (+i), topic (+t), user limit (+l)
- JOIN/PART with multi-channel support and positional keys
- PRIVMSG to users and channels
- MODE handling for common channel flags (i, t, k, l, o)
- KICK and INVITE
- Proper broadcasts for JOIN, PART, TOPIC, MODE, KICK, QUIT, and NICK changes
- Graceful shutdown on SIGINT/SIGTERM (sends NOTICE to connected clients)

**Requirements**
- Linux (developed and tested on Linux)
- A C++ compiler supporting C++98 (g++ is used in the Makefile)
- Standard POSIX headers (sockets, poll, etc.)

**Build**
From the repository root:

```sh
make
```

This produces the `ircserv` binary.

**Run**
Usage:

```sh
./ircserv <port> <password>
```

Example:

```sh
./ircserv 6667 secretpass
```

**Quick test with netcat**
Open a terminal and run:

```sh
nc localhost 6667
```

Then type the registration lines (each followed by Enter):

```
PASS secretpass

NICK mynick

USER myuser 0 * :Real Name

JOIN #test
```

You should see the server replies (JOIN broadcast, 332/353/366 replies, etc.).

**Testing with HexChat/other IRC clients**
- Add a new network/connection to `localhost` on port `6667` with the server password.
- Register with NICK/USER or let the client send them automatically.
- Observe JOIN behavior; the server emits numeric replies and broadcasts as RFC-like messages.

**Server shutdown**
- Press Ctrl+C in the server terminal or send SIGTERM to the process; the server will send a `NOTICE` shutdown message to connected clients and then close their connections.

**Code structure**
- `main.cpp` — binary entrypoint and argument parsing
- `Server.hpp/cpp` — accept loop, poll-based multiplexing, graceful shutdown
- `Client.hpp/cpp` — per-connection state and IRC command handlers
- `ClientManager.hpp/cpp` — client lifecycle and lookup helpers
- `Channel.hpp/cpp` — channel state and broadcast helper
- `ChannelManager.hpp/cpp` — map of channels
- `ParsedCommand.hpp/cpp` — parses raw IRC lines into command and params
- `parser.hpp/cpp` — command-line parsing for server port and password

**Notes & limitations**
- This project is educational and not production-ready. It intentionally keeps things simple and uses blocking send() calls in places.
- Not all RFC edge cases or numerics are implemented.
- The server runs single-threaded using `poll()`; `std::atomic` is not used (code is C++98).

**Contributing / Next steps**
- Improve error handling and make send operations non-blocking/buffered
- Add full MODE parsing and persistent channel state
- Add automated tests and example client scripts

**License**
- This repository does not include a license header. Add one if you want to publish it.
