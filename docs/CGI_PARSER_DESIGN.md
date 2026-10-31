# **CGI Parser Design**

## **Subject Requirement**

The Webserv subject requires the server to execute CGI scripts based on their file extension and to support at least one CGI type, such as Python or PHP-CGI.

The subject does not impose specific configuration directive names. Therefore, this parser will use a simple custom directive that explicitly associates one file extension with one interpreter.

Supporting multiple CGI types is not required for the mandatory part. However, the selected map-based design can be extended later without changing the configuration model.

## **Selected Configuration Syntax**

The `cgi` directive is valid only inside a `location` block:

```nginx
server {
    root /var/www;

    location /cgi-bin/ {
        allow_methods GET POST;
        cgi .py /usr/bin/python3;
    }
}
```

Its syntax is:

```text
cgi <extension> <interpreter_path>;
```

Each directive creates one explicit association:

```text
.py -> /usr/bin/python3
```

If support for multiple CGI types is implemented later, the directive may be repeated with different extensions:

```nginx
location /cgi-bin/ {
    root /var/www;
    cgi .py /usr/bin/python3;
    cgi .sh /bin/bash;
}
```

## **Internal Representation**

`LocationConfig` will store CGI handlers in a map:

```cpp
std::map<std::string, std::string>
```

The key is the file extension and the value is the absolute path to its interpreter. An empty map means that CGI is not configured for the location. A non-empty map means that the location contains CGI rules.

CGI configuration belongs only to `LocationConfig`. It is not inherited from `ServerConfig`.

## **Root Requirement**

A CGI location must use `root`. The root may be:

- Declared locally inside the location.
- Inherited from the enclosing server.

Both configurations are valid:

**Example 1: inherited root**

```nginx
server {
    root /var/www;

    location /cgi-bin/ {
        cgi .py /usr/bin/python3;
    }
}
```

**Example 2: local root**

```nginx
server {
    location /cgi-bin/ {
        root /var/www;
        cgi .py /usr/bin/python3;
    }
}
```

The final root requirement must be checked after the complete server block has been parsed and configuration inheritance has been resolved. This makes the result independent of directive order.

## **Unsupported Combinations**

For the first CGI implementation, the parser will reject CGI locations containing either `alias` or `return`.

Invalid examples:

```nginx
location /cgi-bin/ {
    alias /var/scripts/;
    cgi .py /usr/bin/python3;
}
```

```nginx
location /cgi-bin/ {
    root /var/www;
    return 301 /maintenance;
    cgi .py /usr/bin/python3;
}
```

This restriction removes ambiguous path resolution and avoids configurations where `return` prevents the CGI rule from ever being used.

## **Parser Validation Criteria**

### **Directive structure**

The parser must verify that:

- `cgi` appears only inside a `location` block.
- The directive contains exactly one extension and one interpreter path.
- The interpreter token contains the terminating semicolon.
- Neither argument is empty.
- Extra arguments are rejected.

### **Extension validation**

The extension must:

- Start with `.`.
- Contain at least one character after the dot.
- Not contain `/`.
- Not already exist in the CGI handler map.

Valid examples:

```text
.py
.php
.sh
```

Invalid examples:

```text
py
.
/script.py
.py/
```

Extension matching will be case-sensitive, so `.py` and `.PY` are different extensions.

### **Interpreter validation**

The interpreter path must:

- Be an absolute path beginning with `/`.
- Exist according to `stat()`.
- Refer to a regular file according to `S_ISREG()`.
- Be executable according to `access(path, X_OK)`.

`access(path, X_OK)` is not sufficient by itself because directories may also have execute permission. For that reason, the parser must combine `stat()`, `S_ISREG()` and `access()`.

### **Location-level validation**

After parsing the complete server and resolving inheritance, every location with a non-empty CGI map must:

- Have a local or inherited `root`.
- Not have `alias` configured.
- Not have `return` configured.

## **Summary of the Design**

- CGI is configured only inside locations.
- One directive represents one complete extension-interpreter pair.
- The pairs are stored in a map.
- CGI configuration is not inherited.
- A local or inherited `root` is mandatory.
- `alias` and `return` are rejected in CGI locations.
- Interpreter paths must be absolute, regular files and executable.
- Cross-directive checks are performed after the server has been fully parsed and inheritance has been resolved.

