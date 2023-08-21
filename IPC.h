#ifndef _IPC_H_
#define _IPC_H_
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>  // for O_* constants
#include <string.h>
#include <sys/mman.h>  // for POSIX shared memory API
#include <sys/stat.h>  // for mode constants
#include <unistd.h>    // ftruncate
#include <stdint.h>
#include <stdio.h>
#include <iomanip>
#include <unistd.h>
#include "datastruct.h"

#define READ_DATA 0xacef8795
#define WRITE_DATA 0xbbbbbbbb
#define SHM_NAME "cache_nic"
#define SHM_SIZE 512
#define REQ_PACKET_SIZE 64
#define DATA_PACKET_SIZE 256
#define CHECKREADY(p) (*(p + 15) >> 31)
#define CHECKVALID(p) ((*(p + 15) >> 30) & 0b1)
#define CHECKACK(p) ((*(p + 15) >> 29) & 0b1)
#define CHECKFINISH(p) ((*(p + 15) >> 28) & 0b1)
#define GETSRC_ID(p) (*p)
#define GETDST_ID(p) (*(p + 1))
#define GETPACKET_ID(p) (*(p + 2))
#define GETREQ(p, pos) (*(p + 3 + pos))
#define GETDATA(p, pos) (*(p + 5 + pos))
#define GETREAD(p) (*(p + 13))
#define GETREQUEST_SIZE(p) (*(p + 14))
#define GETTEST(p,pos) (*(p + pos))


// Mutex to protect the global parameter
pthread_mutex_t* mutex;
pthread_mutex_t mutex_sendBack;
packet_data* sendBackPacket;
#define SEND_BACK_PACKET_SIZE 1000

void runCache_noxim();
void executeRemainTraffic();
void* checkNoC(void* arg);
void* sentBackToNoC(void* arg);
char* concatenateStrings(const char* str1, const char* str2);
void sendBackPacket_init();
void resetSendBackPacket(packet_data* packet);
void transPacketToTraffic(packet_data packet, int id);
void setIPC_Data(uint32_t *ptr, uint32_t data, int const_pos, int varied_pos);
void setIPC_Valid(uint32_t *ptr);
void resetIPC_Valid(uint32_t *ptr);
void setIPC_Ready(uint32_t *ptr);
void resetIPC_Ready(uint32_t *ptr);
void setIPC_Ack(uint32_t *ptr);
void resetIPC_Ack(uint32_t *ptr);

#endif