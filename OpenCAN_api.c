#include "cobs.h"
#include "OpenCAN_api.h"
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>

/** 
 * Orders the data in the circular buffer.
 */
static void vExtractDataCircularBuffer(uint8_t *src, uint8_t *dst, uint8_t i, uint8_t len)
{
    uint8_t upperBytesNo = len - 1 - i;
    memcpy(dst, &src[i + 1], upperBytesNo);
    memcpy(&dst[upperBytesNo], src, i + 1);
}

static void vBuildCANFrameFromData(uint8_t *data, CANMsg_Standard_t *msg)
{
    // Create the CAN message from the received data
    memcpy(msg->Data, &data[2], 8U);
    msg->DLC = data[0] >> 4;
    msg->msgID = ((data[0] & 0x0F) << 8) + data[1];
}

// Returns the number of bytes written
static uint8_t OpenCAN_Write(HANDLE hComm, uint8_t *Buf, uint8_t Len)
{
    DWORD bytesWritten;
    if (WriteFile(hComm, (void *)Buf, (DWORD)Len, &bytesWritten, NULL))
        return (uint8_t)bytesWritten;
    else
        return 0;
}

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
    // TODO: Maybe I also need a transmit timeout?
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

void OpenCAN_WriteCAN(HANDLE hComm, CANMsg_Standard_t *txMsg)
{
    uint8_t rawMsg[TX_MSG_DEC_SIZE];
    uint8_t encodedMsg[TX_MSG_ENC_SIZE];

    // Command to execute on the OpenCAN
    rawMsg[0] = CAN_WRITE_MSG;
    // TODO: Change DLC and MSGID to be same format as receive
    // Address upper byte
    rawMsg[1] = txMsg->msgID >> 4;
    // Address lower 3 bits and DLC
    rawMsg[2] = ((txMsg->msgID & 0b00000001111) << 4) + txMsg->DLC;
    // Copy data buffer in preparation for send
    memcpy((void *)&rawMsg[3], txMsg->Data, 8);

    // Encode data using COBS
    StuffData(rawMsg, TX_MSG_DEC_SIZE, encodedMsg);
    encodedMsg[TX_MSG_ENC_SIZE - 1] = 0x00;

    OpenCAN_Write(hComm, (uint8_t *)encodedMsg, TX_MSG_ENC_SIZE);
}

/**
 * The simplest possible Serial Read function. May need to adapt to be able to
 * read while transmitting & not read transmitted bytes.
 */
uint8_t OpenCAN_ReadCAN(HANDLE hComm, CANMsg_Standard_t *rxMsg)
{
    uint8_t ucReadResult = 0U;
    uint8_t ucBufferIndex = 0U;
    uint8_t pucCircularBuffer[RX_MSG_ENC_SIZE];
    uint8_t pucEncodedData[RX_MSG_ENC_SIZE];
    uint8_t pucDecodedData[RX_MSG_DEC_SIZE];

    // Infinite loop: read bytes until EOF (0x00)
    for (;;)
    {
        ucReadResult = ReadFile(hComm, &pucCircularBuffer[ucBufferIndex], 1U, NULL, NULL);
        if (!ucReadResult)
            return 1U;

        if (pucCircularBuffer[ucBufferIndex] == 0U)
        {
            // We got the EOF character; Decode COBS frame and build CAN frame
            vExtractDataCircularBuffer(pucCircularBuffer, pucEncodedData, ucBufferIndex, RX_MSG_ENC_SIZE);
            UnStuffData(pucEncodedData, RX_MSG_ENC_SIZE, pucDecodedData);
            vBuildCANFrameFromData(pucDecodedData, rxMsg);
            return 0U;
        }

        ucBufferIndex = (ucBufferIndex + 1) % RX_MSG_ENC_SIZE;
    }
}