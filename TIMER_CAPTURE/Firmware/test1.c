
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

#include "clock.h"

#define NUM_WORDS_TO_TEST (0x1000 / 4)

enum memory_location { MEMORY_LOCATION_OCM, MEMORY_LOCATION_BRAM };

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
	    mmap(NULL, 0x4000, PROT_READ | PROT_WRITE, MAP_SHARED, dh, 0xFFFC0000);
	bram =
	    mmap(NULL, 0x1000, PROT_READ | PROT_WRITE, MAP_SHARED, dh, 0xA0028000);

	return 0;
}

void
unmap_regs() {
	munmap(timer_regs, 0x1000);
	munmap(cmda_regs, 0x1000);
	munmap(ps_clk_reg, 0x1000);
	munmap(pl_clk_reg, 0x1000);
	munmap(ocm, 0x4000);
	munmap(bram, 0x1000);
}

void
start_dma(uint32_t *src, uint32_t *dst, uint32_t len) {
	// soft reset
	cmda_regs[CDMACR] = 0b100;
	while (cmda_regs[CDMACR] & 0b100) {}

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
wait_timer() {
	while ((timer_regs[3] & 0b111) == 0b010) {}
}

void
run_test_1() {
	// 1. fill ocm with random data
	for (int i = 0; i < NUM_WORDS_TO_TEST; i++) { ocm[i] = rand(); }

	// 2. use dma to transfer from ocm to bram

	// clear CDMACR
	cmda_regs[CDMACR] = 0;

	// set timer_enable to 0
	timer_regs[1] = timer_regs[1] & ~0b10;

	// set timer_enable to 1
	timer_regs[1] = timer_regs[1] | 0b10;

	start_dma(
	    (uint32_t *) 0xFFFC0000, (uint32_t *) 0xC0000000, NUM_WORDS_TO_TEST);

	wait_timer();

	// 3. measure the time it takes to transfer
	int timer_out = timer_regs[2];
	printf("OCM -> BRAM timer_out: %d\n", timer_out);

	// set timer_enable to 0
	timer_regs[1] = timer_regs[1] & ~0b10;

	if (!memcmp(ocm, bram, NUM_WORDS_TO_TEST)) {
		// printf("OCM -> BRAM success\n");
	} else {
		printf("OCM -> BRAM fail\n");
	}

	// 4. use dma to transfer from bram to ocm

	// clear CDMACR
	cmda_regs[CDMACR] = 0;

	// set timer_enable to 1
	timer_regs[1] = timer_regs[1] | 0b10;

	start_dma(
	    (uint32_t *) 0xC0000000, (uint32_t *) 0xFFFC2000, NUM_WORDS_TO_TEST);

	// 5. measure the time it takes to transfer
	timer_out = timer_regs[2];
	printf("BRAM -> OCM timer_out: %d\n", timer_out);

	// set timer_enable to 0
	timer_regs[1] = timer_regs[1] & ~0b10;

	if (!memcmp(ocm, bram, NUM_WORDS_TO_TEST)) {
		// printf("BRAM -> OCM success\n");
	} else {
		printf("BRAM -> OCM fail\n");
	}

	// 6. compare ocm
	if (!memcmp(ocm, &ocm[2048], NUM_WORDS_TO_TEST)) {
		printf("OCM -> BRAM -> OCM success\n");
	} else {
		printf("OCM -> BRAM -> OCM fail\n");
	}
}

int
main() {
	FILE *csv;

	srand(time(0));

	if (map_regs() == -1) {
		printf("Must be ROOT to open /dev/mem\n");
		return 1;
	}

	set_clock(PS_CLK_1499_MHZ, PL_CLK_300_MHZ, PL_CLK_250_MHZ);

	// csv = fopen("timer_out.csv", "w+");
	// fprintf(csv, "PS clk, PL clk, write timer, read timer\n");
	// fprintf(csv, "%s, %s, %d, %d\n", ps_clk_str[ps], pl_clk_str[pl],
	// results.timer_write, results.timer_read);

	// transfer 4k bytes from OCM (0xFFFC_0000) to BRAM (0xC000_0000)
	// printf("DMA started\n");
	// wait_dma();

	// if (memcmp(ocm, bram, NUM_WORDS_TO_TEST) == 0) {
	// 	printf("OCM and BRAM are the same\n");
	// } else {
	//     printf("OCM and BRAM are different\n");
	// }

	run_test_1();

	unmap_regs();
}