
#include <malloc.h>
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

uint32_t *ocm_regs;
uint32_t *aes_regs;
uint32_t *timer_regs;
uint32_t *ps_clk_reg;
uint32_t *pl_clk_reg;

#define ADDRESS_OCM 0xFFFC0000
#define ADDRESS_AES_SLAVE 0xB0000000
#define ADDRESS_CAPTURE_TIMER_SLAVE 0xB0002000

#define AES_KEY_SIZE_128 0b00
#define AES_KEY_SIZE_192 0b01
#define AES_KEY_SIZE_256 0b10

uint32_t timer_value;

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
main(int argc, char *argv[]) {
	srand(time(0));

	if (map_regs() == -1) {
		printf("Must be ROOT to open /dev/mem\n");
		return 1;
	}

    // if (argc != 3) {
    //     printf("Usage: -s \"string\" or -f \"file.txt\"\n");
    //     return 1;    
    // }

    // if (strcmp(argv[1], "-s") != 0 && strcmp(argv[1], "-f") != 0) {
    //     printf("Usage: -s \"string\" or -f \"file.txt\"\n");
    //     return 1;
    // }

    // char *test_string;
    // uint16_t test_string_length;
    // if (strcmp(argv[1], "-s") == 0) {
    //     printf("String: \'%s\'\n", argv[2]);
	// 	test_string_length = strlen(argv[2]);
    //     test_string = argv[2];
	// } else {
    //     printf("File: %s\n", argv[2]);
    //     FILE *file = fopen(argv[2], "r");
    //     if (file == NULL) {
    //         printf("Error opening file\n");
    //         return 1;
    //     }

    //     fseek(file, 0, SEEK_END);
    //     test_string_length = ftell(file);

    //     test_string = malloc(test_string_length);
    //     fseek(file, 0, SEEK_SET);
    //     fread(test_string, 1, test_string_length, file);
    //     fclose(file);
    // }

	set_clock(PS_CLK_1499_MHZ, PL_CLK_300_MHZ);
	setup_capture_timer_interrupt();

	for (int i = 0; i < 128/8; i++) {
        ocm_regs[i] = 0;
        printf("ocm[%d] = 0x%08X\n", i, ocm_regs[i]);
    }

    // in hex
    char aes_key[] = "00112233445566778899AABBCCDDEEFF";

    // regs 3-10 are for 256 bit key, 7-10 are for 128
	aes_regs[7] = (s_to_h(&aes_key[0]) << 24) | (s_to_h(&aes_key[2]) << 16) | (s_to_h(&aes_key[4]) << 8) | s_to_h(&aes_key[6]);
    aes_regs[8] = (s_to_h(&aes_key[8]) << 24) | (s_to_h(&aes_key[10]) << 16) | (s_to_h(&aes_key[12]) << 8) | s_to_h(&aes_key[14]);
	aes_regs[9] = (s_to_h(&aes_key[16]) << 24) | (s_to_h(&aes_key[18]) << 16) |
	              (s_to_h(&aes_key[20]) << 8) | s_to_h(&aes_key[22]);
	aes_regs[10] = (s_to_h(&aes_key[24]) << 24) | (s_to_h(&aes_key[26]) << 16) |
	               (s_to_h(&aes_key[28]) << 8) | s_to_h(&aes_key[30]);
                   
	// set IV
    uint64_t iv = 0x0;
	aes_regs[11] = iv >> 32;
	aes_regs[12] = iv & 0xFFFFFFFF;

	// set number_blocks
	aes_regs[2] = 2;

	// reset
	aes_regs[0] = 0b1;
	aes_regs[0] = 0b0;

    // set key size
    aes_regs[0] = (AES_KEY_SIZE_128 << 4);

	// start
	aes_regs[0] |= 0b10;

	// wait until aes done
	while (!(aes_regs[1] & 0b10)) {}

	for (int i = 0; i < 128/8; i++) {
        ocm_regs[i] = 0;
        printf("ocm[%d] = 0x%08X\n", i, ocm_regs[i]);
    }

	// for (int i = 16; i < 32; i++) { printf("0x%08X\n", aes_regs[i]); }
	// printf("\nTimer value: %d nanoseconds\n", timer_value);

	// aes_regs[0] = 0b00;

	unmap_regs();
}