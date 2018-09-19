#include "OpenCAN_api.h"
#include <stdio.h>


void* OpenCAN_Open(char* portName){
    HANDLE hComm;
    hComm = CreateFile( portName,                       //port name
                        GENERIC_READ | GENERIC_WRITE,   //Read/Write
                        0,                              // No Sharing
                        NULL,                           // No Security
                        OPEN_EXISTING,                  // Open existing port only
                        0,                              // Non Overlapped I/O
                        NULL);                          // Null for Comm Devices
    if (hComm == INVALID_HANDLE_VALUE)
        return NULL;
    else
        return hComm;
}

void OpenCAN_Close(HANDLE hComm){
    CloseHandle(hComm);
}

// Returns the number of bytes written
uint8_t OpenCAN_Write(HANDLE hComm, uint8_t* Buf, uint8_t Len){
    DWORD bytesWritten;
    if (WriteFile(hComm, (void*) Buf, (DWORD) Len, &bytesWritten, NULL))
        return (uint8_t) bytesWritten;
    else
        return 0;
}

void OpenCAN_WriteCAN(HANDLE hComm, CANMsg_Standard_t *txMsg){
    uint8_t rawMsg[MSG_RAW_SIZE];

    // Set start and end bytes
    rawMsg[0] = MSG_START_BYTE;
    rawMsg[MSG_RAW_SIZE - 1] = MSG_END_BYTE;

    // Command to execute on the OpenCAN
    rawMsg[1] = CAN_WRITE_MSG;
    // Address upper byte
    rawMsg[2] = txMsg->msgID >> 4;
    // Address lower 3 bits and DLC
    rawMsg[3] = ((txMsg->msgID & 0b00000001111) << 4) + txMsg->DLC;
    // Copy data buffer in preparation for send
    memcpy((void*) &rawMsg[4], txMsg->Data, 8);

    OpenCAN_Write(hComm, (uint8_t*) &rawMsg, MSG_RAW_SIZE);
}