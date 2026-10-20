#ifndef NETWORK_H
#define NETWORK_H
#ifndef NOGDI
#define NOGDI
#endif
#ifndef NOUSER
#define NOUSER
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <winsock2.h>

typedef struct{
    float distance[6];
} SensorData;

typedef struct{
    float x;
    float y;
    int valid;
} PlayerPosition;

int initUDP(int port);
int receiveUDPSingle(SensorData* data);
PlayerPosition calculatePlayerPosition(SensorData *data);
void closeUDP(void);

#endif