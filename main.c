#include <Windows.h>
#include <stdio.h>
#include "OpenCAN_api.h"


int main()
{
    char* Buf = "\xDEThis is a test.\xAD";
    HANDLE hComm = OpenCAN_Open("COM3");

    char data[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};

    CANMsg_Standard_t canMsg;
    CANMsg_Standard_t *pCanMsg;
    pCanMsg = &canMsg;

    pCanMsg->msgID = 0x201;
    pCanMsg->DLC = 8;
    memcpy((void*) pCanMsg->Data, (void*) data, 8);

    if (hComm == NULL)
        printf("Error in opening serial port");
    else
        printf("opening serial port successful");

    OpenCAN_WriteCAN(hComm, pCanMsg);

    CloseHandle(hComm);  //Closing the Serial Port

    return 0;
}