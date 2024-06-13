# local-superchat

A multithreaded LAN chat application written in C, inspired by YouTube Super Chat. Clients on the same local network connect to a host machine and exchange messages in real time.

## How it works

- The **server** listens on port `3000` and spawns a dedicated thread for each connected client
- The **client** auto-discovers the host IP via the `en0` interface and connects to the server
- Messages are broadcast to all other connected clients in real time
- Usernames must be alphanumeric (with underscores), are lowercased automatically, and must be unique

## Requirements

- clang
- POSIX threads (`pthreads`)
- macOS or Linux

## Build

```sh
make
```

This produces `server` and `client` binaries. To clean up:

```sh
make clean
```

## Usage

**On the host machine**, start the server:

```sh
./server
```

**On each client machine** (must be on the same local network), run:

```sh
./client
```

You will be prompted for a username (max 16 characters, letters/numbers/underscores only). Once connected, type messages and press Enter to send.

## Configuration

Edit `common.h` to change defaults:

| Constant             | Default | Description                  |
|----------------------|---------|------------------------------|
| `SERVER_PORT`        | `3000`  | TCP port the server binds to |
| `MAX_USERNAME_LENGTH`| `16`    | Maximum username length       |
| `MAX_MESSAGE_LENGTH` | `128`   | Maximum message length        |
