#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void printHeader() {
  printf("=============================== NX Bitcoin Miner ===============================\n");
  printf("================================== By Sblerky ==================================\n");
  printf("==================================== V 1.0.0 ===================================\n");
}

void printMenu(){
  printHeader();
  printf("\n\n");
  printf("   Waiting for ntgbtminer to send data...\n\n");
}

void printHashrate(int hashrate){
  printHeader();
  printf("\n\n");
  printf("   Hashrate : %d hashes / second\n", hashrate);
}