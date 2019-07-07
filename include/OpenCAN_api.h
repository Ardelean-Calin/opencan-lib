#ifndef OPEN_CAN_
#define OPEN_CAN_

#include <windows.h>
#include <stdint.h>
#include <stddef.h>

/* Defines */
// Fixed packet size for USB messages
#define USB_DEC_PACKET_SIZE (16U)
#define USB_ENC_PACKET_SIZE (USB_DEC_PACKET_SIZE + 2)

/* Typedefs and enums */
typedef struct
{
    uint32_t msgID;
    uint8_t isExtended;
    uint8_t DLC;
    uint8_t Data[8];
} CANMsg_Standard_t;

enum
{
    TX_CAN_SEND = 0,
    RX_CAN_RECV,
    TX_ACK_REQUEST,
    RX_ACK_SEND,
    TX_CAN_SET_BAUDRATE
};

// Supported baudrates by device
enum
{
    CAN_BAUDRATE_1000K = 0,
    CAN_BAUDRATE_800K,
    CAN_BAUDRATE_666K,
    CAN_BAUDRATE_500K,
    CAN_BAUDRATE_250K,
    CAN_BAUDRATE_125K,
    CAN_BAUDRATE_100K,
    CAN_BAUDRATE_83K,
    CAN_BAUDRATE_50K,
    CAN_BAUDRATE_20K,
    CAN_BAUDRATE_10K,
    CAN_BAUDRATE_NOT_SUPPORTED
};

#ifdef __cplusplus
extern "C"
{
#endif
    /*
 * Function prototypes 
 */

    uint8_t OpenCAN_Open(HANDLE *hSerialPort, uint8_t *pucCOMNumber);
    void OpenCAN_Close(HANDLE hSerialPort);
    uint8_t OpenCAN_WriteCAN(HANDLE hSerialPort, CANMsg_Standard_t *txMsg);
    uint8_t OpenCAN_SetBaudrate(HANDLE hSerialPort, uint8_t ucBaudSelector);
    uint8_t OpenCAN_ReadCAN(HANDLE hSerialPort, CANMsg_Standard_t *rxMsg);
#ifdef __cplusplus
}
#endif
#endif