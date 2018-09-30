#include <Windows.h>
#include <stdio.h>
#include "OpenCAN_api.h"

const char data[] = {0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48};

int main()
{
    CANMsg_Standard_t txCanMsg;
    CANMsg_Standard_t rxCanMsg;

    txCanMsg.msgID = 0x201;
    txCanMsg.DLC = 8;
    memcpy((void *)txCanMsg.Data, (void *)data, 8);

    HANDLE hComm = OpenCAN_Open("COM3");

    if (hComm == NULL)
    {
        printf("Error in opening serial port\n");
        return -1;
    }
    printf("Opening serial port successful\n");

    PurgeComm(hComm, PURGE_RXCLEAR | PURGE_TXCLEAR | PURGE_RXABORT | PURGE_TXABORT);

    OpenCAN_WriteCAN(hComm, &txCanMsg);
    uint8_t error = OpenCAN_ReadCAN(hComm, &rxCanMsg);

    if (error)
    {
        printf("Error reading from OpenCAN.");
        return -1;
    }

    printf("Received message:\n  ID: %x; DLC:%x; Data: ", rxCanMsg.msgID, rxCanMsg.DLC);

    for (int i = 0; i < 8; i++)
    {
        printf("%x ", rxCanMsg.Data[i]);
    }

    printf("\nClosing serial port");
    CloseHandle(hComm); //Closing the Serial Port

    return 0;
}