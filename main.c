#include <Windows.h>
#include <stdio.h>
#include "OpenCAN_api.h"

const char data[] = {0x41, 0x00, 0x43, 0x44, 0x00, 0x00, 0x47, 0x48};

DWORD WINAPI ReadThreadFunction(LPVOID lpParam);

DWORD WINAPI ReadThreadFunction(LPVOID lpParam)
{
    uint32_t i = 0;
    for (;;)
    {
        printf("Sunt in thread: %d\n", i);
        Sleep(500);
        i++;
    }
}

int main()
{
    CANMsg_Standard_t txCanMsg;
    CANMsg_Standard_t rxCanMsg;

    txCanMsg.msgID = 0x201;
    txCanMsg.DLC = 8;
    memcpy((void *)txCanMsg.Data, (void *)data, 8);

    HANDLE hComm = OpenCAN_Open("COM7");

    if (hComm == NULL)
    {
        printf("Error in opening serial port\n");
        return -1;
    }
    printf("Opening serial port successful\n");

    PurgeComm(hComm, PURGE_RXCLEAR | PURGE_TXCLEAR | PURGE_RXABORT | PURGE_TXABORT);

    OpenCAN_WriteCAN(hComm, &txCanMsg);
    uint8_t error = OpenCAN_ReadCAN(hComm, &rxCanMsg);

    if (!error)
    {
        printf("Received message:\n  ID: %x; DLC: %d; Data: ", rxCanMsg.msgID, rxCanMsg.DLC);

        for (int i = 0; i < 8; i++)
        {
            printf("%02x ", rxCanMsg.Data[i]);
        }
    }

    printf("\nClosing serial port\n");
    CloseHandle(hComm); //Closing the Serial Port
    // HANDLE threadHandle = CreateThread(NULL, 0, ReadThreadFunction, NULL, 0, NULL);
    // WaitForSingleObject(threadHandle, INFINITE);

    return 0;
}