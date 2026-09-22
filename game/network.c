#include "network.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <string.h>

#pragma comment(lib, "ws2_32.lib")

#define false 0 
#define true 1

static SOCKET UDPSocket= INVALID_SOCKET;

int initUDP(int port) {
    WSADATA WSAData;

    if(WSAStartup(MAKEWORD(2, 2), &WSAData) != 0) {
        printf("Winsock init failed. <ERR: %d>\n", WSAGetLastError());
        return false;
    }

    UDPSocket= socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(UDPSocket == INVALID_SOCKET) {
        printf("Socket creation failed. <ERR: %d>\n", WSAGetLastError());
        WSACleanup();
        return false;
    }

    u_long mode= 1;
    if(ioctlsocket(UDPSocket, FIONBIO, &mode) != 0) {
        printf("ioctlsocket failed. <ERR: %d>\n", WSAGetLastError());
        closeUDP();
        return false;
    }

    struct sockaddr_in serverAddr;
    serverAddr.sin_family= AF_INET;
    serverAddr.sin_port= htons(port);
    serverAddr.sin_addr.s_addr= INADDR_ANY;

    if(bind(UDPSocket, (const struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        printf("Bind failed. <ERR: %d>\n", WSAGetLastError());
        closeUDP();
        return false;
    }
    return true;
}

int receiveUDPBoxPayload(BoxPayload *data) {
    if(UDPSocket == INVALID_SOCKET) { return false; }
    struct sockaddr_in fromAddr;
    int fromLen= sizeof(fromAddr);

    int read= recvfrom(UDPSocket, (char*)data, sizeof(BoxPayload), 0, (struct sockaddr*)&fromAddr, &fromLen);

    if(read == sizeof(BoxPayload)) {
        printf("Byte size received: %d\n", read);
        return true;
        }
    // printf("Byte size received: %d, <ERR: %d>\n", read, WSAGetLastError());
    return false;
}

int receiveUDPDataPayload(DataPayload *data) {
    if(UDPSocket == INVALID_SOCKET) { return false; }
    struct sockaddr_in fromAddr;
    int fromLen= sizeof(fromAddr);

    int read= recvfrom(UDPSocket, (char*)data, sizeof(DataPayload), 0, (struct sockaddr*)&fromAddr, &fromLen);

    if(read == sizeof(DataPayload)) {
        printf("Byte size received: %d\n", read);
        return true;
        }
    // printf("Byte size received: %d, <ERR: %d>\n", read, WSAGetLastError());
    return false;
}

int receivePlayerLoc(PlayerLoc* data) {
    if(UDPSocket == INVALID_SOCKET) { return false; }
    struct sockaddr_in fromAddr;
    int fromLen= sizeof(fromAddr);

    int read= recvfrom(UDPSocket, (char*)data, sizeof(PlayerLoc), 0, (struct sockaddr*)&fromAddr, &fromLen);

    if(read == sizeof(PlayerLoc)) {
        printf("Byte size received: %d\n", read);
        return true;
        }
    // printf("Byte size received: %d <ERR: %d>\n", read, WSAGetLastError());
    return false;
}

void closeUDP(void) {
    if(UDPSocket != INVALID_SOCKET) {
        closesocket(UDPSocket);
    }
    WSACleanup();
}