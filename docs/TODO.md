# TO DO-s

## [Connection.cpp](/src/Connection.cpp)
- [x] temp no termina necesariamente en '\0' 
- [x] Añadir gestion de timeouts
- [ ] Añadir enum de estados

## [ServerCore.cpp](/src/ServerCore.cpp)
- [ ] Revisar todos los posibles errores de epoll_ctl con errno
- [x] Destruir puntero de ls si falla el setup
- [x] modificar orden de socks_.push y epoll_ctl en initSocket()
- [x] cerrar epoll_fd cuando se llame el destructor
- [x] poner sockets de conexiones (accept) en modo no bloquente
- [ ] Distincion de errores en recv y send en modo no bloqueante
- [ ] Investigar sobre SIGPIPE en errores de send
- [ ] Comprobar eficiencia a la hora de aceptar clientes
- [x] checkear retorno de setup en caso de error
- [ ] Cerrar el programa cuando se necesario
    - [ ] epoll_wait fail

## [ListeningSocket.cpp](/src/ListeningSocket.cpp)
- [x] No instanciar con puerto invalido
