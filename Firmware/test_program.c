
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

enum pattern_gen_mode {
	PATTERN_GEN_SLIDING_ZEROES,
	PATTERN_GEN_SLIDING_ONES,
	PATTERN_GEN_LFSR
};

enum memory_location { MEMORY_LOCATION_OCM, MEMORY_LOCATION_BRAM };

enum PS_clock_frequency {
	PS_CLK_1499_MHZ,
	PS_CLK_1250_MHZ,
	PS_CLK_1000_MHZ,
	PS_CLK_858_MHZ,
	PS_CLK_416_6_MHZ,
};

enum PL_clock_frequency {
	PL_CLK_300_MHZ,
	PL_CLK_266_MHZ,
	PL_CLK_187_5_MHZ,
	PL_CLK_150_MHZ,
	PL_CLK_100_MHZ,
};

struct test_params {
	enum memory_location memory_location;
	enum pattern_gen_mode pattern_gen_mode;
	uint32_t pattern_gen_seed;
};

struct test_results {
	bool reads_done;
	bool writes_done;
	bool compare_mismatch_found;
	bool compare_success;
	uint32_t timer_write;
	uint32_t timer_read;
};

bool
is_sudo() {
	int dh = open("/dev/mem", O_RDWR | O_SYNC);
	return dh != -1;
}

void
set_clock(enum PS_clock_frequency ps_clk, enum PL_clock_frequency pl_clk) {
	uint8_t div0, div1;
	int dh = open("/dev/mem", O_RDWR | O_SYNC);
	if (dh == -1) {
		printf("Must be ROOT to open /dev/mem\n");
	}

	uint32_t *pl_clk_reg =
	    mmap(NULL, 0x1000, PROT_READ | PROT_WRITE, MAP_SHARED, dh, 0xFF5E0000);

	int i         = 0;
	uint32_t *pl0 = pl_clk_reg;

	pl0 += 0xC0;  // PL0_REF_CTRL reg offset 0xC0

	// frequency = 1.5Ghz/divisor0/divisor1
	switch (pl_clk) {
	case PL_CLK_300_MHZ:
		div0 = 0x5;
		div1 = 0x0;
		break;
	case PL_CLK_266_MHZ:
        // not actually 266MHz, but 1.5GHz/6 = 250MHz
		div0 = 0x6;
		div1 = 0x0;
		break;
	case PL_CLK_187_5_MHZ:
		div0 = 0x8;
		div1 = 0x0;
		break;
	case PL_CLK_150_MHZ:
		div0 = 0xA;
		div1 = 0x0;
		break;
	case PL_CLK_100_MHZ:
		div0 = 0xF;
		div1 = 0x0;
		break;
	}

	*pl0 = (1 << 24)       // bit 24 enables clock
	       | (div1 << 16)  // bit 21:16 is divisor 1
	       | (div0 << 8);  // bit 13:8 is clock divisor 0

	munmap(pl_clk_reg, 0x1000);

	uint32_t *ps_clk_reg =
        mmap(NULL, 0x1000, PROT_READ | PROT_WRITE, MAP_SHARED, dh, 0xFD1A0000);

    uint32_t *apll_ctrl = ps_clk_reg + 0x20;
    uint32_t *apll_cfg = ps_clk_reg + 0x24;
    uint32_t *pll_status = ps_clk_reg + 0x44;

    munmap(ps_clk_reg, 0x1000);
}

struct test_results
get_results() {
	struct test_results results = {0};
	int dh                      = open("/dev/mem", O_RDWR | O_SYNC);

	uint32_t *tester_regs =
	    mmap(NULL, 32, PROT_READ | PROT_WRITE, MAP_SHARED, dh, 0xA0000000);

	results.reads_done             = (tester_regs[3] >> 3u) & 1u;
	results.writes_done            = (tester_regs[3] >> 2u) & 1u;
	results.compare_mismatch_found = (tester_regs[3] >> 1u) & 1u;
	results.compare_success        = tester_regs[3] & 1u;
	results.timer_write            = tester_regs[4];
	results.timer_read             = tester_regs[5];

	munmap(tester_regs, 32);
	return results;
}

void
print_results(struct test_results results) {
	printf("--------------------------\n");
	printf("Reads done: %d\n", results.reads_done);
	printf("Writes done: %d\n", results.writes_done);
	printf("Compare mismatch found: %d\n", results.compare_mismatch_found);
	printf("Compare success: %d\n", results.compare_success);
	printf("Timer write: 0x%X\n", results.timer_write);
	printf("Timer read: 0x%X\n", results.timer_read);
}

struct test_results
run_test(struct test_params params) {
	struct test_results results = {0};
	uint32_t reg0               = 0;
	int dh                      = open("/dev/mem", O_RDWR | O_SYNC);
	uint32_t *tester_regs =
	    mmap(NULL, 32, PROT_READ | PROT_WRITE, MAP_SHARED, dh, 0xA0000000);

	tester_regs[1] = params.pattern_gen_seed;

	if (params.memory_location == MEMORY_LOCATION_OCM) {
		tester_regs[2] = 0xFFFC0000;
	} else if (params.memory_location == MEMORY_LOCATION_BRAM) {
		// BRAM
		tester_regs[2] = 0xC0000000;
	}

	// start test
	reg0 |= 1u;
	reg0 |= (params.pattern_gen_mode) << 1u;
	tester_regs[0] = reg0;

	munmap(tester_regs, 32);

	do {
		results = get_results();
	} while (!results.reads_done && !results.writes_done);

	tester_regs =
	    mmap(NULL, 32, PROT_READ | PROT_WRITE, MAP_SHARED, dh, 0xA0000000);

    // clear test
    tester_regs[0] = 0;

    munmap(tester_regs, 32);

	return results;
}

int
main() {
	struct test_params params   = {0};
	struct test_results results = {0};

	if (!is_sudo()) {
		printf("Must be ROOT to open /dev/mem\n");
		return 1;
	}

	while (1) {
		params.memory_location  = MEMORY_LOCATION_BRAM;  // Test #1
		params.memory_location  = MEMORY_LOCATION_OCM;   // Test #2
		params.pattern_gen_mode = PATTERN_GEN_LFSR;
		params.pattern_gen_seed = 0x123456;

		printf("test start\n");
		results = run_test(params);

		print_results(results);

		sleep(1);
	}
}