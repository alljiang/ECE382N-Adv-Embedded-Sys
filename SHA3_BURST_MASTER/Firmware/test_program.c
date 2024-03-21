
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

uint32_t *ocm_regs;
uint32_t *burst_regs;
uint32_t *timer_regs;
uint32_t *ps_clk_reg;
uint32_t *pl_clk_reg;

#define ADDRESS_OCM 0xFFFC0000
#define ADDRESS_BURST_MASTER_SLAVE 0xB0000000
#define ADDRESS_CAPTURE_TIMER_SLAVE 0xB0002000

int
map_regs() {
	int dh = open("/dev/mem", O_RDWR | O_SYNC);
	if (dh == -1)
		return -1;

	ocm_regs =
	    mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, dh, ADDRESS_OCM);

	timer_regs = mmap(NULL,
	                  32,
	                  PROT_READ | PROT_WRITE,
	                  MAP_SHARED,
	                  dh,
	                  ADDRESS_CAPTURE_TIMER_SLAVE);

	burst_regs = mmap(NULL,
	                  128,
	                  PROT_READ | PROT_WRITE,
	                  MAP_SHARED,
	                  dh,
	                  ADDRESS_BURST_MASTER_SLAVE);
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

int
main() {
	srand(time(0));

	if (map_regs() == -1) {
		printf("Must be ROOT to open /dev/mem\n");
		return 1;
	}

	set_clock(PS_CLK_1499_MHZ, PL_CLK_150_MHZ);

	char test_string[]          = "The quick brown fox jumps over the lazy dog";
	uint16_t test_string_length = sizeof(test_string) - 1;

	int ocm_index = 0;
	for (int i = 0; i < test_string_length; i++) ocm_regs[i] = 0;

	for (int i = 0; i < test_string_length; i++) {
		if (test_string_length - i > 4) {
			ocm_regs[ocm_index] =
			    (test_string[i] << 24) | (test_string[i + 1] << 16) |
			    (test_string[i + 2] << 8) | test_string[i + 3];
			ocm_index++;
			i += 3;
		} else {
			for (int j = 0; j < test_string_length - i; j++) {
				ocm_regs[ocm_index] =
				    ocm_regs[ocm_index] | (test_string[i + j] << (24 - 8 * j));
			}
			break;
		}
	}

	// set NUMBER_BYTES
	burst_regs[2] = test_string_length;

	// reset
	burst_regs[0] = 0b1;
	burst_regs[0] = 0b0;

	// start
	burst_regs[0] = 0b10;

	// wait until keccak done
	while (!(burst_regs[1] & 0b1000)) {}
    usleep(10000);

	for (int i = 16; i < 32; i++) { printf("0x%08X\n", burst_regs[i]); }

	burst_regs[0] = 0b00;

	unmap_regs();
}