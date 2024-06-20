# c-web-server
`c-web-server` is a blocking web server.It ideally has one worker process per cpu or processor core. 
Connections are handled individually by coroutines.There is no need to create new threads or processes for each connection.

# Features
* Single-threaded, blocking I/O
* Multi-core support with processor affinity
* Facilitate coroutines for fast task switching

# Build from Source

```
$ make
```

# Usage
Start the web server.
```
./tcp_server start
```

Stop the web server.
```
./tcp_server stop
```

