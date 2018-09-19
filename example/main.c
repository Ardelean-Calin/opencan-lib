#include "OpenCAN_api.h"
#include <stdio.h>
#include <sys/time.h>
#include <event.h>

void* hcom;
uint8_t Buf[] = {0, 1, 2, 3, 4, 5, 6, 7};
CANMsg_Standard_t txMsg;

void sendMsg(int fd, short event, void *arg){
    
    OpenCAN_WriteCAN(hcom, &txMsg);
}

int main(){
    hcom = OpenCAN_Open("COM3");
    if (hcom == NULL)
        printf("Error in opening serial port");
    else
        printf("opening serial port successful");

    txMsg.DLC = 8;
    txMsg.msgID = 0x201;
    memcpy((void*) txMsg.Data, Buf, 8);

    struct event ev;
    struct timeval tv;

    tv.tv_sec = 0;
    tv.tv_usec = 10000;

    event_init();
    evtimer_set(&ev, sendMsg, NULL);
    evtimer_add(&ev, &tv);
    event_dispatch();
    OpenCAN_Close(hcom);
    return 0;
}