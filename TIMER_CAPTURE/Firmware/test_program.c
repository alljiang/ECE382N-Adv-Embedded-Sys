
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

#define NUM_WORDS_TO_TEST (0x1000 / 4)

enum memory_location { MEMORY_LOCATION_OCM, MEMORY_LOCATION_BRAM };

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
	PL_CLK_266_MHZ,
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

enum CDMA_regs {
	CDMACR,
	CDMASR,
	CURDESC_PNTR,
	CURDESC_PNTR_MSB,
	TAILDESC_PNTR,
	TAILDESC_PNTR_MSB,
	SA,
	SA_MSB,
	DA,
	DA_MSB,
	BTT
};

uint32_t *timer_regs;
uint32_t *cmda_regs;
uint32_t *ps_clk_reg;
uint32_t *pl_clk_reg;

uint32_t *ocm;
uint32_t *bram;

int
map_regs() {
	int dh = open("/dev/mem", O_RDWR | O_SYNC);
	if (dh == -1)
		return -1;

	timer_regs =
	    mmap(NULL, 0x1000, PROT_READ | PROT_WRITE, MAP_SHARED, dh, 0xB0002000);
	cmda_regs =
	    mmap(NULL, 0x1000, PROT_READ | PROT_WRITE, MAP_SHARED, dh, 0xB0000000);
	ps_clk_reg =
	    mmap(NULL, 0x1000, PROT_READ | PROT_WRITE, MAP_SHARED, dh, 0xFD1A0000);
	pl_clk_reg =
	    mmap(NULL, 0x1000, PROT_READ | PROT_WRITE, MAP_SHARED, dh, 0xFF5E0000);

	ocm =
	    mmap(NULL, 0x1000, PROT_READ | PROT_WRITE, MAP_SHARED, dh, 0xFFFC0000);
	bram =
	    mmap(NULL, 0x1000, PROT_READ | PROT_WRITE, MAP_SHARED, dh, 0xA0028000);

	return 0;
}

void
unmap_regs() {
	munmap(pl_clk_reg, 0x1000);
	munmap(ps_clk_reg, 0x1000);
	munmap(timer_regs, 4 * 4);
}

void
start_dma(uint32_t *src, uint32_t *dst, uint32_t len) {
	// can do soft reset here if stuffs not working

	// set IOC_IrqEn to 1 (bit 12 of CDMACR)
	cmda_regs[CDMACR] = cmda_regs[CDMACR] | 0x1 << 12;

	// set SA
	cmda_regs[SA] = (uintptr_t) src;

	// set DA
	cmda_regs[DA] = (uintptr_t) dst;

	// set BTT [25:0]. This also starts the transfer
	cmda_regs[BTT] = len;
}

void
wait_dma() {
    while (!(cmda_regs[CDMASR] & 0x2)) {}
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
	default:
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
	FILE *csv;

	srand(time(0));

	if (map_regs() == -1) {
		printf("Must be ROOT to open /dev/mem\n");
		return 1;
	}

	// csv = fopen("timer_out.csv", "w+");
	// fprintf(csv, "PS clk, PL clk, write timer, read timer\n");
	// fprintf(csv, "%s, %s, %d, %d\n", ps_clk_str[ps], pl_clk_str[pl],
	// results.timer_write, results.timer_read);

	// fill up OCM with random data
    printf("Filling up OCM with random data\n");
	for (int i = 0; i < NUM_WORDS_TO_TEST; i++) {
        int random = rand();
		ocm[i] = random;
	}
    printf("OCM filled\n");

	// transfer 4k bytes from OCM (0xFFFC_0000) to BRAM (0xC000_0000)
	start_dma((uint32_t *) 0xFFFC0000, (uint32_t *) 0xC0000000, 0x1000 / 4);
    printf("DMA started\n");
    wait_dma();

	if (memcmp(ocm, bram, NUM_WORDS_TO_TEST) == 0) {
		printf("OCM and BRAM are the same\n");
    } else {
        printf("OCM and BRAM are different\n");
    }

	unmap_regs();
}