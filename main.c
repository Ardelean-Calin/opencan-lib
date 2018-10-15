#include <Windows.h>
#include <stdio.h>
#include "OpenCAN_api.h"

const char data[] = {0x41, 0x00, 0x43, 0x44, 0x00, 0x00, 0x47, 0x48};

DWORD WINAPI ReadThreadFunction(LPVOID lpParam);
DWORD WINAPI WriteThreadFunction(LPVOID lpParam);

DWORD WINAPI ReadThreadFunction(LPVOID lpParam)
{
    HANDLE hCom = *(PHANDLE)lpParam;

    CANMsg_Standard_t rxCanMsg = {0};
    OpenCAN_ReadCAN(hCom, &rxCanMsg);

    printf("Received message:\n  ID: %x; Extended: %d; DLC: %d; Data: ", rxCanMsg.msgID, rxCanMsg.isExtended, rxCanMsg.DLC);
    for (int i = 0; i < 8; i++)
    {
        printf("%02x ", rxCanMsg.Data[i]);
    }
    printf("\n");
}

DWORD WINAPI WriteThreadFunction(LPVOID lpParam)
{
    HANDLE hCom = *(HANDLE *)lpParam;

    CANMsg_Standard_t txCanMsg = {0};
    txCanMsg.msgID = 0x201;
    txCanMsg.DLC = 8;
    memcpy((void *)txCanMsg.Data, (void *)data, 8);

    // Wait a bit for the Read Thread to start waiting
    Sleep(100);

    OpenCAN_WriteCAN(hCom, &txCanMsg);
}

int main()
{
    HANDLE hSerialPort = OpenCAN_Open("COM3");

    if (hSerialPort == NULL)
    {
        printf("Error in opening serial port\n");
        return -1;
    }
    printf("Opening serial port successful\n");

    // Delete any bytes still in the RX buffer
    PurgeComm(hSerialPort, PURGE_RXCLEAR | PURGE_TXCLEAR | PURGE_RXABORT | PURGE_TXABORT);

    HANDLE writeThreadHandle = CreateThread(NULL, 1000, WriteThreadFunction, &hSerialPort, 0, NULL);
    HANDLE readThreadHandle = CreateThread(NULL, 1000, ReadThreadFunction, &hSerialPort, 0, NULL);
    HANDLE handles[] = {readThreadHandle, writeThreadHandle};
    WaitForMultipleObjects(2, handles, TRUE, 1000);

    printf("Closing serial connection\n");

    OpenCAN_Close(hSerialPort);
    return 0;
}