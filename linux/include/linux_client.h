#include <stdio.h>
#include <stdint.h>
#include <string>
#include <uart.h>

class LinuxClient {
public:
	void usage(FILE *output) const;
	int32_t parse(int32_t argc, char *const argv[]);
	void action(void);
	int32_t connect(void);
	void exec(void);

private:
	Stream serial;
	std::string dev_port;
	bool get_stats = false;
	bool act_do = false;
	bool deact_do = false;
	uint32_t n_do;
};
