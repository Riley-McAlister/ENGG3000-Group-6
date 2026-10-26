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

#define BOX_COUNT 3
#define SENSOR_COUNT 2

#include <winsock2.h>
#include <stdint.h>

typedef struct {
  uint8_t id;
  float distance[2];
} BoxPayload;
typedef struct {
  BoxPayload data[3];
} DataPayload;
typedef struct {
    float loc[2];
} PlayerLoc;

int initUDP(int port);
int receiveUDPBoxPayload(BoxPayload* data);
int receiveUDPDataPayload(DataPayload* data);
int receivePlayerLoc(PlayerLoc* data);
void closeUDP(void);

#endif