#include <stdio.h>
#include <string.h>
#include <switch.h>
#include <time.h>

#include "sha2.h"
#include "graphics.h"


uint32_t logo_offset = 40;
uint8_t bytes;
const uint8_t CMD_BLOCK_HEADER = 0x41; //A
const uint8_t CMD_TARGET = 0x42; //B
const uint8_t CMD_NEW_BLOCK = 0x43; //C
const uint8_t CMD_STATUS = 0x44; //D
const uint8_t RESP_WAIT_CMD = 0x61; //a
const uint8_t RESP_WAIT_DATA = 0x62; //b
const uint8_t RESP_SUCCESS = 0x63; //c
const uint8_t ZERO = 0x0;


size_t transport_safe_write(const void *buffer, size_t size)
{
    const u8 *bufptr = (const u8 *)buffer;
    size_t cursize = size;
    size_t tmpsize = 0;

    while (cursize)
    {
        tmpsize = usbCommsWrite(bufptr, cursize);
        bufptr += tmpsize;
        cursize -= tmpsize;
    }

    return size;
}

size_t transport_safe_read(void *buffer, size_t size)
{
    u8 *bufptr = buffer;
    size_t cursize = size;
    size_t tmpsize = 0;

    while (cursize)
    {
        tmpsize = usbCommsRead(bufptr, cursize);
        bufptr += tmpsize;
        cursize -= tmpsize;
    }

    return size;
}

void sb(unsigned char c) {
    transport_safe_read(&bytes, sizeof(bytes));
    transport_safe_write(&c, sizeof(c));

}

int receive_bytes(unsigned char *header, size_t len) {
    for(int i = 0; i < len; i++) {
      transport_safe_read(&header[i], sizeof(header[i]));
      if (i == 1) {
        printf("   Receiving Bytes...\n\n");
        consoleUpdate(NULL);
      }
      transport_safe_write(&RESP_WAIT_DATA, sizeof(RESP_WAIT_DATA));
    }
    return 0;
}

int receive_block_header(unsigned char *header) {
    for(int i = 0; i < 76; i++) {
      transport_safe_read(&header[i], sizeof(header[i]));
      if (i == 1) {
        printf("   Receiving Header...\n\n");
        consoleUpdate(NULL);
      }
      transport_safe_write(&RESP_WAIT_DATA, sizeof(RESP_WAIT_DATA));
    }
    return 0;
}

int receive_height(unsigned char *height) {
  for(int i = 0; i < 16; i++) {
      transport_safe_read(&height[i], sizeof(height[i]));
      if (i == 1) {
        printf("   Receiving Height...\n\n");
        consoleUpdate(NULL);
      }
      transport_safe_write(&RESP_WAIT_DATA, sizeof(RESP_WAIT_DATA));
    }
  return 0;
}

void receive_block_data(unsigned char *block_header, unsigned char *target, unsigned char *height) {
    receive_block_header(block_header);
    receive_bytes(target, 32);
    receive_height(height);
}

int mine_nonce(unsigned char *block_header, unsigned char *target, uint32_t *nonce_out, PadState *pad) {
    uint32_t nonce = *nonce_out;
    transport_safe_read(&bytes, sizeof(bytes));
    transport_safe_write(&ZERO, sizeof(ZERO));
    time_t timerMining;
    time(&timerMining);

    time_t timerHashrate;
    time(&timerHashrate);
    uint32_t lastNonce = nonce;
    int hashrate = 0;
    while(1) {

        // Append nonce
        memcpy(&block_header[76], &nonce, 4);

        // Increase nonce
        nonce++;
        
        uint8_t hash_rev[32];
        uint8_t hash2[32];
        uint8_t hash[32];
        calc_sha_256(hash2, block_header, 80);
        calc_sha_256(hash_rev, hash2, 32);
        for(int i=0; i < 32; i++) {
            hash[31-i] = hash_rev[i];
        }

        // Check hash...
        for(int i=0; i < 32; i++) {
            if(hash[i] < target[i]) {
                printf("   Success !\n");
                *nonce_out = nonce - 1;
                return 0;
            } else if(hash[i] > target[i]) {
                break;
            }
        }
    
        
        // Check if we need to receive a new block.
        time_t difftimer;
        time(&difftimer);
        int secondsMining = difftime(difftimer, timerMining);
        if (secondsMining > 120){
          transport_safe_read(&bytes, sizeof(bytes));
          transport_safe_write(&ZERO, sizeof(ZERO));
        }

        //Compute Hashrate
        int secondsHashrate = difftime(difftimer, timerHashrate);
        if (secondsHashrate > 3) {
          consoleClear();
          int totalNonces = nonce - lastNonce;
          lastNonce = nonce;
          time(&timerHashrate);
          hashrate = totalNonces / 3;
          printHashrate(hashrate);
          consoleUpdate(NULL);
        }

        if(bytes == CMD_NEW_BLOCK) {
            printf("   Checking with ntgbtminer...\n\n");
            consoleUpdate(NULL);
            *nonce_out = nonce;
            return 1;
        }

        //Check user input
        padUpdate(pad);

        u64 kDown = padGetButtonsDown(pad);

        if (kDown & HidNpadButton_Plus){
          return 99;
        }
      }
}


void miner(PadState *pad) {
    unsigned char block_header[80];
    unsigned char target[32];
    unsigned char height[16];
    unsigned char old_height[16];
    uint32_t nonce = 0;

    while(1) {
        receive_block_data(block_header, target, height);

        if (memcmp(old_height, height, 16) == 0){
          printf("   No new block.\n\n");
        } else {
          printf("   New block data !\n\n");
          nonce = 0;
        }
        int mine_result = mine_nonce(block_header, target, &nonce, pad);
        printf("   Nonce : %d\n\n", nonce);
        memcpy(old_height, height, sizeof(old_height));

        // Indicate success to host
        if(mine_result == 0) {
            transport_safe_read(&bytes, sizeof(bytes));
            transport_safe_write(&RESP_SUCCESS, sizeof(RESP_SUCCESS));
            printf("   Sending status...\n");
            // send nonce over.
            char data[4];
            memcpy(data, &nonce, 4);
            for(int i=0; i < 4; i++) {
                sb(data[i]);
            }
        }

        if(mine_result == 99) {
          break;
        }
    }
}
