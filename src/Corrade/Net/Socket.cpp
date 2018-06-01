#include "Socket.h"

#include <Corrade/Utility/Debug.h>

#ifdef CORRADE_TARGET_WINDOWS

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>


namespace Corrade { namespace Net {

struct Socket::SocketData {
    SOCKET socket = INVALID_SOCKET;
};

Socket::Socket(const std::string& host, int port):
    _data{new SocketData}
{
    // Initialize Winsock
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        Utility::Error() << "Socket::Socket(): WSAStartup failed with error code:" << iResult;
        return;
    }

    struct addrinfo hints;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    /* Resolve the server address and port */
    struct addrinfo *result = nullptr;
    iResult = getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &result);
    if(iResult != 0) {
        Utility::Error() << "Socket::Socket(): getaddrinfo failed with error:" << iResult;
        close();
        return;
    }

    /* Attempt to connect to an address until one succeeds */
    for(struct addrinfo *ptr = result; ptr; ptr = ptr->ai_next) {
        _data->socket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);

        if (_data->socket == INVALID_SOCKET) {
            Utility::Error() << "Socket::Socket(): socket failed with error" << WSAGetLastError();
            close();
            return;
        }

        iResult = connect(_data->socket, ptr->ai_addr, int(ptr->ai_addrlen));
        if (iResult == SOCKET_ERROR) {
            closesocket(_data->socket);
            _data->socket = INVALID_SOCKET;
            continue;
        }
        break;
    }

    freeaddrinfo(result);

    if(_data->socket == INVALID_SOCKET) {
        Utility::Error() << "Socket::Socket(): unable to connect to server";
        close();
        return;
    }

    _connected = true;
}

Socket::~Socket() {
    const int iResult = shutdown(_data->socket, SD_SEND);
    if(iResult == SOCKET_ERROR) {
        Utility::Error() << "Socket::Socket(): shutdown failed with error:" << WSAGetLastError();
    }

    close();
}

void Socket::send(Containers::ArrayView<char> data) {
    CORRADE_ASSERT(_connected, "Socket::send(): socket not connected", );

    int iResult = ::send(_data->socket, data, data.size(), 0);
    if(iResult == SOCKET_ERROR) {
        Utility::Error() << "Socket::Socket(): send failed with error:" <<  WSAGetLastError();
        close();
        return;
    }
}

Containers::ArrayView<char> Socket::receive(Containers::Array<char>& dest, int timeout) {
    CORRADE_ASSERT(_connected, "Socket::receive(): socket not connected", {});

    /* Wait timeout milliseconds to receive data */
    fd_set sockets{1};
    sockets.fd_array[0] = _data->socket;
    const timeval t{0, timeout*1000};
    int ret = select(0, &sockets, nullptr, nullptr, (timeout == -1) ? nullptr : &t);
    if(ret == 0) {
        /* Timeout */
        return nullptr;
    }

    ret = recv(_data->socket, dest.data(), dest.size(), 0);
    if(ret < 0) {
        Utility::Error() << "Socket::receive(): recv failed with error:" << WSAGetLastError();
        return {};
    }

    return dest.prefix(ret);
}

void Socket::close() {
    if(_data->socket != INVALID_SOCKET) closesocket(_data->socket);
    WSACleanup();
    _connected = false;
}

}}

#endif
