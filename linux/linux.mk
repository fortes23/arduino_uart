.PHONY: linux-build linux-clean

linux-build:
	g++ -I linux/include linux/linux_client.cpp linux/uart.cpp linux/stream.cpp -o linux_uart

linux-clean:
	rm -f *.o
	rm -f linux_uart
