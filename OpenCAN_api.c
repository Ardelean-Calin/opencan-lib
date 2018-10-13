#include "cobs.h"
#include "OpenCAN_api.h"
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>

// Function prototypes
static void vExtractDataCircularBuffer(uint8_t *src, uint8_t *dst, uint8_t i, uint8_t len);
static void vBuildCANFrameFromData(uint8_t *data, CANMsg_Standard_t *msg);
static uint8_t ucWriteToFile(HANDLE hComm, uint8_t *Buf, uint8_t Len);
static uint8_t ucReadFromFile(HANDLE hComm, uint8_t *pucDest, uint8_t ucBytesToRead);
void vSerializeMessage(CANMsg_Standard_t *src, uint8_t *dest);
void *OpenCAN_Open(char *portName);
void OpenCAN_Close(HANDLE hComm);
// Global variables

void *OpenCAN_Open(char *portName)
{
    HANDLE hComm;
    hComm = CreateFile(portName,                     //port name
                       GENERIC_READ | GENERIC_WRITE, //Read/Write
                       0,                            // No Sharing
                       0,                            // No Security
                       OPEN_EXISTING,                // Open existing port only
                       FILE_FLAG_OVERLAPPED,         // Overlapped I/O
                       NULL);                        // Null for Comm Devices

    COMMTIMEOUTS commTimeouts;
    GetCommTimeouts(hComm, &commTimeouts);
    commTimeouts.ReadIntervalTimeout = MAXDWORD;
    commTimeouts.ReadTotalTimeoutConstant = MAXDWORD;
    commTimeouts.ReadTotalTimeoutMultiplier = MAXDWORD;
    SetCommTimeouts(hComm, &commTimeouts);

    // Receive events on byte receive
    if (!SetCommMask(hComm, EV_RXCHAR))
        return NULL;

    if (hComm == INVALID_HANDLE_VALUE)
        return NULL;
    else
        return hComm;
}

void OpenCAN_Close(HANDLE hComm)
{
    CloseHandle(hComm);
}

/**
 * Blocks until we send a CAN Message.
 */
uint8_t OpenCAN_WriteCAN(HANDLE hComm, CANMsg_Standard_t *txMsg)
{
    uint8_t rawMsg[TX_MSG_DEC_SIZE];
    uint8_t encodedMsg[TX_MSG_ENC_SIZE];
    uint8_t bytesWritten;

    // Transform our message structure into a byte array
    vSerializeMessage(txMsg, &rawMsg[0]);
    // Encode data using COBS
    StuffData(rawMsg, TX_MSG_DEC_SIZE, encodedMsg);
    // Put the EOF as the last byte
    encodedMsg[TX_MSG_ENC_SIZE - 1] = 0x00;

    // Send the data off to be written when time allows it. ucWriteToFile blocks
    // until data was sent or until timeout.
    bytesWritten = ucWriteToFile(hComm, (uint8_t *)encodedMsg, TX_MSG_ENC_SIZE);
    if (!bytesWritten)
        return 1U;
    else
        return 0U;
}

/**
 * Blocks until we receive a correctly formatted RX Message.
 */
uint8_t OpenCAN_ReadCAN(HANDLE hComm, CANMsg_Standard_t *rxMsg)
{
    uint8_t ucBufferIndex;
    uint8_t ucBytesRead;
    uint8_t ucBytesReadTotal;
    uint8_t pucCircularBuffer[RX_MSG_ENC_SIZE];
    uint8_t pucEncodedData[RX_MSG_ENC_SIZE];
    uint8_t pucDecodedData[RX_MSG_DEC_SIZE];

    // We initialize these variables
    ucBufferIndex = 0U;
    ucBytesRead = 0U;
    ucBytesReadTotal = 0U;

    do
    {
        ucBytesRead = ucReadFromFile(hComm, &pucCircularBuffer[ucBufferIndex], 1U);
        ucBytesReadTotal += ucBytesRead;
        if (ucBytesRead)
        {
            // The EOF byte is 0x00 - Read until it comes
            if (pucCircularBuffer[ucBufferIndex] == 0x00U)
            {
                vExtractDataCircularBuffer(pucCircularBuffer, pucEncodedData, ucBufferIndex, RX_MSG_ENC_SIZE);
                UnStuffData(pucEncodedData, RX_MSG_ENC_SIZE, pucDecodedData);
                vBuildCANFrameFromData(pucDecodedData, rxMsg);
                break;
            }

            // Increment the ringbuffer index
            ucBufferIndex = (ucBufferIndex + 1) % RX_MSG_ENC_SIZE;
        }

    } while (1);

    // Return the number of bytes read in total
    return ucBytesReadTotal;
}

void vSerializeMessage(CANMsg_Standard_t *src, uint8_t *dest)
{
    // Command to execute on the OpenCAN
    dest[0] = CAN_WRITE_MSG;
    // TODO: Change DLC and MSGID to be same format as receive
    // Address upper byte
    dest[1] = src->msgID >> 4;
    // Address lower 3 bits and DLC
    dest[2] = ((src->msgID & 0b00000001111) << 4) + src->DLC;
    // Copy data buffer in preparation for send
    memcpy((void *)&dest[3], src->Data, 8);
}

// Returns the number of bytes written
static uint8_t ucWriteToFile(HANDLE hComm, uint8_t *Buf, uint8_t Len)
{
    DWORD bytesWritten;
    DWORD dwRes;
    OVERLAPPED osWrite = {0};
    osWrite.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    if (osWrite.hEvent == NULL)
        // error creating osWrite event handle
        return 0U;

    if (!WriteFile(hComm, (void *)Buf, (DWORD)Len, &bytesWritten, &osWrite))
    {
        // Error while writing file.
        if (GetLastError() != ERROR_IO_PENDING)
        {
            bytesWritten = 0U;
        }
        else
        {
            // Write is pending, so wait for it indefinitely (?)
            dwRes = WaitForSingleObject(osWrite.hEvent, INFINITE);
            switch (dwRes)
            {
            case WAIT_OBJECT_0:
                if (!GetOverlappedResult(hComm, &osWrite, (LPDWORD)&bytesWritten, FALSE))
                {
                    // Error transfering
                    bytesWritten = 0U;
                }
                break;
            default:
                bytesWritten = 0U;
                break;
            }
        }
    }

    CloseHandle(osWrite.hEvent);
    return (uint8_t)bytesWritten;
}

static uint8_t ucReadFromFile(HANDLE hComm, uint8_t *pucDest, uint8_t ucBytesToRead)
{
    DWORD bytesRead = 0U;
    BOOL fWaitingOnRead = FALSE;

    OVERLAPPED osReader = {0};
    osReader.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (osReader.hEvent == NULL)
        // Error creating overlapped event; abort.
        return 0U;

    // Start a read operation
    if (!fWaitingOnRead)
    {
        // Issue read operation.
        if (!ReadFile(hComm, pucDest, ucBytesToRead, &bytesRead, &osReader))
        {
            if (GetLastError() != ERROR_IO_PENDING) // read not delayed?
                // Error in communications; report it.
                return 0U;
            else
                fWaitingOnRead = TRUE;
        }
        else
        {
            // read completed immediately
            CloseHandle(osReader.hEvent);
            return (uint8_t)bytesRead;
        }
    }

    DWORD dwRes;

    if (fWaitingOnRead)
    {
        dwRes = WaitForSingleObject(osReader.hEvent, INFINITE);
        switch (dwRes)
        {
        // Read completed.
        case WAIT_OBJECT_0:
            if (!GetOverlappedResult(hComm, &osReader, &bytesRead, FALSE))
                // Error in communications; report it.
                return 0U;
            else
            {
                // Read completed successfully.
                CloseHandle(osReader.hEvent);
                return (uint8_t)bytesRead;
            }

            //  Reset flag so that another opertion can be issued.
            fWaitingOnRead = FALSE;
            break;

        case WAIT_TIMEOUT:
            // Operation isn't complete yet. fWaitingOnRead flag isn't
            // changed since I'll loop back around, and I don't want
            // to issue another read until the first one finishes.
            //
            // This is a good time to do some background work.
            break;

        default:
            // Error in the WaitForSingleObject; abort.
            // This indicates a problem with the OVERLAPPED structure's
            // event handle.
            break;
        }
    }

    CloseHandle(osReader.hEvent);
    return (uint8_t)bytesRead;
}

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