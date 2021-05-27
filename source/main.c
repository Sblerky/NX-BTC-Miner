// Include the most common headers from the C standard library
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Include the main libnx system header, for Switch development
#include <switch.h>
#include <miner.h>
#include <graphics.h>

// Main program entrypoint
int main(int argc, char* argv[])
{
    consoleInit(NULL);

    padConfigureInput(1, HidNpadStyleSet_NpadStandard);

    PadState pad;
    padInitializeDefault(&pad);
    printMenu();
    consoleUpdate(NULL);

    usbCommsInitialize();

    miner(&pad);

    usbCommsExit();

    // Deinitialize and clean up resources used by the console (important!)
    consoleExit(NULL);
    return 0;
}
