linux:
	g++ linux/linux_client.cpp linux/uart.cpp linux/stream.cpp -o linux_uart

clean:
	rm -f linux/*.o
	rm -f linux_uart

.PHONY: linux