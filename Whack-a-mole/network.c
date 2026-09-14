#include "network.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#pragma comment(lib, "ws2_32.lib")

#define false 0
#define true 1

float sensor1Pos = 0.25f;
float sensor2Pos = 0.25f;
float sensor3Pos = 0.75f;
float sensor4Pos = 0.75f;
float sensor5Pos = 1.25f;
float sensor6Pos = 1.25f;

static SOCKET UDPSocket = INVALID_SOCKET;

int initUDP(int port)
{
    WSADATA WSAData;

    if (WSAStartup(MAKEWORD(2, 2), &WSAData) != 0)
    {
        printf("Winsock init failed. ERR: %d.\n", WSAGetLastError());
        return false;
    }

    UDPSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (UDPSocket == INVALID_SOCKET)
    {
        printf("Socket creation failed. ERR: %d.\n", WSAGetLastError());
        WSACleanup();
        return false;
    }

    u_long mode = 1;
    if (ioctlsocket(UDPSocket, FIONBIO, &mode) != 0)
    {
        printf("ioctlsocket failed. ERR: %d.\n", WSAGetLastError());
        closesocket(UDPSocket);
        WSACleanup();
        return false;
    }

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(UDPSocket, (const struct sockaddr *)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
    {
        printf("Bind failed. ERR: %d.\n", WSAGetLastError());
        closesocket(UDPSocket);
        WSACleanup();
        return false;
    }
    return true;
}

int receiveUDPSingle(SensorData *data)
{
    if (UDPSocket == INVALID_SOCKET)
    {
        return false;
    }
    struct sockaddr_in fromAddr;
    int fromLen = sizeof(fromAddr);

    int read = recvfrom(UDPSocket, (char *)data, sizeof(SensorData), 0, (struct sockaddr *)&fromAddr, &fromLen);

    if (read == sizeof(SensorData))
    {
        printf("Read: distances:\n");

        for (int i = 0; i < 6; i++)
        {
            printf("Sensor %d: %.2f cm\n", i, data->distance[i]);
        }
        return true;
    }
    printf("Byte read: %d, ERR: %d\n", read, WSAGetLastError());
    return false;
}

int receiveUDPPayload(SensorData *payload)
{
    if (UDPSocket == INVALID_SOCKET)
    {
        return false;
    }
    struct sockaddr_in fromAddr;
    int fromLen = sizeof(fromAddr);

    int read = recvfrom(UDPSocket, (char *)payload, sizeof(SensorData), 0, (struct sockaddr *)&fromAddr, &fromLen);

    if (read == sizeof(SensorData))
    {
        printf("Read: %d\n", read);
        return true;
    }
    printf("Byte read: %d, ERR: %d\n", read, WSAGetLastError());
    return false;
}

PlayerPosition calculatePlayerPosition(SensorData *data)
{
    PlayerPosition result = {0};

    // Sensor Position in metres, from the left of the 1.5m area
    float sensorX[6] = {sensor1Pos, sensor2Pos, sensor3Pos, sensor4Pos, sensor5Pos,
                        sensor6Pos};
    // Sensors positioned on the front wall, y pos = 0
    float sensorY[6] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    // convert distance in cm to m
    float distance[6];
    // check number of valid sensors (Sensors receiving player distance)
    int validSensors = 0;
    for (int i = 0; i < 6; i++)
    {
        distance[i] = data->distance[i] / 100.0f;

        // ignore invalid readings
        if (data->distance[i] > 0.0f && data->distance[i] < 500.0f)
        {
            validSensors++;
        }
    }

    // 3 sensors required to estimate 2D position
    if (validSensors < 3)
    {
        result.valid = 0;
        return result;
    }
    // intial starting position
    float bestX = 0.75f;
    float bestY = 1.30f;
    float lowestError = 1000000.0f;
    // search the playing area in 1cm increments
    for (float x = 0.0f; x <= 1.5f; x += 0.01f){
        for(float y = 0.6f; y <=2.0f; y +=0.01f){
        float totalError = 0.0f;
        int measurementsUsed = 0;
        
        //iterate through sensors
        for(int i = 0; i < 6; i++){

            //ignore invalid readings
            if(data ->distance[i] <= 0.0f ||
            data->distance[i] >=500.0f){
                continue;
            }
            //calculate expected distance from sensor to initial position.
            float dx = x - sensorX[i];
            float dy = y - sensorY[i];

            float expectedDistance = sqrt(dx * dx + dy * dy);
            // calulate error using difference between measured and expected distance.
            float error = expectedDistance - distance[i];
            //square errors to combat positives and negatives cancelling each other. 
            totalError += error * error;

            measurementsUsed++;
        }
        //calculate average error
        if(measurementsUsed > 0){
            totalError /= measurementsUsed;
        }
        //Store position with lowest error
        if(totalError < lowestError){
            lowestError = totalError;

            bestX = x;
            bestY = y;
        }
    }
}

    //return calculated position
    result.x = bestX;
    result.y = bestY;
    result.valid = 1;

    return result;
}

void closeUDP(void)
{
    if (UDPSocket != INVALID_SOCKET)
    {
        closesocket(UDPSocket);
    }
    WSACleanup();
}