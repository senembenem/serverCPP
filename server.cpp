#include "server.h"


WindowsSockIO::WindowsSockIO() {
    if(WSAStartup(MAKEWORD(2, 2), &wsaData) == 0)
        this->wsaUNLocked = TRUE;
    else
        this->wsaUNLocked = FALSE;
}


WindowsSockIO::~WindowsSockIO() {
    WSACleanup();
}


void WindowsSockIO::TCP_SOCKET::reset_addr() {
    if(NetworkInfo::TCP::Using) {
        sockaddr_in &add = this->server_addr;
        add.sin_addr.s_addr = 0;
        add.sin_family = 0;
        add.sin_port = 0;

        this->otherSockets.clear();
        this->unlocked = false;
    }
}


WindowsSockIO::TCP_SOCKET::TCP_SOCKET(WindowsSockIO& parent, short sin_family, unsigned short sin_port, std::string sin_addr) : parent(parent) {
    if(parent.wsaUNLocked == false || NetworkInfo::TCP::Using)
        return;

    if(sin_family != AF_INET && sin_family != AF_INET6)
        return;

    SOCKET& sock = this->your_socket;
    sock = socket(sin_family, SOCK_STREAM, IPPROTO_TCP);
    BOOL optVAL = TRUE;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&optVAL), sizeof(optVAL));
    if(sock == INVALID_SOCKET)
        return;

    server_addr.sin_family = sin_family;

    if(inet_pton(sin_family, sin_addr.c_str(), &server_addr.sin_addr.s_addr) != 1) {
        closesocket(sock);
        reset_addr();
        return;
    }

    this->server_addr.sin_port = htons(sin_port);
    NetworkInfo::TCP::Using = true;
    this->unlocked = true;
}

WindowsSockIO::TCP_SOCKET::~TCP_SOCKET() {
    if(NetworkInfo::TCP::youAreServer) closeServer();
    else if (NetworkInfo::TCP::youAreServer == false) disconnect();
    NetworkInfo::TCP::Connected = false;
    NetworkInfo::TCP::youAreServer = false;
    NetworkInfo::TCP::Using = false;
    this->unlocked = false;
}


void WindowsSockIO::TCP_SOCKET::BindAndListen() {
    if(!this->unlocked || NetworkInfo::TCP::Connected || !NetworkInfo::TCP::Using) return;
    NetworkInfo::TCP::youAreServer = true;

    if(bind(this->your_socket, (struct sockaddr*)&this->server_addr, sizeof(this->server_addr)) != SOCKET_ERROR) {
        if(listen(this->your_socket, SOMAXCONN) != SOCKET_ERROR) {
            SOCKET cli_sock = accept(this->your_socket, nullptr, nullptr);
            if(cli_sock != INVALID_SOCKET) {
                this->otherSockets.push_back(cli_sock);
                NetworkInfo::TCP::Connected = true;
                return;
            }
        }
    }

    NetworkInfo::TCP::Connected = false;
}


SOCKET WindowsSockIO::TCP_SOCKET::Connect() {
    if(!this->unlocked || NetworkInfo::TCP::Connected || !NetworkInfo::TCP::Using) return INVALID_SOCKET;

    if(connect(this->your_socket, (struct sockaddr*)&this->server_addr, sizeof(this->server_addr)) != SOCKET_ERROR) {
        NetworkInfo::TCP::youAreServer = false;
        NetworkInfo::TCP::Connected = true;
        return this->your_socket;
    }

    NetworkInfo::TCP::Connected = false;
    return INVALID_SOCKET;
}


int WindowsSockIO::TCP_SOCKET::sendAll(SOCKET sock, const char* buffer, int totalBytes) {
    if(NetworkInfo::TCP::Connected && this->unlocked && NetworkInfo::TCP::Using) {
        if(sock == INVALID_SOCKET)
            return SOCKET_ERROR;

        int bytesSent = 0;

        while(bytesSent < totalBytes) {
            int result = send(sock, buffer + bytesSent, totalBytes - bytesSent, 0);

            if(result == SOCKET_ERROR)
                return SOCKET_ERROR;

            bytesSent = bytesSent + result;
        }

        return bytesSent;
    }

    return SOCKET_ERROR;
}

void WindowsSockIO::TCP_SOCKET::disconnect() {
    if(this->your_socket != INVALID_SOCKET && NetworkInfo::TCP::Connected && NetworkInfo::TCP::Using) {
        NetworkInfo::TCP::Connected = false;
        NetworkInfo::TCP::Using = false;
        shutdown(this->your_socket, SD_BOTH);
        closesocket(this->your_socket);
        this->your_socket = INVALID_SOCKET;
    }
}


void WindowsSockIO::TCP_SOCKET::closeServer() {
    if(this->your_socket == INVALID_SOCKET || NetworkInfo::TCP::Using == false)
        return;

    if(NetworkInfo::TCP::youAreServer == false) return;

    if(this->otherSockets.size() > 0)
        for(SOCKET sock : this->otherSockets) {
            shutdown(sock, SD_BOTH);
            closesocket(sock);
        }

    this->otherSockets.clear();
    this->disconnect();
}










WindowsSockIO::UDP_SOCKET::UDP_SOCKET(WindowsSockIO& parent, short sin_family, unsigned short sin_port, std::string sin_addr) : parent(parent) {
    if(parent.wsaUNLocked == false || NetworkInfo::UDP::Using)
        return;

    if(sin_family != AF_INET && sin_family != AF_INET6) {
        return;
    }

    your_socket = INVALID_SOCKET;
    your_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    BOOL optVal = TRUE;
    setsockopt(your_socket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&optVal), sizeof(optVal));

    if(your_socket == INVALID_SOCKET)
        return;

    server_addr.sin_family = sin_family;

    if(inet_pton(sin_family, sin_addr.c_str(), &server_addr.sin_addr.s_addr) != 1) {
        this->reset_addr();
        return;
    }

    server_addr.sin_port = htons(sin_port);
    this->unlocked = true;
    NetworkInfo::UDP::Using = true;
}


void WindowsSockIO::UDP_SOCKET::reset_addr() {
    if(NetworkInfo::UDP::Using) {
        this->server_addr.sin_addr.s_addr = 0;
        this->server_addr.sin_family = 0;
        this->server_addr.sin_port = 0;
    }
}


WindowsSockIO::UDP_SOCKET::~UDP_SOCKET() {
    this->unlocked = false;
    NetworkInfo::UDP::youAreServer = false;
    NetworkInfo::UDP::Listening = false;
    NetworkInfo::UDP::Using = false;

    closesocket(your_socket);

    reset_addr();
}



int WindowsSockIO::UDP_SOCKET::sendData(SOCKET sock, const char* buffer, int bufferSize, struct sockaddr_in sock_addr) {
    if(bufferSize <= 0 || this->unlocked == false) return SOCKET_ERROR;

    return sendto(sock, buffer, bufferSize, 0, reinterpret_cast<struct sockaddr*>(&sock_addr), sizeof(sock_addr));
}


void WindowsSockIO::UDP_SOCKET::Bind() {
    if(!this->unlocked || NetworkInfo::UDP::Listening == true || !NetworkInfo::UDP::Using) return;
    NetworkInfo::UDP::youAreServer = true;

    if(bind(your_socket, (struct sockaddr*)&this->server_addr, sizeof(this->server_addr)) != SOCKET_ERROR) {
        NetworkInfo::UDP::Listening = true;
    }
}


const int WindowsSockIO::UDP_SOCKET::receiveFROM(SOCKET serverSOCK, char buffer[], int bufferSize, sockaddr& clientAddr) {
    if(serverSOCK == INVALID_SOCKET || bufferSize <= 0 || this->unlocked == false || !NetworkInfo::UDP::Listening || !NetworkInfo::UDP::Using) {
        return SOCKET_ERROR;
    }

    socklen_t clientAddr_len = sizeof(clientAddr);

    const int result = recvfrom(serverSOCK, buffer, bufferSize, 0, (struct sockaddr*)&clientAddr, &clientAddr_len);
    return result;
}


void WindowsSockIO::UDP_SOCKET::closeServer() {
    if(this->unlocked && NetworkInfo::UDP::youAreServer == true && NetworkInfo::UDP::Listening == true && NetworkInfo::UDP::Using) {
        closesocket(your_socket);
        this->unlocked = false;
        NetworkInfo::UDP::youAreServer = false;
        NetworkInfo::UDP::Listening = false;
    }
}