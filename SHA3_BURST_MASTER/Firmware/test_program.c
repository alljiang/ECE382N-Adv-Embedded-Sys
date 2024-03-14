
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "clock.h"

uint32_t *timer_regs;
uint32_t *ps_clk_reg;
uint32_t *pl_clk_reg;

#define ADDRESS_BURST_MASTER_SLAVE 0xB0000000
#define ADDRESS_CAPTURE_TIMER_SLAVE 0xB0002000


int
map_regs() {
	int dh = open("/dev/mem", O_RDWR | O_SYNC);
	if (dh == -1)
		return -1;

	timer_regs =
	    mmap(NULL, 32, PROT_READ | PROT_WRITE, MAP_SHARED, dh, 0xA0000000);
	ps_clk_reg =
	    mmap(NULL, 0x1000, PROT_READ | PROT_WRITE, MAP_SHARED, dh, 0xFD1A0000);
	pl_clk_reg =
	    mmap(NULL, 0x1000, PROT_READ | PROT_WRITE, MAP_SHARED, dh, 0xFF5E0000);

	return 0;
}

void
unmap_regs() {
	munmap(pl_clk_reg, 0x1000);
	munmap(ps_clk_reg, 0x1000);
	munmap(timer_regs, 32);
}

void
set_clock(enum PS_clock_frequency ps_clk, enum PL_clock_frequency pl_clk) {
	uint8_t div0, div1;
	int i         = 0;
	uint32_t *pl0 = pl_clk_reg;

	pl0 += 0xC0 / 4;  // PL0_REF_CTRL reg offset 0xC0

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

	// 33.3333MHz * FBDIV / DIV_BY_2
	uint8_t fbdiv, div_by_2;
	uint8_t lock_dly = 63, lfhf = 3, cp, res;
	uint16_t lock_cnt;

	switch (ps_clk) {
	case PS_CLK_1499_MHZ:
		fbdiv    = 45;
		div_by_2 = 0;

		cp       = 3;
		res      = 12;
		lock_cnt = 825;
		break;
	case PS_CLK_1250_MHZ:
		fbdiv    = 75;
		div_by_2 = 1;

		cp       = 3;
		res      = 2;
		lock_cnt = 600;
		break;
	case PS_CLK_1000_MHZ:
		fbdiv    = 30;
		div_by_2 = 0;

		cp       = 4;
		res      = 6;
		lock_cnt = 1000;
		break;
	case PS_CLK_858_MHZ:
		fbdiv    = 26;
		div_by_2 = 0;

		cp       = 3;
		res      = 10;
		lock_cnt = 1000;
		break;
	case PS_CLK_416_6_MHZ:
		fbdiv    = 25;
		div_by_2 = 1;

		cp       = 3;
		res      = 10;
		lock_cnt = 1000;
		break;
	}

	uint32_t *apll_ctrl  = ps_clk_reg + 0x20 / 4;
	uint32_t *apll_cfg   = ps_clk_reg + 0x24 / 4;
	uint32_t *pll_status = ps_clk_reg + 0x44 / 4;

	// step 2 program APLL_CFG
	*apll_cfg = (lock_dly << 25u) | (lock_cnt << 13u) | (lfhf << 10u) |
	            (cp << 5u) | res;

	// step 3 program the bypass
	*apll_ctrl = 0x2D08;

	// step 4 assert reset
	*apll_ctrl = 0x2D09;

	// step 5 deassert reset
	*apll_ctrl = 0x2D08;

	// step 6 check for lock, wait until PLL_STATUS[APLL_LOCK] = 0x1
	while (!(*pll_status & 0x1)) {}

	// step 7 deassert bypass
	*apll_ctrl = 0x2D00;
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
	int rv;
	struct test_results results = {0};
	uint32_t reg0               = 0;

	timer_regs[1] = params.pattern_gen_seed;

	if (params.memory_location == MEMORY_LOCATION_OCM) {
		timer_regs[2] = 0xFFFC0000;
	} else if (params.memory_location == MEMORY_LOCATION_BRAM) {
		// BRAM
		timer_regs[2] = 0xC0000000;
	}

	// start test
	reg0 |= 1u;
	reg0 |= (params.pattern_gen_mode) << 1u;
	timer_regs[0] = reg0;

	while (((timer_regs[3] >> 2) & 0b11) != 0b11) {}
    
	results.reads_done             = (timer_regs[3] >> 3u) & 1u;
	results.writes_done            = (timer_regs[3] >> 2u) & 1u;
	results.compare_mismatch_found = (timer_regs[3] >> 1u) & 1u;
	results.compare_success        = timer_regs[3] & 1u;
	results.timer_write            = timer_regs[4];
	results.timer_read             = timer_regs[5];

	// clear test
	timer_regs[0] = 0;

	return results;
}

int
main() {
	struct test_params params   = {0};
	struct test_results results = {0};
	FILE *csv;

	srand(time(0));

	if (map_regs() == -1) {
		printf("Must be ROOT to open /dev/mem\n");
		return 1;
	}

	csv = fopen("Test_out.csv", "w+");
	fprintf(csv, "PS clk, PL clk, write timer, read timer\n");

	for (int ps = 0; ps < PS_CLK_COUNT; ps++) {
		for (int pl = 0; pl < PL_CLK_COUNT; pl++) {
			bool failed = false;
			set_clock(ps, pl);

			int tests_remaining = TEST_LOOPS;
			while (TEST_LOOPS == 0 || tests_remaining-- != 0) {
				params.memory_location = MEMORY_LOCATION_BRAM;  // Test #1
				// params.memory_location  = MEMORY_LOCATION_OCM;  // Test #2
				params.pattern_gen_mode = PATTERN_GEN_LFSR;
				params.pattern_gen_seed = rand();

				results = run_test(params);

				failed = failed || results.compare_mismatch_found;
                // print_results(results);
				fprintf(csv, "%s, %s, %d, %d\n", ps_clk_str[ps], pl_clk_str[pl], results.timer_write, results.timer_read);
			}

			if (!failed) {
				printf("Test passed: ");
			} else {
				printf("Test failed: ");
			}
			printf(
			    "%d Loops of 1024 32-bit words in %s. PS clk: %s, PL clk: %s\n",
			    TEST_LOOPS,
			    memory_location_str[params.memory_location],
			    ps_clk_str[ps],
			    pl_clk_str[pl]);
		}
	}

	unmap_regs();
}