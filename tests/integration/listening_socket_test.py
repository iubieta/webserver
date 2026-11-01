# listening_socket_test.py
#
# ListeningSocket class test file
# -----------------------------------------------------------------------------
# Test	|	Caso								|	Comprobación
# ----------------------------------------------------------------------
# 1		|	Bind correcto en puerto libre		|	setup() → true, puerto visible en ss
# 2		|	Conexión aceptada por el kernel		|	nc -z → éxito
# 3		|	Puerto ocupado por otro proceso		|	setup() → false, sin crash
# 4		|	Reinicio inmediato (SO_REUSEADDR)	|	segundo arranque → true
# 5		|	Puerto privilegiado sin permisos	|	→ false
# 6		|	Puerto fuera de rango (70000, 0)	|	→ false
# 7		|	Host inexistente en la máquina		|	→ false
# 8		|	Host específico vs comodín			|	ss muestra 127.0.0.1:8080, no 0.0.0.0
# 9		|	Varios puertos simultáneos			|	los N aparecen en ss
# 10	|	Liberación al destruir				|	tras salir del scope, ss no lo muestra
# 11	|	acceptClient() sin cliente			|	→ -1 (no bloqueante confirmado)
# 12	|	acceptClient() con cliente			|	→ fd ≥ 0
# 13	|	Leaks								|	valgrind limpio en todos los anteriores

