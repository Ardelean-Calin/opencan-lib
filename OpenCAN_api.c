#include "OpenCAN_api.h"
#include <stdio.h>

void *OpenCAN_Open(char *portName)
{
    HANDLE hComm;
    hComm = CreateFile(portName,                     //port name
                       GENERIC_READ | GENERIC_WRITE, //Read/Write
                       0,                            // No Sharing
                       0,                            // No Security
                       OPEN_EXISTING,                // Open existing port only
                       0,                            // Non-Overlapped I/O
                       0);                           // Null for Comm Devices

    // Set a 10ms timeout between bytes and make the read function blocking.
    // Will transmission work while waiting for receive? TODO
    COMMTIMEOUTS commTimeouts;
    GetCommTimeouts(hComm, &commTimeouts);
    commTimeouts.ReadIntervalTimeout = 10;            // More than 10ms between bytes is a timeout
    commTimeouts.ReadTotalTimeoutConstant = MAXDWORD; // Infinite wait time for message
    commTimeouts.ReadTotalTimeoutMultiplier = MAXDWORD;
    SetCommTimeouts(hComm, &commTimeouts);

    if (hComm == INVALID_HANDLE_VALUE)
        return NULL;
    else
        return hComm;
}

void OpenCAN_Close(HANDLE hComm)
{
    CloseHandle(hComm);
}

// Returns the number of bytes written
uint8_t OpenCAN_Write(HANDLE hComm, uint8_t *Buf, uint8_t Len)
{
    DWORD bytesWritten;
    if (WriteFile(hComm, (void *)Buf, (DWORD)Len, &bytesWritten, NULL))
        return (uint8_t)bytesWritten;
    else
        return 0;
}

void OpenCAN_WriteCAN(HANDLE hComm, CANMsg_Standard_t *txMsg)
{
    uint8_t rawMsg[TX_MSG_RAW_SIZE];

    // Set start and end bytes
    rawMsg[0] = TX_MSG_START_BYTE;
    rawMsg[TX_MSG_RAW_SIZE - 1] = TX_MSG_END_BYTE;

    // Command to execute on the OpenCAN
    rawMsg[1] = CAN_WRITE_MSG;
    // Address upper byte
    rawMsg[2] = txMsg->msgID >> 4;
    // Address lower 3 bits and DLC
    rawMsg[3] = ((txMsg->msgID & 0b00000001111) << 4) + txMsg->DLC;
    // Copy data buffer in preparation for send
    memcpy((void *)&rawMsg[4], txMsg->Data, 8);

    OpenCAN_Write(hComm, (uint8_t *)&rawMsg, TX_MSG_RAW_SIZE);
}

/**
 * The simplest possible Serial Read function. May need to adapt to be able to
 * read while transmitting & not read transmitted bytes.
 */
uint8_t OpenCAN_ReadCAN(HANDLE hComm, uint8_t *readBuffer)
{
    uint8_t result = ReadFile(hComm, readBuffer, RX_MSG_RAW_SIZE, NULL, NULL);
    if (result)
    {
        return 0U;
    }
    else
    {
        return 1U;
    }
}