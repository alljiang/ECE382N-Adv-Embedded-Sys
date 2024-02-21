
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

uint32_t *timer_regs;
uint32_t *cmda_regs;

uint32_t *ocm;
uint32_t *bram;

void
sighandler(int signo) {
	if (signo == SIGIO) {
        int timer_out = timer_regs[2];
        printf("Interrupt received. Time: %d\n", timer_out);

        // stop timer
        timer_regs[1] &= ~0b11;
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
wait_timer() {
	while ((timer_regs[3] & 0b111) == 0b010) {}
}
void
setup_capture_timer_interrupt() {
	struct sigaction action;
	int fc, fd;

	sigemptyset(&action.sa_mask);
	sigaddset(&action.sa_mask, SIGIO);

	action.sa_handler = sighandler;
	action.sa_flags   = 0;

	sigaction(SIGIO, &action, NULL);

	fd = open("/dev/timer_int", O_RDWR);

	if (fd == -1) {
		perror("Unable to open /dev/timer_int");
		exit(-1);
	}

	printf("/dev/timer_int opened successfully \n");

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

void
run_test_2() {
	timer_regs[1] &= ~0b11;
    usleep(1);
	timer_regs[1] |= 0b11;

	wait_timer();
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
	setup_capture_timer_interrupt();

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

	// run_test_1();
	run_test_2();

	unmap_regs();
}