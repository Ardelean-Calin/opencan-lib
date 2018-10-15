#include "cobs.h"
#include "OpenCAN_api.h"
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>

// Function prototypes
static void vExtractDataCircularBuffer(uint8_t *src, uint8_t *dst, uint8_t i, uint8_t len);
static void vBuildCANFrameFromData(uint8_t *data, CANMsg_Standard_t *msg);
static uint8_t ucWriteToFile(HANDLE hSerialPort, uint8_t *Buf, uint8_t Len);
static uint8_t ucReadFromFile(HANDLE hSerialPort, uint8_t *pucDest, uint8_t ucBytesToRead);
void vSerializeMessage(CANMsg_Standard_t *src, uint8_t *dest);
void *OpenCAN_Open(char *portName);
void OpenCAN_Close(HANDLE hSerialPort);
// Global variables

void *OpenCAN_Open(char *portName)
{
    HANDLE hSerialPort;
    hSerialPort = CreateFile(portName,                     //port name
                             GENERIC_READ | GENERIC_WRITE, //Read/Write
                             0,                            // No Sharing
                             0,                            // No Security
                             OPEN_EXISTING,                // Open existing port only
                             FILE_FLAG_OVERLAPPED,         // Overlapped I/O
                             NULL);                        // Null for Comm Devices

    // Set baudrate. VERY important for Read, even though we don't
    // need to specify it on the uC
    DCB dcb;
    GetCommState(hSerialPort, &dcb);
    dcb.BaudRate = CBR_256000;
    dcb.ByteSize = 8;
    dcb.Parity = NOPARITY;
    dcb.StopBits = ONESTOPBIT;
    SetCommState(hSerialPort, &dcb);

    // Receive events on byte receive
    if (!SetCommMask(hSerialPort, EV_RXCHAR))
        return NULL;

    if (hSerialPort == INVALID_HANDLE_VALUE)
        return NULL;
    else
        return hSerialPort;
}

void OpenCAN_Close(HANDLE hSerialPort)
{
    CloseHandle(hSerialPort);
}

/**
 * Blocks until we send a CAN Message.
 */
uint8_t OpenCAN_WriteCAN(HANDLE hSerialPort, CANMsg_Standard_t *txMsg)
{
    uint8_t rawMsg[USB_DEC_PACKET_SIZE];
    uint8_t encodedMsg[USB_ENC_PACKET_SIZE];
    uint8_t bytesWritten;

    // Transform our message structure into a byte array
    vSerializeMessage(txMsg, &rawMsg[0]);
    // Encode data using COBS
    StuffData(rawMsg, USB_DEC_PACKET_SIZE, encodedMsg);
    // Put the EOF as the last byte
    encodedMsg[USB_ENC_PACKET_SIZE - 1] = 0x00;

    // Send the data off to be written when time allows it. ucWriteToFile blocks
    // until data was sent or until timeout.
    bytesWritten = ucWriteToFile(hSerialPort, (uint8_t *)encodedMsg, USB_ENC_PACKET_SIZE);
    if (!bytesWritten)
        return 1U;
    else
        return 0U;
}

/**
 * Blocks until we receive a correctly formatted RX Message.
 */
uint8_t OpenCAN_ReadCAN(HANDLE hSerialPort, CANMsg_Standard_t *rxMsg)
{
    uint8_t ucBufferIndex;
    uint8_t ucBytesRead;
    uint8_t ucBytesReadTotal;
    uint8_t pucCircularBuffer[USB_ENC_PACKET_SIZE];
    uint8_t pucEncodedData[USB_ENC_PACKET_SIZE];
    uint8_t pucDecodedData[USB_DEC_PACKET_SIZE];

    // Specifies wether the read was complete or not
    uint8_t msgReadComplete = FALSE;

    // We initialize these variables
    ucBufferIndex = 0U;
    ucBytesRead = 0U;
    ucBytesReadTotal = 0U;

    do
    {
        ucBytesRead = ucReadFromFile(hSerialPort, &pucCircularBuffer[ucBufferIndex], 1U);
        ucBytesReadTotal += ucBytesRead;
        if (ucBytesRead)
        {
            // The EOF byte is 0x00 - Read until it comes
            if (pucCircularBuffer[ucBufferIndex] == 0x00U)
            {
                vExtractDataCircularBuffer(pucCircularBuffer, pucEncodedData, ucBufferIndex, USB_ENC_PACKET_SIZE);
                UnStuffData(pucEncodedData, USB_ENC_PACKET_SIZE, pucDecodedData);
                if (pucDecodedData[0] == RX_CAN_RECV)
                {
                    vBuildCANFrameFromData(pucDecodedData, rxMsg);
                    msgReadComplete = TRUE;
                }
            }

            // Increment the ringbuffer index
            ucBufferIndex = (ucBufferIndex + 1) % USB_ENC_PACKET_SIZE;
        }

    } while (!msgReadComplete);

    // Return the number of bytes read in total
    return ucBytesReadTotal;
}

void vSerializeMessage(CANMsg_Standard_t *src, uint8_t *dest)
{
    // Command to execute on the OpenCAN
    dest[0] = TX_CAN_SEND;
    dest[1] = src->isExtended;
    dest[2] = (src->msgID & 0xFF000000) >> 24;
    dest[3] = (src->msgID & 0x00FF0000) >> 16;
    dest[4] = (src->msgID & 0x0000FF00) >> 8;
    dest[5] = (src->msgID & 0x000000FF);
    dest[6] = src->DLC;
    // Copy data buffer in preparation for send
    memcpy((void *)&dest[7], src->Data, 8);
}

// Returns the number of bytes written
static uint8_t ucWriteToFile(HANDLE hSerialPort, uint8_t *Buf, uint8_t Len)
{
    DWORD bytesWritten;
    DWORD dwRes;
    OVERLAPPED osWrite = {0};
    osWrite.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    if (osWrite.hEvent == NULL)
        // error creating osWrite event handle
        return 0U;

    if (!WriteFile(hSerialPort, (void *)Buf, (DWORD)Len, &bytesWritten, &osWrite))
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
                if (!GetOverlappedResult(hSerialPort, &osWrite, (LPDWORD)&bytesWritten, FALSE))
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

static uint8_t ucReadFromFile(HANDLE hSerialPort, uint8_t *pucDest, uint8_t ucBytesToRead)
{
    // TODO: Can we simplify this?
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
        if (!ReadFile(hSerialPort, pucDest, ucBytesToRead, &bytesRead, &osReader))
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

    // Stores result of the wait for event
    DWORD dwRes;

    if (fWaitingOnRead)
    {
        dwRes = WaitForSingleObject(osReader.hEvent, INFINITE);
        switch (dwRes)
        {
        // Read completed.
        case WAIT_OBJECT_0:
            if (!GetOverlappedResult(hSerialPort, &osReader, &bytesRead, FALSE))
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
        default:
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
    msg->isExtended = data[1];
    msg->msgID = (data[2] << 24) + (data[3] << 16) + (data[4] << 8) + data[5];
    msg->DLC = data[6];
    memcpy(msg->Data, &data[7], 8U);
}