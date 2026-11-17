# Manual HTTP Integration Tests

  Run these commands in order with the server listening on 127.0.0.1:8080. The /uploads location must allow GET, POST, and DELETE.
1. Create a text file
  Expected: 201 Created, Location: /uploads/prueba-curl.txt, and an empty response body. If the file already exists, expect 200 OK.

  ``` bash
  curl -4 --http1.1 -v \
  -H 'Content-Type: text/plain' \
  --data-binary 'Hola desde curl' \
  http://localhost:8080/uploads/prueba-curl.txt

  ``` 

2. Retrieve the text file
  Expected: 200 OK, Content-Length: 15, and body Hola desde curl.

  ``` bash
  curl -4 --http1.1 -v \
  http://localhost:8080/uploads/prueba-curl.txt

  ```

3. Update the existing file
  Expected: 200 OK. The following GET must return Contenido actualizado with Content-Length: 21.

  ```bash
  curl -4 --http1.1 -v \
  -H 'Content-Type: text/plain' \
  --data-binary 'Contenido actualizado' \
  http://localhost:8080/uploads/prueba-curl.txt

  ```

  ```bash
  curl -4 --http1.1 -v \
  http://localhost:8080/uploads/prueba-curl.txt

  ``` 

4. Send an empty body
  Expected: POST returns 204 No Content without hanging. The following GET returns 200 OK with Content-Length: 0.

  ```bash
  curl -4 --http1.1 -v \
  -H 'Content-Type: text/plain' \
  --data-binary '' \
  http://localhost:8080/uploads/prueba-curl.txt
  
  ```

  ```bash
  curl -4 --http1.1 -v \
  http://localhost:8080/uploads/prueba-curl.txt

  ```

5. Send a single space
  Expected: POST returns 200 OK. GET returns one byte, and od prints 20, the hexadecimal value of a space.

  ```bash
  curl -4 --http1.1 -v \
  -H 'Content-Type: text/plain' \
  --data-binary ' ' \
  http://localhost:8080/uploads/prueba-curl.txt

  ```

  ```bash
  curl -4 --http1.1 -v \
  -o /tmp/webserv-espacio.txt \
  http://localhost:8080/uploads/prueba-curl.txt

  ```

  ``` bash
  od -An -tx1 /tmp/webserv-espacio.txt
  ```

6. Delete the file and verify removal
  Expected: DELETE returns 204 No Content without a body or Content-Length. The following GET returns 404 Not Found.
  
  ```bash
  curl -4 --http1.1 -v \
  -X DELETE \
  http://localhost:8080/uploads/prueba-curl.txt

  ```
```bash
  curl -4 --http1.1 -v \
  http://localhost:8080/uploads/prueba-curl.txt
```

7. Upload and download a binary file
Expected: POST sends eight bytes. GET returns Content-Length: 8. cmp prints nothing and its exit status is 0, confirming identical files.

```bash
  printf '\000\001\002\012\015\177\200\377' > /tmp/webserv-binario.bin
```

```bash
  curl -4 --http1.1 -v \
  -H 'Content-Type: application/octet-stream' \
  --data-binary @/tmp/webserv-binario.bin \
  http://localhost:8080/uploads/prueba-binaria.bin
```

```bash
  curl -4 --http1.1 -v \
  -o /tmp/webserv-binario-descargado.bin \
  http://localhost:8080/uploads/prueba-binaria.bin
```

```bash
  cmp /tmp/webserv-binario.bin /tmp/webserv-binario-descargado.bin
  echo $?
``` 

8. Upload and download a 100 KiB binary file
  Expected: the request and GET response report Content-Length: 102400. cmp returns exit status 0.

  ```bash
  dd if=/dev/urandom of=/tmp/webserv-grande.bin bs=1024 count=100
  ```

  ```bash
  curl -4 --http1.1 -v \
  -H 'Content-Type: application/octet-stream' \
  --data-binary @/tmp/webserv-grande.bin \
  http://localhost:8080/uploads/prueba-grande.bin
  ```

```bash
  curl -4 --http1.1 -v \
  -o /tmp/webserv-grande-descargado.bin \
  http://localhost:8080/uploads/prueba-grande.bin
```

```bash
cmp /tmp/webserv-grande.bin /tmp/webserv-grande-descargado.bin
echo $?
```

9. Send text using chunked transfer encoding
  Expected: POST uses Transfer-Encoding: chunked without Content-Length. GET returns the decoded text Hola enviado con chunks with Content-Length: 23.

  ```bash
  curl -4 --http1.1 -v \
  -H 'Content-Type: text/plain' \
  -H 'Transfer-Encoding: chunked' \
  --data-binary 'Hola enviado con chunks' \
  http://localhost:8080/uploads/prueba-chunked.txt
  ```

  ```bash
  curl -4 --http1.1 -v \
  http://localhost:8080/uploads/prueba-chunked.txt

  ```

10. Send an empty chunked body
  Expected: POST returns 204 No Content without hanging. GET confirms that the existing file is empty with Content-Length: 0.

  ```bash
  curl -4 --http1.1 -v \
  -H 'Content-Type: text/plain' \
  -H 'Transfer-Encoding: chunked' \
  --data-binary '' \
  http://localhost:8080/uploads/prueba-chunked.txt
  ```

```bash
  curl -4 --http1.1 -v \
  http://localhost:8080/uploads/prueba-chunked.txt
```

11. Upload a 100 KiB binary file using chunked encoding
  Expected: GET returns Content-Length: 102400. cmp returns exit status 0, confirming that chunk decoding preserved every payload byte.

  ```bash
  curl -4 --http1.1 -v \
  -H 'Content-Type: application/octet-stream' \
  -H 'Transfer-Encoding: chunked' \
  --data-binary @/tmp/webserv-grande.bin \
  http://localhost:8080/uploads/prueba-grande-chunked.bin
  ``` 

  ```bash
  curl -4 --http1.1 -v \
  -o /tmp/webserv-grande-chunked-descargado.bin \
  http://localhost:8080/uploads/prueba-grande-chunked.bin

  ```
```bash
  cmp /tmp/webserv-grande.bin /tmp/webserv-grande-chunked-descargado.bin
  echo $?
```

For file uploads, the current implementation returns 201 Created when creating a resource and 200 OK when replacing an existing resource with nonempty content.