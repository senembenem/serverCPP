#ifndef SERVER_H
#define SERVER_H

#include <iostream>
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <vector>
#pragma comment(lib, "ws2_32.lib")


class NetworkInfo {
    public:
    struct TCP {
        public:
        inline static bool Connected = false;
        inline static bool youAreServer = false;
        inline static bool Using = false;
    };

    struct UDP {
        public:
        inline static bool youAreServer = false;
        inline static bool Listening = false;
        inline static bool Using = false;
    };
};


class WindowsSockIO {
    public:
    BOOL wsaUNLocked = FALSE;
    WindowsSockIO();
    ~WindowsSockIO();
    class TCP_SOCKET {
        public:
        TCP_SOCKET(WindowsSockIO& parent, short sin_family, unsigned short sin_port, std::string sin_addr);
        ~TCP_SOCKET();
        void closeServer();
        void disconnect();
        SOCKET Connect();
        void BindAndListen();
        int sendAll(SOCKET sock, const char* buffer, int totalBytes);

        void reset_addr(); // WARNING!! PROGRAM CRASH RISK IN THIS FUNC
        std::vector<SOCKET> otherSockets = {};

        private:
        WindowsSockIO& parent;
        bool unlocked = false;
        sockaddr_in server_addr {};
        SOCKET your_socket;
    };

    class UDP_SOCKET {
        public:
            UDP_SOCKET(WindowsSockIO& parent, short sin_family, unsigned short sin_port, std::string sin_addr);
            ~UDP_SOCKET();
            void closeServer();
            void Bind();

            const int receiveFROM(SOCKET serverSOCK, char buffer[], int bufferSize, sockaddr& clientAddr);

            
            int sendData(SOCKET sock, const char* buffer, int bufferSize, struct sockaddr_in sock_addr);

            void reset_addr();

            sockaddr_in client_addr {};
        private:
            WindowsSockIO& parent;
            bool unlocked = false;
            sockaddr_in server_addr {};
            SOCKET your_socket;
    };

    private:
    WSADATA wsaData;
};


#endif