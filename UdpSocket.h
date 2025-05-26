#ifndef UDP_SOCKET_H
#define UDP_SOCKET_H

#include <stdio.h>
#include <stdint.h>
#include "u32_fmt_t.h"

#ifdef PLATFORM_WINDOWS
    #include <winsock2.h>
    #include <WS2tcpip.h>
    #pragma comment(lib,"ws2_32.lib")
#elif defined(PLATFORM_LINUX)
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <fcntl.h>
	#include <cerrno>
#endif

class UdpSocket {
public:
    UdpSocket() {
        #ifdef PLATFORM_WINDOWS
            // Initialize Winsock
            if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
                printf("WSAStartup failed with error code : %d", WSAGetLastError());
                exit(EXIT_FAILURE);
            }
        #endif

        // Create socket
        #ifdef PLATFORM_WINDOWS
            if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == SOCKET_ERROR)
        #elif defined(PLATFORM_LINUX)
            if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        #endif
        {
            #ifdef PLATFORM_WINDOWS
                printf("socket() failed with error code : %d", WSAGetLastError());
            #elif defined(PLATFORM_LINUX)
                printf("socket() failed with error code : %d", errno);
            #endif
            exit(EXIT_FAILURE);
        }

        memset((char*)&si_other, 0, sizeof(si_other));
        si_other.sin_family = AF_INET;
    }

    ~UdpSocket() {
        #ifdef PLATFORM_WINDOWS
            closesocket(s);
            WSACleanup();
        #elif defined(PLATFORM_LINUX)
            close(s);
        #endif
    }

    bool setNonBlocking() {
        #ifdef PLATFORM_WINDOWS
            u_long iMode = 1;
            return ioctlsocket(s, FIONBIO, &iMode) == 0;
        #elif defined(PLATFORM_LINUX)
            int flags = fcntl(s, F_GETFL, 0);
            if (flags == -1) return false;
            return fcntl(s, F_SETFL, flags | O_NONBLOCK) != -1;
        #endif
    }

    bool bindTo(const char* ip, uint16_t port) {
        struct sockaddr_in server;
        memset((char*)&server, 0, sizeof(server));
        server.sin_family = AF_INET;
        
        if (ip == nullptr || strcmp(ip, "0.0.0.0") == 0) {
            #ifdef PLATFORM_WINDOWS
                server.sin_addr = in4addr_any;
            #elif defined(PLATFORM_LINUX)
                server.sin_addr.s_addr = INADDR_ANY;
            #endif
        } else {
            inet_pton(AF_INET, ip, &server.sin_addr);
        }
        
        server.sin_port = htons(port);
        
        return ::bind(s, (struct sockaddr*)&server, sizeof(server)) == 0;
    }

    bool setTarget(const char* ip, uint16_t port) {
        si_other.sin_port = htons(port);
        return inet_pton(AF_INET, ip, &si_other.sin_addr) == 1;
    }

    int sendTo(const void* data, size_t length) {
        return sendto(s, (const char*)data, length, 0, 
                     (struct sockaddr*)&si_other, sizeof(si_other));
    }

    int receiveFrom(void* buffer, size_t length, char* src_ip = nullptr, uint16_t* src_port = nullptr) {
        struct sockaddr_in src_addr;
        socklen_t addr_len = sizeof(src_addr);
        
        int received = recvfrom(s, (char*)buffer, length, 0, 
                              (struct sockaddr*)&src_addr, &addr_len);
        
        if (received > 0 && src_ip != nullptr) {
            inet_ntop(AF_INET, &src_addr.sin_addr, src_ip, INET_ADDRSTRLEN);
        }
        
        if (received > 0 && src_port != nullptr) {
            *src_port = ntohs(src_addr.sin_port);
        }
        
        return received;
    }

private:
    #ifdef PLATFORM_WINDOWS
        SOCKET s;
        WSADATA wsa;
    #elif defined(PLATFORM_LINUX)
        int s;
    #endif
    
    struct sockaddr_in si_other;
};

#endif 