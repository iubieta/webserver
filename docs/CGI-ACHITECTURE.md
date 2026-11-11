1. **Stage 1. CGI Execution Preparation**
	**Objective:** Build the data required by the child process.
    - Selection of the interpreter and script.
    - Construction of `argv`.
    - Construction of `envp`.
    - CGI variables: `REQUEST_METHOD`, `SCRIPT_NAME`, `QUERY_STRING`, `CONTENT_LENGTH`, etc.
2. **Stage 2. Inter-Process Communication**
	**Objective:** Implement the creation of the CGI process and its communication channels.
    - `fork()` and `execve()`.
    - `pipe()`, `dup2()`, and `close()`.
    - Script input via `stdin`.
    - Script output via `stdout`.
    - Working directory and proper descriptor cleanup.
3. **Stage 3. Non-Blocking Communication with the Event Loop**
	**Objective:** Integrate the CGI pipes with the multiplexing mechanism already used by Webserv.
    - Progressive writing of the HTTP body to CGI.
    - Progressive reading of the CGI output.
    - Simultaneous management of both channels.
    - EOF, disconnections, and process lifecycle.
4. **Stage 4. CGI Response Parser**
	**Objective:** Correctly interpret what the script returns.
    - Separation of headers and body.
    - `Content-Type`, `Status`, and `Location`.
    - Normal responses and redirections.
    - Incorrect, duplicate, or incomplete headers.
    - Binary bodies and partial reads.
5. **Stage 5. Integration with HttpResponse**
	**Objective:** Transform the CGI result into a valid HTTP response.
    - Delivery of the result to the Response Builder.
    - Handling of HTTP status codes.
    - Management of headers and body.
    - Error responses.
6. **Stage 6. Robustness and Error Handling**
	**Objective:** Cover edge cases that could compromise server stability.
    - Timeouts.
    - `execve()` failures.
    - Abnormally terminated processes.
    - Scripts that fail to produce a valid response.
    - Cleanup of processes and descriptors.
    - Disconnected clients.
7. **Stage 7. Full Validation**
	**Objective:** Test and verify the CGI within the completed Webserv.
    - GET and `QUERY_STRING`.
    - POST and body transfer.
    - Multiple interpreters or CGI programs.
    - Large and binary responses.
    - Multiple simultaneous CGI requests.
