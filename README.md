# remote_command
Remote command execution client/server


## Building
```sh
(cd src/server && make) 
(cd src/cleint && make)
```

## Usage
To start the server, simply provide a positional parameter specifying a port to listen on.

To connect a client to the server, pass in the following positional parameters
```sh
SERVER_HOSTNAME : Host name of server computer (will convert to ipv6 to connect)
SERVER_PORT     : Port that the server is running on 
# EXECUTIONS    : How many times the command specified should be run on the server
DELAY           : Integer delay in units of seconds
COMMAND         : command in "" that will be executed on the server
```
```sh
# Starting the server to listen for clients
./server <PORT>

# Starting a client connection to server
./client <SERVER_HOSTNAME> <SERVER_PORT> <# EXECUTIONS> <DELAY> "<COMMAND>"
```


