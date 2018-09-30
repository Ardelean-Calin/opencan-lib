#ifndef OPEN_CAN_
#define OPEN_CAN_

#include <Windows.h>

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;

typedef struct
{
    uint16_t msgID;
    uint8_t DLC;
    uint8_t Data[8];
} CANMsg_Standard_t;

#define TX_MSG_START_BYTE 0xDE
#define TX_MSG_END_BYTE 0xAD
#define TX_MSG_RAW_SIZE 13
#define RX_MSG_START_BYTE 0xBE
#define RX_MSG_END_BYTE 0xEF
#define RX_MSG_RAW_SIZE 12

enum
{
    CAN_WRITE_MSG = 0,
    CAN_CHANGE_BAUDRATE
};

/*
 * Function prototypes 
 */
void *OpenCAN_Open(char *portName);
void OpenCAN_Close(HANDLE handle);
uint8_t OpenCAN_Write(HANDLE hComm, uint8_t *Buf, uint8_t Len);
void OpenCAN_WriteCAN(HANDLE hComm, CANMsg_Standard_t *txMsg);
uint8_t OpenCAN_ReadCAN(HANDLE hComm, uint8_t *readBuffer);
#endif