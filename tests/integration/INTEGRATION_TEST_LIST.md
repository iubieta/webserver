# Integration Test List

This is a list of basic request tests made to an nginx server with a basic config.

### Config
```nginx
server {
	listen		80;
	server_name nginx-lab.com;
	root		/var/www;
	index		index.html;
	autoindex	off;
}
```

## Tests

### GET Method

#### TEST 1 - GET /redir_1
```
GET /redir_1 HTTP/1.1
Host: localhost
```
Commands:
```
curl -v localhost:8080/redir_1
```
```
printf 'GET /redir_1 HTTP/1.1\r\nHost: localhost\r\n\r\n' | nc -q 1 localhost 8080 
```
Response:
- **Status** : 301 Moved Permanently
- **Headers** : Server, Date, Content-Type, Conten-Length, **Location**, Connection
- **Body** : 301 error page
```
HTTP/1.1 301 Moved Permanently
Server: nginx/1.31.6
Date: Tue, 06 Oct 2026 18:07:00 GMT
Content-Type: text/html
Content-Length: 169
Location: http://localhost/permanent_redir
Connection: keep-alive
```
> [!NOTE] 
> For historical reasons, a user agent MAY change the request method from POST 
> to GET for the subsequent request. If this behavior is undesired, 
> the 308 (Permanent Redirect) status code can be used instead.

#### TEST 2 - GET /redir_2
```
GET /redir_2 HTTP/1.1
Host: localhost
```
Commands:
```
curl -v localhost:8080/redir_2
```
```
printf 'GET /redir_2 HTTP/1.1\r\nHost: localhost\r\n\r\n' | nc -q 1 localhost 8080 
```
Response:
- **Status** : 302 Moved Temporarily
- **Headers** : Server, Date, Content-Type, Conten-Length, **Location**, Connection
- **Body** : 302 error page
```
HTTP/1.1 302 Moved Temporarily
Server: nginx/1.31.6
Date: Tue, 06 Oct 2026 18:10:34 GMT
Content-Type: text/html
Content-Length: 145
Location: http://localhost/temporary_redir
Connection: keep-alive
```
> [!NOTE] 
> For historical reasons, a user agent MAY change the request method from POST 
> to GET for the subsequent request. If this behavior is undesired, 
> the 307 (Temporary Redirect) status code can be used instead.

#### TEST 3 - GET /redir_3
```
GET /redir_2 HTTP/1.1
Host: localhost
```
Commands:
```
curl -v localhost:8080/redir_2
```
```
printf 'GET /redir_2 HTTP/1.1\r\nHost: localhost\r\n\r\n' | nc -q 1 localhost 8080 
```
Response:
- **Status** : 307 Temporary Redirect
- **Headers** : Server, Date, Content-Type, Conten-Length, **Location**, Connection
- **Body** : 307 error page
```
HTTP/1.1 307 Temporary Redirect
Server: nginx/1.31.6
Date: Tue, 06 Oct 2026 18:10:34 GMT
Content-Type: text/html
Content-Length: 145
Location: http://localhost/temporary_redir
Connection: keep-alive
```

#### TEST 4 - GET /redir_4
```
GET /redir_1 HTTP/1.1
Host: localhost
```
Commands:
```
curl -v localhost:8080/redir_1
```
```
printf 'GET /redir_1 HTTP/1.1\r\nHost: localhost\r\n\r\n' | nc -q 1 localhost 8080 
```
Response:
- **Status** : 308 Permanent Redirect
- **Headers** : Server, Date, Content-Type, Conten-Length, **Location**, Connection
- **Body** : 308 error page
```
HTTP/1.1 308 Permanent Redirect
Server: nginx/1.31.6
Date: Tue, 06 Oct 2026 18:18:07 GMT
Content-Type: text/html
Content-Length: 171
Location: http://localhost/permanent_redir
Connection: keep-alive
```

#### TEST 5 - GET /redir_0
```
GET /redir_1 HTTP/1.1
Host: localhost
```
Commands:
```
curl -v localhost:8080/redir_1
```
```
printf 'GET /redir_1 HTTP/1.1\r\nHost: localhost\r\n\r\n' | nc -q 1 localhost 8080 
```

#### TEST 1 - GET /
Request:
```
GET / HTTP/1.1
Host: localhost
```
Commands:
```
curl -v localhost:8080/
```
```
printf 'GET / HTTP/1.1\r\nHost: localhost\r\n\r\n' | nc -q 1 localhost 8080 
```
Response:
- With autoindex:
    - Status Code: 200 OK
    - Headers: Server, Date, Content-Type, Transfer-Encoding, Connection
    - Body: Auto generated html index
```
HTTP/1.1 200 OK
Server: nginx/1.31.6
Date: Fri, 02 Oct 2026 16:22:13 GMT
Content-Type: text/html
Transfer-Encoding: chunked
Connection: keep-alive

f7
<html>
<head><title>Index of /</title></head>
<body>
<h1>Index of /</h1><hr><pre><a href="../">../</a>
<a href="images/">images/</a>                                            01-Oct-2026 21:12                   -
</pre><hr></body>
</html>

0
``` 
> [!WARNING]
> When content is sent in chunks (Transfer-Encoding: chunked), the
> server sends each chunk's size in hexadecimal before its content. A
> chunk of size 0 marks the end of the body. This is only visible with
> nc, because it prints every received byte without processing it.

- Without autoindex:
    - Status Code: 403 Fordbidden
    - Headers: Server, Date, Content-Type, Content-Length, Connection
    - Body: 403 error page
```
HTTP/1.1 403 Forbidden
Server: nginx/1.31.6
Date: Fri, 02 Oct 2026 16:24:12 GMT
Content-Type: text/html
Content-Length: 153
Connection: keep-alive

<html>
<head><title>403 Forbidden</title></head>
<body>
<center><h1>403 Forbidden</h1></center>
<hr><center>nginx/1.31.6</center>
</body>
</html>
```
- With index file:
    - Status Code: 200 OK
    - Headers: Server, Date, Content-Type, Content-Length, 
    Last-Modified, Connection, ETag, Accept-Ranges
    - Body: file content
```
HTTP/1.1 200 OK
Server: nginx/1.31.6
Date: Fri, 02 Oct 2026 16:26:22 GMT
Content-Type: text/html
Content-Length: 11
Last-Modified: Fri, 02 Oct 2026 16:26:19 GMT
Connection: keep-alive
ETag: "6abfdb2b-b"
Accept-Ranges: bytes

Index page
```

#### TEST 2 - GET /index.html
Request:
```
GET /index.html HTTP/1.1
Host: localhost
```
Commands:
```
curl -v localhost:8080/index.html
```
```
printf 'GET /index.html HTTP/1.1\r\nHost: localhost\r\n\r\n' | nc -q 1 localhost 8080 
```
Response:
- Status Code: 200 OK
- Headers: Server, Date, Content-Type, Content-Length, 
Last-Modified, Connection, ETag, Accept-Ranges
- Body: file content
```
HTTP/1.1 200 OK
Server: nginx/1.31.6
Date: Fri, 02 Oct 2026 16:26:22 GMT
Content-Type: text/html
Content-Length: 11
Last-Modified: Fri, 02 Oct 2026 16:26:19 GMT
Connection: keep-alive
ETag: "6abfdb2b-b"
Accept-Ranges: bytes

Index page
```

#### TEST 3 - GET /unknown-file
Request:
```
GET /unknown HTTP/1.1
Host: localhost
```
Commands:
```
curl -v localhost:8080/unknown
```
```
printf 'GET /unknown HTTP/1.1\r\nHost: localhost\r\n\r\n' | nc -q 1 localhost 8080 
```
Response:
- Status Code: 404 Not Found
- Headers: Server, Date, Content-Type, Content-Length, Connection
- Body: 404 error page
```
HTTP/1.1 404 Not Found
Server: nginx/1.31.6
Date: Fri, 02 Oct 2026 16:47:39 GMT
Content-Type: text/html
Content-Length: 153
Connection: keep-alive

<html>
<head><title>404 Not Found</title></head>
<body>
<center><h1>404 Not Found</h1></center>
<hr><center>nginx/1.31.6</center>
</body>
</html>
```

#### TEST 5 - GET /subdir/
Request:
```
GET /sub/ HTTP/1.1
Host: localhost
```
Commands:
```
curl -v localhost:8080/sub/
```
```
printf 'GET /sub/ HTTP/1.1\r\nHost: localhost\r\n\r\n' | nc -q 1 localhost 8080 
```
Response: Same as root directory; it depends on the index and autoindex options

#### TEST 6 - GET /subdir (without / at the end)
Request:
```
GET /sub HTTP/1.1
Host: localhost
```
Commands:
```
curl -v localhost:8080/sub
```
```
printf 'GET /sub HTTP/1.1\r\nHost: localhost\r\n\r\n' | nc -q 1 localhost 8080 
```
Response:
- Status Code: 301 Moved Permanently
- Headers: Server, Date, Content-Type, Content-Length, Location, Connection
- Body: 301 status page
```
HTTP/1.1 301 Moved Permanently
Server: nginx/1.31.6
Date: Fri, 02 Oct 2026 17:28:06 GMT
Content-Type: text/html
Content-Length: 169
Location: http://localhost/sub/
Connection: keep-alive

<html>
<head><title>301 Moved Permanently</title></head>
<body>
<center><h1>301 Moved Permanently</h1></center>
<hr><center>nginx/1.31.6</center>
</body>
</html>
```
> [!NOTE]
> Requesting a directory without a trailing slash (`/sub`) returns
> `301 Moved Permanently` with a `Location` header pointing to the
> corrected URL (`/sub/`). The client must send a new request to it;
> browsers do this automatically, `curl` only with `-L`. The query
> string is preserved.

> [!WARNING]
> By default NGINX sends an absolute URL in `Location`:
> `Location: http://host:port/path/?query`
> The host comes from the request's `Host` header, but the port comes
> from the server's own `listen`. Behind a port mapping (Docker) or a
> proxy, that port is not the one the client used, so following the
> redirect fails.
> Fix: `absolute_redirect off;` makes NGINX send a relative reference,
> which the client resolves against the URL it already used:
> `Location: /path/?query`
> RFC 9110 allows relative references in `Location`.
```
HTTP/1.1 301 Moved Permanently
Server: nginx/1.31.6
Date: Fri, 02 Oct 2026 17:52:36 GMT
Content-Type: text/html
Content-Length: 169
Connection: keep-alive
Location: /sub/

<html>
<head><title>301 Moved Permanently</title></head>
<body>
<center><h1>301 Moved Permanently</h1></center>
<hr><center>nginx/1.31.6</center>
</body>
</html>
```

------------------------ 

### HEAD method

#### TEST 1 - HEAD /
Request:
```
HEAD / HTTP/1.1
Host: localhost
```
Commands:
```
curl -I localhost:8080/
```
```
printf 'HEAD / HTTP/1.1\r\nHost: localhost\r\n\r\n' | nc -q 1 localhost 8080 
```
Response: Same as GET but without body

- With autoindex:
    - Status Code: 200 OK
    - Headers: Server, Date, Content-Type, Transfer-Encoding, Connection
    - Body: Auto generated html index

- Without autoindex:
    - Status Code: 403 Fordbidden
    - Headers: Server, Date, Content-Type, Content-Length, Connection
    - Body: 403 error page

- With index file:
    - Status Code: 200 OK
    - Headers: Server, Date, Content-Type, Content-Length, 
    Last-Modified, Connection, ETag, Accept-Ranges
    - Body: file content
```
HTTP/1.1 200 OK
Server: nginx/1.31.6
Date: Fri, 02 Oct 2026 17:20:18 GMT
Content-Type: text/html
Content-Length: 11
Last-Modified: Fri, 02 Oct 2026 17:20:17 GMT
Connection: keep-alive
ETag: "6abfe7d1-b"
Accept-Ranges: bytes
```
> [!NOTE]
> Watch how the content-length is the same even if the response has no body
> That's intended and useful fot the client to know the size of the resource
> it is asking for.

