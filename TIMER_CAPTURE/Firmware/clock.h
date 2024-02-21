#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

enum PS_clock_frequency {
	PS_CLK_1499_MHZ,
	PS_CLK_1250_MHZ,
	PS_CLK_1000_MHZ,
	PS_CLK_858_MHZ,
	PS_CLK_416_6_MHZ,
	PS_CLK_COUNT
};

char *ps_clk_str[] = {
    "1499MHz",
    "1250MHz",
    "1000MHz",
    "858MHz",
    "416.6MHz",
};

enum PL_clock_frequency {
	PL_CLK_300_MHZ,
	PL_CLK_250_MHZ,
	PL_CLK_187_5_MHZ,
	PL_CLK_150_MHZ,
	PL_CLK_100_MHZ,
	PL_CLK_COUNT
};

char *pl_clk_str[] = {
    "300MHz",
    "266MHz",
    "187.5MHz",
    "150MHz",
    "100MHz",
};

uint32_t *ps_clk_reg;
uint32_t *pl_clk_reg;

void
set_clock(enum PS_clock_frequency ps_clk,
          enum PL_clock_frequency pl0_clk,
          enum PL_clock_frequency pl1_clk) {
	uint8_t div0, div1;
	int i         = 0;
	uint32_t *pl0 = pl_clk_reg + 0xC0 / 4;  // PL0_REF_CTRL reg offset 0xC0
	uint32_t *pl1 = pl_clk_reg + 0xC4 / 4;

	// frequency = 1.5Ghz/divisor0/divisor1
	switch (pl0_clk) {
	case PL_CLK_300_MHZ:
		div0 = 0x5;
		div1 = 0x0;
		break;
	case PL_CLK_250_MHZ:
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
	default:
		break;
	}

	*pl0 = (1 << 24)       // bit 24 enables clock
	       | (div1 << 16)  // bit 21:16 is divisor 1
	       | (div0 << 8);  // bit 13:8 is clock divisor 0

	switch (pl1_clk) {
	case PL_CLK_300_MHZ:
		div0 = 0x5;
		div1 = 0x0;
		break;
	case PL_CLK_250_MHZ:
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
	default:
		break;
	}

	*pl1 = (1 << 24)       // bit 24 enables clock
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
