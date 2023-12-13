make:
	gcc server.c -o server
	gcc client.c -o client
	gcc storage_server.c -o ss