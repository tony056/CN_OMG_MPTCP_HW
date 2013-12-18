all: sender receiver
sender: urt-sender.c
	gcc -Wall -o sender urt-sender.c
receiver: urt-receiver.c
	gcc -Wall -o receiver urt-receiver.c
clean:
	rm -rf sender receiver
