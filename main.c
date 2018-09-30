#include <Windows.h>
#include <stdio.h>
#include "OpenCAN_api.h"

const char data[] = {0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48};

int main()
{
    uint8_t receiveBuffer[RX_MSG_RAW_SIZE];
    CANMsg_Standard_t canMsg;
    CANMsg_Standard_t *pCanMsg;

    pCanMsg = &canMsg;
    pCanMsg->msgID = 0x201;
    pCanMsg->DLC = 8;
    memcpy((void *)pCanMsg->Data, (void *)data, 8);

    HANDLE hComm = OpenCAN_Open("COM3");

    if (hComm == NULL)
    {
        printf("Error in opening serial port\n");
        return -1;
    }
    printf("Opening serial port successful\n");

    PurgeComm(hComm, PURGE_RXCLEAR | PURGE_TXCLEAR | PURGE_RXABORT | PURGE_TXABORT);

    OpenCAN_WriteCAN(hComm, pCanMsg);
    uint8_t result = OpenCAN_ReadCAN(hComm, receiveBuffer);

    for (int i = 0; i < RX_MSG_RAW_SIZE; i++)
    {
        printf("%x ", receiveBuffer[i]);
    }

    printf("\nClosing serial port");
    CloseHandle(hComm); //Closing the Serial Port

    return 0;
}