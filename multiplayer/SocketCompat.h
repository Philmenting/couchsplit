// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
// Compatibility for Project PM's Winsock transport. The wire format is unchanged.
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
inline int bridgeSelect(SOCKET s, fd_set* wr, fd_set* ex, timeval* tv) {
    return select(0, nullptr, wr, ex, tv);
}
inline int bridgeSend(SOCKET s, const char* p, int n, int flags) {
    return send(s, p, n, flags);
}
#else
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <unistd.h>
#include <cerrno>
#include <cstdint>
#include <chrono>
#include <cstring>
using SOCKET = int;
using BOOL = int;
using DWORD = uint32_t;
struct WSADATA {};
constexpr int INVALID_SOCKET = -1, SOCKET_ERROR = -1, TRUE = 1;
constexpr int WSAEWOULDBLOCK = EAGAIN;
inline int MAKEWORD(int, int) { return 0; }
inline int WSAStartup(int, WSADATA*) { return 0; }
inline int WSAGetLastError() { return errno == EINPROGRESS || errno == EWOULDBLOCK ? EAGAIN : errno; }
inline int closesocket(SOCKET s) { return close(s); }
inline int ioctlsocket(SOCKET s, unsigned long req, unsigned long* p) { return ioctl(s, req, p); }
inline DWORD GetTickCount() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}
inline BOOL GetComputerNameA(char* p, DWORD* n) {
    if (!n || *n < 2 || gethostname(p, *n) != 0) return 0;
    p[*n - 1] = 0; *n = strlen(p); return 1;
}
inline int bridgeSelect(SOCKET s, fd_set* wr, fd_set* ex, timeval* tv) {
    return select(s + 1, nullptr, wr, ex, tv);
}
inline int bridgeSend(SOCKET s, const char* p, int n, int flags) {
    return send(s, p, n, flags | MSG_NOSIGNAL);
}
#endif
inline bool bridgeConnectOK(SOCKET s) {
    int error = 0;
#ifdef _WIN32
    int n = sizeof(error);
#else
    socklen_t n = sizeof(error);
#endif
    return getsockopt(s, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&error), &n) == 0 && !error;
}
