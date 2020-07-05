CC=g++

client:
	$(CC) -o client client.cpp -lws2_32
server:
	$(CC) -o server server.cpp -lws2_32