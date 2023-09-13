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

#define SHM_SIZE 8192
#define SHM_NAME "cache_nic"
#define CHECKREADY(p)               ((*p >> 31) & 0b1)
#define CHECKVALID(p)               ((*p >> 30) & 0b1)
#define CHECKACK(p)                 ((*p >> 29) & 0b1)
#define GETSRC_ID(p)                (*(p + 1))
#define GETDST_ID(p)                (*(p + 2))
#define GETPACKET_ID(p)             (*(p + 3))
#define GETPACKET_NUM(p)            (*(p + 4))
#define GETTENSOR_ID(p)             (*(p + 5))
#define GETPACKET_SIZE(p)           (*(p + 6))
#define GETADDR(p, index)           (static_cast<uint64_t>(*(p + 7 + 12*index)))
#define GETREQ_TYPE(p, index)       (*(p + 8 + 12*index))
#define GETFLIT_WORD_NUM(p, index)  (*(p + 9 + 12*index))
#define GETDATA(p, index, pos)      (static_cast<int>(*(p + 10 + pos + 12*index)))

// Mutex to protect the global parameter
pthread_mutex_t* mutex;
pthread_mutex_t mutex_sendBack;
Packet* sendBackPacket;
#define SEND_BACK_PACKET_SIZE 1000

void runCache_noxim();
void executeRemainTraffic();
void* checkNoC(void* arg);
void* sentBackToNoC(void* arg);
void sendBackPacket_init();
void resetSendBackPacket(Packet* packet);
void transPacketToTraffic(Packet packet, int id);
bool checkDependencyTable(int tensor_id, int packet_num, int nic_id);
void assignPacket(Packet* p, Packet assign_packet);

//! Write data to shared memory
void setIPC_Ready(uint32_t *ptr);
void setIPC_Valid(uint32_t *ptr);
void setIPC_Ack(uint32_t *ptr);
void resetIPC_Ready(uint32_t *ptr);
void resetIPC_Valid(uint32_t *ptr);
void resetIPC_Ack(uint32_t *ptr);
void setIPC_Data(uint32_t *ptr, int data, int const_pos, int index);
void setIPC_Addr(uint32_t *ptr, uint64_t data, int index);

#endif