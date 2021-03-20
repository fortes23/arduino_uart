#include <stdio.h>      // standard input / output functions
#include "linux_client.h"

int main(int argc, char** argv)
{
	LinuxClient client;

	if (client.parse(argc, argv) != 0) {
		return -1;
	}

	if (client.connect() < 0) {
		return -1;
	}

	client.exec();

	return 0;
}
