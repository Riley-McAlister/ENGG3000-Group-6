#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib")

#define false 0
#define true 1

#define PORT 55555
// Subnet broadcast address calculated for 192.168.1.1 with mask 255.255.255.240
#define BROADCAST_IP "10.126.159.255"

// Match your exact data structures from the receiver side
typedef struct {
    float x;
    float y;
} SensorData;

typedef struct {
    SensorData data[6];
} DataPayload;

int main() {
    // Initialize Winsock
    WSADATA WSAData;
    if (WSAStartup(MAKEWORD(2, 2), &WSAData) != 0) {
        printf("Winsock init failed. ERR: %d\n", WSAGetLastError());
        return 1;
    }

    // Create UDP socket
    SOCKET sendSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sendSocket == INVALID_SOCKET) {
        printf("Socket creation failed. ERR: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    // CRITICAL: Enable broadcasting option on the socket, otherwise OS blocks it
    int broadcastOpt = TRUE;
    if (setsockopt(sendSocket, SOL_SOCKET, SO_BROADCAST, (char*)&broadcastOpt, sizeof(broadcastOpt)) == SOCKET_ERROR) {
        printf("setsockopt(SO_BROADCAST) failed. ERR: %d\n", WSAGetLastError());
        closesocket(sendSocket);
        WSACleanup();
        return 1;
    }

    // Configure destination address
    struct sockaddr_in destAddr;
    destAddr.sin_family = AF_INET;
    destAddr.sin_port = htons(PORT);
    destAddr.sin_addr.s_addr = inet_addr(BROADCAST_IP);

    printf("Starting mock UDP broadcast sender on port %d...\n", PORT);
    printf("Targeting broadcast address: %s\n", BROADCAST_IP);

    // Continuous loop to send mock packages every 1 second
    int counter = 0;
    while (1) {
        SensorData data= {500.0f, 250.0f};

        // Send the packet over the network/loopback
        int sentBytes = sendto(
            sendSocket, 
            (char*)&data, 
            sizeof(SensorData), 
            0, 
            (struct sockaddr*)&destAddr, 
            sizeof(destAddr)
        );

        if (sentBytes == SOCKET_ERROR) {
            printf("Sendto failed. ERR: %d\n", WSAGetLastError());
        } else {
            printf("Sent mock DataPayload (%d bytes) - Iteration %d\n", sentBytes, counter);
        }

        counter++;
        Sleep(1000); // Wait 1 second before sending the next one
    }

    // Cleanup (though loop is infinite here)
    closesocket(sendSocket);
    WSACleanup();
    return 0;
}

// gcc sender.c -o sender.exe -lws2_32