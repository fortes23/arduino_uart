all:
	g++ linux_client.cpp uart.cpp -o linux_client

clean:
	rm -f *.o
	rm -f linux_client

.PHONY: all