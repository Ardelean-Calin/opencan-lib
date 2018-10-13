#include <Windows.h>
#include <stdio.h>
#include "OpenCAN_api.h"

const char data[] = {0x41, 0x00, 0x43, 0x44, 0x00, 0x00, 0x47, 0x48};

DWORD WINAPI ReadThreadFunction(LPVOID lpParam);
DWORD WINAPI WriteThreadFunction(LPVOID lpParam);

DWORD WINAPI ReadThreadFunction(LPVOID lpParam)
{
    volatile HANDLE hCom = *(PHANDLE)lpParam;
    printf("Entered read thread...\n");
    CANMsg_Standard_t rxCanMsg = {0};
    OpenCAN_ReadCAN(hCom, &rxCanMsg);
    volatile DWORD test = GetLastError();
    printf("Error: %d\n", test);

    printf("Received message:\n  ID: %x; DLC: %d; Data: ", rxCanMsg.msgID, rxCanMsg.DLC);

    for (int i = 0; i < 8; i++)
    {
        printf("%02x ", rxCanMsg.Data[i]);
    }

    printf("ThreadRead: Read completed...\n");
}

DWORD WINAPI WriteThreadFunction(LPVOID lpParam)
{
    HANDLE hCom = *(HANDLE *)lpParam;

    CANMsg_Standard_t txCanMsg;
    txCanMsg.msgID = 0x201;
    txCanMsg.DLC = 8;
    memcpy((void *)txCanMsg.Data, (void *)data, 8);
    Sleep(1000);
    OpenCAN_WriteCAN(hCom, &txCanMsg);
}

int main()
{
    volatile CANMsg_Standard_t txCanMsg;
    volatile CANMsg_Standard_t rxCanMsg;

    txCanMsg.msgID = 0x201;
    txCanMsg.DLC = 8;
    memcpy((void *)txCanMsg.Data, (void *)data, 8);

    volatile HANDLE hComm = OpenCAN_Open("COM3");

    if (hComm == NULL)
    {
        printf("Error in opening serial port\n");
        return -1;
    }
    printf("Opening serial port successful\n");

    PurgeComm(hComm, PURGE_RXCLEAR | PURGE_TXCLEAR | PURGE_RXABORT | PURGE_TXABORT);

    HANDLE writeThreadHandle = CreateThread(NULL, 0, WriteThreadFunction, &hComm, 0, NULL);
    HANDLE readThreadHandle = CreateThread(NULL, 0, ReadThreadFunction, &hComm, 0, NULL);
    HANDLE handles[] = {readThreadHandle, writeThreadHandle};
    WaitForMultipleObjects(2, handles, FALSE, INFINITE);

    // Does it block?
    // OpenCAN_WriteCAN(hComm, &txCanMsg);
    // OpenCAN_ReadCAN(hComm, &rxCanMsg);

    // OpenCAN_WriteCAN(hComm, &txCanMsg);
    // uint8_t error = OpenCAN_ReadCAN(hComm, &rxCanMsg);

    // if (!error)
    // {
    // printf("\nReceived message:\n  ID: %x; DLC: %d; Data: ", rxCanMsg.msgID, rxCanMsg.DLC);
    // for (int i = 0; i < 8; i++)
    // {
    //     printf("%02x ", rxCanMsg.Data[i]);
    // }
    // }

    // printf("\nClosing serial port\n");
    // CloseHandle(hComm); //Closing the Serial Port
    // // WaitForSingleObject(threadHandle, INFINITE);
    // WaitForMultipleObjects(2, handles, FALSE, INFINITE);
    // Sleep(1000);
    CloseHandle(hComm);

    return 0;
}