#undef UNICODE

#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x501

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>

#include <fstream>

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "AdvApi32.lib")

#define DEFAULT_BUFLEN 512

int main( int argc, char *argv[] )
{
    std::ifstream InFile;

    WSAData wsaData;
    SOCKET ConnectSocket = INVALID_SOCKET;
    struct addrinfo *result = NULL,
                    *ptr = NULL,
                    hints;
    char sendbuf[DEFAULT_BUFLEN];
    char recvbuf[DEFAULT_BUFLEN];
    int iResult;
    int recvbuflen = DEFAULT_BUFLEN;

    // Validate parameters.
    if ( argc != 4 ) {
        printf("usage: <exe> <input filename> <server ip> <server port>\n", argv[0]);
        return 1;
    }

    // Validate file.
    InFile.open( argv[1], std::ios::in | std::ios::binary );
    if ( !InFile.is_open() ) {
        printf("open failed.");
        return 1;
    }

    // Initialize Winsock.
    iResult = WSAStartup( MAKEWORD(2, 2), &wsaData );
    if ( iResult != 0 ){
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the server address and port.
    iResult = getaddrinfo( argv[2], argv[3], &hints, &result );
    if ( iResult != 0 ) {
        printf("getaddrinfo failed with error: %ld\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    // Attempt to connect to an address until one succeeds.
    for ( ptr=result; ptr != NULL; ptr=ptr->ai_next ) {

        // Create a SOCKET for connecting to the server.
        ConnectSocket = socket( ptr->ai_family, ptr->ai_socktype,
            ptr->ai_protocol );
        if ( ConnectSocket == INVALID_SOCKET ) {
            printf("socket failed with error: %ld\n", WSAGetLastError());
            WSACleanup();
            return 1;
        }

        // Connect to server.
        iResult = connect( ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen );
        if ( iResult == SOCKET_ERROR ) {
            closesocket(ConnectSocket);
            ConnectSocket = INVALID_SOCKET;
            continue;
        }
        break;
    }

    freeaddrinfo(result);

    if ( ConnectSocket == INVALID_SOCKET ) {
        printf("Unable to connect to server!\n");
        WSACleanup();
        return 1;
    }

    // Send fragmented file.
    while( InFile.read(sendbuf, DEFAULT_BUFLEN) ) {
        iResult = send( ConnectSocket, sendbuf, InFile.gcount(), 0 );
        if ( iResult == SOCKET_ERROR ) {
            printf("send failed with error: %d\n", WSAGetLastError());
            closesocket(ConnectSocket);
            WSACleanup();
            return 1;
        }
        printf("Bytes sent: %ld\n", iResult);
    }
    iResult = send( ConnectSocket, sendbuf, InFile.gcount(), 0 );
    if ( iResult == SOCKET_ERROR ) {
        printf("send failed with error: %d\n", WSAGetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }
    printf("Bytes sent: %ld\n", iResult);


    // Shutdown the connection since no more data will be sent.
    iResult = shutdown( ConnectSocket, SD_SEND );
    if ( iResult == SOCKET_ERROR ) {
        printf("shutdown failed with error: %d\n", WSAGetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }

    // Receive until the peer closes the connection.
    do {
        iResult = recv( ConnectSocket, recvbuf, recvbuflen, 0 );
        if ( iResult > 0 ) {
            printf("Bytes received: %d\n", iResult);
        }
        else if ( iResult == 0 ) {
            printf("Connection closed.\n");
        }
        else {
            printf("recv failed with error: %d\n", WSAGetLastError());
        }
    } while ( iResult > 0 );

    // Cleanup.
    closesocket(ConnectSocket);
    InFile.close();
    WSACleanup();
    return 0;
}