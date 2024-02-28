
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

#define NUM_TEST_SAMPLES 10000

enum memory_location { MEMORY_LOCATION_OCM, MEMORY_LOCATION_BRAM };

uint32_t *timer_regs;
uint32_t *cmda_regs;

uint32_t *ocm;
uint32_t *bram;

int interrupt_timer_out;
int interrupt_count = 0;

int history[NUM_TEST_SAMPLES];

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

void
sighandler(int signo) {
	if (signo == SIGIO) {
		interrupt_timer_out        = timer_regs[2] / 250;  // convert to us
		history[interrupt_count++] = interrupt_timer_out;

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
    
	// clear CDMACR
	cmda_regs[CDMACR] = 0;

	for (int i = 0; i < NUM_TEST_SAMPLES; i++) {
		timer_regs[1] &= ~0b11;
		usleep(1);
		timer_regs[1] |= 0b11;
		usleep(1);

		wait_timer();

		printf("Iteration %d: %d\n", i, interrupt_timer_out);
	}

	// calculate stats
	int min = history[0];
	int max = history[0];
	int sum = 0;
	for (int i = 0; i < NUM_TEST_SAMPLES; i++) {
		if (history[i] < min) {
			min = history[i];
		}
		if (history[i] > max) {
			max = history[i];
		}
		sum += history[i];
	}

	float stdev = 0;
	float mean  = (float) sum / interrupt_count;
	for (int i = 0; i < interrupt_count; i++) {
		stdev += (history[i] - mean) * (history[i] - mean);
	}
	stdev = sqrt(stdev / (interrupt_count - 1));

	int proc_interrupts_count = 0;
    int interrupt_number;

	FILE *proc_interrupts =
	    popen("cat /proc/interrupts | grep capture-timer", "r");
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