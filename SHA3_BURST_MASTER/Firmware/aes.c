
#include <fcntl.h>
#include <malloc.h>
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

uint32_t *ocm_regs;
uint32_t *aes_regs;
uint32_t *timer_regs;
uint32_t *ps_clk_reg;
uint32_t *pl_clk_reg;

#define ADDRESS_OCM 0xFFFC0000
#define ADDRESS_AES_SLAVE 0xB0000000
#define ADDRESS_CAPTURE_TIMER_SLAVE 0xB0002000

uint32_t timer_value;

enum AES_KEY_SIZE {
	AES_KEY_SIZE_128 = 0b00,
	AES_KEY_SIZE_192 = 0b01,
	AES_KEY_SIZE_256 = 0b10
};

void
sighandler(int signo) {
	// if (signo == SIGIO) {
	// 	timer_value = timer_regs[2] * 1000 / 300;  // convert to ns
	// }

	return; /* Return to main loop */
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

	aes_regs = mmap(
	    NULL, 128, PROT_READ | PROT_WRITE, MAP_SHARED, dh, ADDRESS_AES_SLAVE);
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

// string to hex
int
s_to_h(char *str) {
	int lh = str[0] <= '9' ? str[0] - '0' : str[0] - 'A' + 10;
	int rh = str[1] <= '9' ? str[1] - '0' : str[1] - 'A' + 10;
	return lh * 16 + rh;
}

int
initialize_stuff() {
	srand(time(0));

	if (map_regs() == -1) {
		printf("Must be ROOT to open /dev/mem\n");
		return 1;
	}

	set_clock(PS_CLK_1499_MHZ, PL_CLK_300_MHZ);
	// setup_capture_timer_interrupt();

	return 0;
}

// returns number of blocks
int
set_plaintext(char *data, int32_t num_bytes) {
    int32_t byte_index = 0;
    int num_blocks = num_bytes >> 4;
    if (num_blocks << 4 < num_bytes) {
        num_blocks++;
    }

	// each block is 128 bits (16 bytes)
	for (int i = 0; i < 4 * num_blocks; i++) {
		uint32_t full_word = 0;
		for (int j = 0; j < 4; j++) {
            if (byte_index++ < num_bytes) {
				full_word = (full_word << 8) | data[i * 4 + j];
			} else {
                full_word = (full_word << 8) | 0;
            }
		}
		ocm_regs[i] = full_word;
	}

	// set number_blocks
	aes_regs[2] = num_blocks;

    return num_blocks;
}

void
set_aes_key(char *key_in_hex, enum AES_KEY_SIZE key_size) {
	int reg_offset;
	int bits;
	switch (key_size) {
	case AES_KEY_SIZE_128:
		reg_offset = 7;
		bits       = 128;
		break;
	case AES_KEY_SIZE_192:
		reg_offset = 5;
		bits       = 192;
		break;
	case AES_KEY_SIZE_256:
		reg_offset = 3;
		bits       = 256;
		break;
	}

	for (int i = 0; i < bits / 32; i++) {
		aes_regs[i + reg_offset] = (s_to_h(&key_in_hex[i * 8]) << 24) |
		                           (s_to_h(&key_in_hex[i * 8 + 2]) << 16) |
		                           (s_to_h(&key_in_hex[i * 8 + 4]) << 8) |
		                           s_to_h(&key_in_hex[i * 8 + 6]);
	}

	// set key size
	aes_regs[0] = (key_size << 4);
}

void
set_aes_iv(uint64_t iv_upper_half, uint64_t iv_lower_half) {
	aes_regs[11] = iv_upper_half >> 32;
	aes_regs[12] = iv_upper_half & 0xFFFFFFFF;
	aes_regs[13] = iv_lower_half >> 32;
	aes_regs[14] = iv_lower_half & 0xFFFFFFFF;
}

void
run_accelerator() {
	// reset
	aes_regs[0] |= 0b1;
	aes_regs[0] &= ~0b1;

	// start
	aes_regs[0] |= 0b10;

	// wait until aes done
	while (!(aes_regs[1] & 0b10)) {}
}

int
main(int argc, char *argv[]) {
	int rv;


	rv = initialize_stuff();
	if (rv != 0)
		return rv;

    /*
    TODO
    use the command line arguments to pass in:
    - the plaintext as a file
    - the key (and the key size - this can be implied based on the length of the key)
    - the IV
    - output file

    After making this into a command line program, write a python wrapper around it
    to make automated testing for various plaintexts, keys, and IVs
    */

    // pass in the plaintext as a string, second argument is the number of bytes to encrypt
    // all other bytes in the rest of the block are padded as 0s. 
    // we want to keep track of the num_blocks to print it out later
	// int num_blocks = set_plaintext("abcdef", 7);
	int num_blocks = set_plaintext("12345", 5);

	// time calculation code
	clock_t start_time;
	start_time = clock();

	// in hex
	set_aes_key(
	    "00112233445566778899AABBCCDDEEFF00112233445566778899AABBCCDDEEFF",
	    AES_KEY_SIZE_256);

	// set my 128-bit IV. This is passed in as 2 64-bit ints
	set_aes_iv(0, 0);

	run_accelerator();

    // record end time of AES
    clock_t time_taken_t;
	time_taken_t      = clock() - start_time;
	double time_taken = ((double) time_taken_t) / CLOCKS_PER_SEC;  // in seconds

	printf("AES took: %.02f ns to complete \n", time_taken * 1000000);

	for (int i = 0; i < num_blocks * 4; i++) {
		printf("ocm[%d] = 0x%08X\n", i, ocm_regs[i]);
	}

    // TODO: output the encrypted data to a file path specified as a command line argument

	unmap_regs();
}