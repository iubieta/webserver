# SERVER CORE

	To develop the Server Core class, these are the essential attributes and methods:

##  Required Attributes

    Must store the status of the ports, active connections, and parsed configuration: Configuration structures: An object containing the data extracted from the .conf file (ports, host, root directory, client_max_body_size, etc.).
    
	Listening sockets: A collection (such as a std::vector<int>) of the file descriptors for sockets that are in passive mode, waiting for clients on each configured port.
    Multiplexing structure: An array or vector of pollfd structures (if using poll) or an fd_set (if using select) to monitor all descriptors simultaneously.
    
	Client Map: A std::map that associates each file descriptor (int fd) with its processing state, including its HttpRequest and Response instances.
    Temporary Buffers: Memory spaces for storing raw bytes received before passing them to the parser.

## Essential Methods

    The methods must cover everything from network setup to data flow:

    **Initialization and Configuration**
    setupServers(): Performs the sequence of system calls for each port: socket() (create the endpoint), bind() (assign IP/port), and listen() (activate listen mode).
    setNonBlocking(int fd): Calls fcntl() with the O_NONBLOCK flag to ensure the server does not hang while reading or writing.
    
**Event Loop (Core Engine)**
    
    run(): Contains the main loop where the single permitted call to poll() (or equivalent) is executed to monitor read (POLLIN) and write (POLLOUT) events.
    handleEvents(): Analyzes the results returned by multiplexing to decide whether to accept a new connection or process data from an existing one.
    
**Connection Management**

    acceptConnection(int listen_fd): Uses accept() to create a new communication socket for the incoming client and add it to the monitoring structure (pollfd).
    receiveRequest(int client_fd): Uses recv() or read() to retrieve raw bytes and passes them to the HttpRequest object via its feed() method.
    
	sendResponse(int client_fd): Once the Response Builder has the response ready, it uses send() or write() to transfer the bytes to the client’s socket.
    closeConnection(int client_fd): Closes the descriptor with close(), frees the memory of the associated objects, and removes it from the monitoring loop.
    
	**Dynamic Content Support (CGI)**

    executeCGI(): This is the only part where you’ll use fork() and execve() to launch external processes, managing the pipes needed for data input and output

# Purpose of the Server Core

    Its main purpose is to manage the low-level network infrastructure and coordinate the flow of data between clients and the server’s internal components (Parser and Builder). It is responsible for ensuring the server’s resilience, allowing it to handle multiple simultaneous connections without blocking through the use of I/O multiplexing.

**Functions Performed**
    
	Socket Management: Handles the creation (socket), binding (bind), listening (listen), and acceptance (accept) of TCP connections.
    Input/Output Multiplexing: Runs a single event loop (typically using poll(), select(), or epoll()) to monitor multiple file descriptors simultaneously.
    Non-Blocking Configuration: Marks sockets as non-blocking (using `fcntl` with `O_NONBLOCK`) to ensure that operations such as `recv()` or `send()` do not halt program execution if data is not ready.
    Data Flow Control: Monitors when a socket has data ready to be read (POLLIN) or when it has space to write a response (POLLOUT).
    
**Required Input Parameters**
	
    Configuration Data: Receives critical parameters from the .conf file parser, such as the host:port (listen) pair and the interface address, to create the corresponding listening sockets.
    
	Raw Network Bytes: Using the recv() function, it receives the raw data packets sent by the client (the browser) before they are processed.
    Monitoring Structures: Requires an array of pollfd structures (or equivalents) containing the file descriptors and events it must monitor.
    
**Output Parameters Generated and Interaction Between Objects**

    New File Descriptors: When accepting a connection, it creates a new communication socket specific to that client.
    HTTP Response Messages: It generates the physical byte output to the client’s socket once the response has been constructed.
    
	Which object does it pass the information to? The Server Core does not process the HTTP logic itself, but rather delegates the information to the following objects:
    To the Request Parser (HttpRequest class): It passes the raw bytes received from the socket (typically via a method such as feed()) so that this class can extract the method, URI, and headers.
    To the Response Builder (Response class): After consulting the Parser and applying business logic (such as searching for a file or executing a CGI), the Core interacts with the Builder to obtain the final HTTP string (headers + body) and send it back to the client.
    
	In summary, the Server Core is the technical “boss” in charge of the network and real-time processing, delegating the interpretation work to the Parser and the construction work to the Builder.

**From the Server Core to the Request Parser**

    The Server Core is responsible for the “brute force” of the network.
        
	What it does: It uses functions such as recv() or read() to retrieve the raw bytes directly from the client’s socket.
    The handoff: These bytes, exactly as they arrive from the network, are passed to the HttpRequest class (the Parser), typically via a method called feed(). The Parser processes these bytes one by one to identify where the headers end and where the body begins.
    
**From the Parser to the Response Builder**

    Once the Parser has “translated” the bytes into understandable data (such as the GET method or the path /index.html), the server makes business logic decisions.
    The handoff: The information extracted by the Parser (the requested URI, the allowed methods, etc.) is used to configure the Response Builder.
        
	What the Builder Does: With that information, the Builder locates the file on disk, generates the status code (e.g., 200 OK), and constructs the complete HTTP message (status line + headers + body) in a text string or buffer format.

**From the Response Builder to the Client (via Server Core)**

    Finally, the constructed message must be sent back to the user’s browser.
        
	What the Builder Does: It delivers the data packet—already formatted according to the HTTP protocol—to the Core.
    The Final Transfer: The Server Core takes that packet and uses the client socket to physically send it over the network using the send() or write() functions. Once the transfer is complete, the Core decides whether to close the socket or keep it open for future requests (Keep-Alive).
        
	In summary:
	Core: Manages the socket and byte transfer.
	Parser: Converts raw bytes into logical information.
	Builder: Converts the logical information into a formatted response.