
#include <fcntl.h>
#include <math.h>
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
#define TEST_LOOPS 500

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

int history[TEST_LOOPS];
int sample_count = 0;
int interrupt_count;

void
sighandler(int signo) {
	if (signo == SIGIO) {
		interrupt_count++;
	}

	return; /* Return to main loop */
}

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
    wait_dma();
    for (volatile int i = 0; i < 100000; i++) {}

	// 3. measure the time it takes to transfer
	int timer_out = timer_regs[2];
	// printf("OCM -> BRAM timer_out: %d\n", timer_out);
	history[sample_count++] = timer_out;

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
	// printf("BRAM -> OCM timer_out: %d\n", timer_out);

	// set timer_enable to 0
	timer_regs[1] = timer_regs[1] & ~0b10;

	if (!memcmp(ocm, bram, NUM_WORDS_TO_TEST)) {
		// printf("BRAM -> OCM success\n");
	} else {
		printf("BRAM -> OCM fail\n");
	}

	// 6. compare ocm
	if (!memcmp(ocm, &ocm[2048], NUM_WORDS_TO_TEST)) {
		// printf("OCM -> BRAM -> OCM success\n");
	} else {
		printf("OCM -> BRAM -> OCM fail\n");
	}
}

void
setup_dma_interrupt() {
	struct sigaction action;
	int fc, fd;

	sigemptyset(&action.sa_mask);
	sigaddset(&action.sa_mask, SIGIO);

	action.sa_handler = sighandler;
	action.sa_flags   = 0;

	sigaction(SIGIO, &action, NULL);

	fd = open("/dev/dma_int", O_RDWR);

	if (fd == -1) {
		perror("Unable to open /dev/dma_int");
		exit(-1);
	}

	printf("/dev/dma_int opened successfully \n");

	fc = fcntl(fd, F_SETOWN, getpid());

	if (fc == -1) {
		perror("SETOWN failed\n");
		exit(-1);
	}

	fc = fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_ASYNC);

	if (fc == -1) {
		perror("SETFL failed\n");
		exit(-1);
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
	setup_dma_interrupt();

	for (int i = 0; i < TEST_LOOPS; i++) { run_test_1(); }

	// calculate stats
	int min = history[0];
	int max = history[0];
	int sum = 0;
	for (int i = 0; i < TEST_LOOPS; i++) {
		if (history[i] < min) {
			min = history[i];
		}
		if (history[i] > max) {
			max = history[i];
		}
		sum += history[i];
	}

	float stdev = 0;
	float mean  = (float) sum / TEST_LOOPS;
	for (int i = 0; i < TEST_LOOPS; i++) {
		stdev += (history[i] - mean) * (history[i] - mean);
	}
	stdev = sqrt(stdev / (TEST_LOOPS - 1));

	int proc_interrupts_count = 0;
	int interrupt_number;

	FILE *proc_interrupts =
	    popen("cat /proc/interrupts | grep cdma-controller", "r");
	fscanf(proc_interrupts, "%d", &interrupt_number);
	fscanf(proc_interrupts, "%*s %d", &proc_interrupts_count);
	pclose(proc_interrupts);

	printf("********************************\n");
	printf("Minimum Latency: %d\n", min);
	printf("Maximum Latency: %d\n", max);
	printf("Average Latency: %.2f\n", mean);
	printf("Standard Deviation: %.2f\n", stdev);
	printf("Number of samples: %d\n", interrupt_count);
	printf("Interrupt %d count: %d\n", interrupt_number, proc_interrupts_count);
	printf("********************************\n");

	unmap_regs();
}