#include <windows.h>
#include <stdio.h>
#include <stdint-gcc.h>
#include "OpenCAN_api.h"

uint8_t data[8];

int main(int argc, char const *argv[])
{
    /* code */
    HANDLE hSerialPort;
    uint8_t comNumber;
    uint8_t error = OpenCAN_Open(&hSerialPort, &comNumber);
    if (error)
    {
        printf("Error in opening serial port\n");
        return -1;
    }
    printf("Opening serial port successful: COM%d\n", comNumber);

    // Start with everything 0
    memset(data, 0x00U, 8);

    CANMsg_Standard_t txCanMsg = {0};
    txCanMsg.msgID = 0x201;
    txCanMsg.DLC = 8;

    uint64_t i = 0xDEADBEEFCAFE0000;
    memcpy((void *)txCanMsg.Data, (void *)&i, 8);
    for (uint64_t i = 0; i < 9; i+=3)
    {
        /* code */
        OpenCAN_SetBaudrate(hSerialPort, i);
        memcpy((void *)txCanMsg.Data, (void *)&i, 8);
        OpenCAN_WriteCAN(hSerialPort, &txCanMsg);
        printf("Going %d", i);
        Sleep(1U);
    }

    OpenCAN_Close(hSerialPort);

    return 0;
}
