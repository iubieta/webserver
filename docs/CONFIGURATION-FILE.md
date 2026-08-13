The configuration file is a critical component that dictates the behavior 
of the web server, allowing it to handle multiple sites and specific routing 
rules without the need to recompile the code. Its design should be based on the
NGINX block structure and strictly comply with the C++98 standard.

# A configuration file must be able to

• Define all interface:port pairs on which a server will listen
(defining multiple websites served by the program).
• Configure the default error pages.
• Set the maximum allowed size for client request bodies.
• Specify the rules or settings for a URL/path  for a website, from among 
the following:
- List of accepted HTTP methods for the path.
- HTTP redirection.
- Directory where the requested file should be located (for example, if the 
URL /kapouet is rooted in /tmp/www, the URL /kapouet/pouic/toto/pouet will 
look for /tmp/www/pouic/toto/pouet).
- Enable or disable directory listing.
- Default file served when the requested resource is a directory.
- File uploads from clients to the server are allowed, and the storage location
is specified.

# Configuration File Structure

The file is organized hierarchically into server blocks (server { ... }), which
in turn contain location blocks (location { ... }):

1. server block: Defines the global configuration for a virtual server or 
a specific instance (port, host, name).
2. location block: Defines rules for specific URL paths within that server 
(methods, file root, CGI).

# Rules for Parsing

When designing the parser for the .conf file, the following technical rules
must be taken into account:

1. C++98 Standard: Modern libraries cannot be used; parsing must be performed 
using standard tools such as std::ifstream, std::stringstream, or std::string.
2. Tokenization: The content must be separated by delimiters (spaces, line
breaks, curly braces { }, and semicolons ;) and comments beginning 
with # must be ignored.
3. Required Parameters: The server must validate that essential directives are
present; for example, listen (port) and root (root directory) are typically 
required for a site to function.
4. Resilience: The program must not crash unexpectedly if the configuration 
file contains syntax errors; instead, it must report the error or use default
values.
5. Interface:Port Pairs: The server must be able to handle and configure 
multiple interface:port pairs to listen on several sockets simultaneously.

During parsing, we must take inheritance into account, as it allows a
configuration defined at a higher level (the server block) to be automatically
applied to lower levels (the location blocks), unless the latter override it.

1. Directive Cascading: If a location block does not specify a directive such 
as root, it is automatically taken from the configuration defined in the 
corresponding server block.
2. Override: When a directive is defined in both the server and location 
blocks, the value in the location block takes precedence for requests matching
that specific path.
3. Copying default values: Directives such as index in a location block
typically copy the values from the server’s root index by default if they are
not declared independently.

# Data to Collect and Division by Blocks

The collected information is divided according to the scope of the application:

1. Server Block Data (server)

This corresponds to the general configuration of the network instance:

* listen: Listening port (required).
* host: IP address of the interface (default 127.0.0.1).
* server_name: Server identifier (useful for virtual hosts).
* error_page: Paths to custom error pages.
* client_max_body_size: Maximum limit in bytes for request bodies.
* root: Root directory for site files.
* index: Default file served when a directory is requested.
2. Location Block Settings

These configure specific rules for particular paths or resources:

* allow_methods: List of accepted HTTP methods (GET, POST, DELETE).
* autoindex: Enable or disable automatic directory listing.
* return: HTTP redirection to another URL.
* alias: Replaces the path portion of the location when searching for files.
* File Upload: Authorization and storage path for files uploaded by clients.

3. CGI Configuration Settings

These are defined within specialized location blocks for dynamic content:

* cgi_path: Location of the interpreters installed on the system 
(e.g., /usr/bin/python3).
* cgi_ext: File extensions that trigger CGI execution (e.g., .py, .sh).


# Information by Class

For a clean C++ architecture, the information is typically distributed across 
two main classes that the Server Core will consult:

1. ServerConfig Class:

- Stores the port, host, server name, and maximum body size.
- Contains a map of error pages (associating error codes with file paths).
- Maintains a list or vector of objects of the LocationConfig class.

2. LocationConfig Class:

- Stores the path prefix (e.g., /images or cgi-bin).
- Stores the allowed methods, autoindex status, and redirection settings.
- Contains the specific paths for CGI interpreters and their associated
extensions.

# Minimum Required Directives

1. Required Server-Level Directives

These directives define the identity and entry point of each virtual server:

* listen: This is the most critical parameter, as it defines the port number on
which the server will listen for incoming connections.
* root: Specifies the root directory of the file system where the website’s
documents are located (this can be an absolute or relative path).
* host: Defines the IP address of the interface to which the socket will be
bound. Although it typically defaults to 127.0.0.1 if omitted, it is essential 
for network configuration.

2. Required Directives for Dynamic Content (CGI)

If the configuration block is intended to handle CGI (Python scripts, PHP, 
etc.), the following additional parameters are required so that the program 
knows how to execute them:

* cgi_path: Specifies the exact location of the interpreters installed on the 
system (for example, /usr/bin/python3 or /bin/bash).
* cgi_ext: Defines the file extensions that the server should treat as CGI 
executable files (e.g., .py, .sh).
* root (in the location block): For CGI paths, this is also marked as a 
required parameter to locate the scripts.

3. Server Logical Requirements

In addition to the keywords listed above, the configuration file must be able 
to structure the following information to comply with the project rules:

* Interface:Port Pairs: The file must allow for the definition of multiple 
blocks so that the server can listen on several ports simultaneously and 
deliver different content.
* Maximum Body Size (client_max_body_size): Although it may have a default 
value, this is a directive that the server must be able to process to limit the
size of client requests and prevent resource exhaustion attacks.
* Error Pages: While the server must have default error pages built into its 
code, the configuration file must allow for the customization of these paths 
(for example, for a 404 error).

Summary of minimum syntax (example):

    server {
    
    listen 8080;                # Required
    root ./html;                # Required
    host 127.0.0.1;             # Required/Default

    location cgi-bin {
        root ./scripts;         # Required for CGI
        cgi_path /usr/bin/python3; # Required for CGI
        cgi_ext .py;            # Required for CGI
        }
    }

# Note

Once the analysis is complete, this information is passed to Server Core to 
open the ports, to the Request Parser to validate size limits, and to the 
Response Builder to locate the requested files on disk.
