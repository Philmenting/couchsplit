// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <atomic>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace coop {
constexpr uint32_t Magic = 0x434F5031;
constexpr int Width = 256, Height = 192, Pixels = Width * Height;
// Both processes must use the same architecture/compiler and IPC ABI.
// A try-lock drops display frames rather than delaying emulation. Input is lock-free.
struct Shared {
    uint32_t magic = Magic;
    uint32_t version = 2;
    std::atomic<uint32_t> lock{0}, keys{0xFFF}, touch{0}, frame{0};
    std::atomic<uint32_t> peers{0}, role{0}, bridgeActive{0};
    std::atomic<uint32_t> shutdown{0};
    uint32_t top[Pixels]{}, bottom[Pixels]{};
};
static_assert(std::atomic<uint32_t>::is_always_lock_free);
class Mapping {
#ifdef _WIN32
    HANDLE file = INVALID_HANDLE_VALUE, mapping = nullptr;
#else
    int fd = -1;
#endif
public:
    Shared* data = nullptr;
    Mapping() = default;
    Mapping(const Mapping&) = delete;
    Mapping& operator=(const Mapping&) = delete;
    ~Mapping() {
#ifdef _WIN32
        if(data) UnmapViewOfFile(data);
        if(mapping) CloseHandle(mapping);
        if(file!=INVALID_HANDLE_VALUE) CloseHandle(file);
#else
        if(data) munmap(data,sizeof(Shared));
        if(fd>=0) close(fd);
#endif
    }
    void open(const std::string& path, bool create) {
        if (data) throw std::runtime_error("IPC mapping already open");
#ifdef _WIN32
        int length=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,path.c_str(),-1,nullptr,0);
        if(!length) throw std::runtime_error("Invalid UTF-8 IPC path");
        std::wstring wide(length,L'\0');MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,path.c_str(),-1,wide.data(),length);
        file=CreateFileW(wide.c_str(),GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,
                         nullptr,create?CREATE_NEW:OPEN_EXISTING,FILE_FLAG_OPEN_REPARSE_POINT,nullptr);
        if(file==INVALID_HANDLE_VALUE) throw std::runtime_error("Cannot open IPC: "+path);
        BY_HANDLE_FILE_INFORMATION info{};
        if(!GetFileInformationByHandle(file,&info) || (info.dwFileAttributes&(FILE_ATTRIBUTE_REPARSE_POINT|FILE_ATTRIBUTE_DIRECTORY)))
            throw std::runtime_error("Unsafe IPC file");
        LARGE_INTEGER size{};size.QuadPart=sizeof(Shared);
        if(create && (!SetFilePointerEx(file,size,nullptr,FILE_BEGIN)||!SetEndOfFile(file))) throw std::runtime_error("Cannot size IPC");
        if(!GetFileSizeEx(file,&size)||size.QuadPart!=sizeof(Shared)) throw std::runtime_error("IPC size mismatch");
        mapping=CreateFileMappingW(file,nullptr,PAGE_READWRITE,0,0,nullptr);
        if(!mapping) throw std::runtime_error("Cannot create IPC mapping");
        void* p=MapViewOfFile(mapping,FILE_MAP_ALL_ACCESS,0,0,sizeof(Shared));
        if(!p) throw std::runtime_error("Cannot map IPC");
#else
        fd = ::open(path.c_str(), O_RDWR | O_CLOEXEC | O_NOFOLLOW | (create ? O_CREAT | O_EXCL : 0), 0600);
        if (fd < 0) throw std::runtime_error("Cannot open IPC: " + path);
        struct stat st{};
        if (fstat(fd, &st) != 0 || !S_ISREG(st.st_mode) || st.st_uid != getuid())
            throw std::runtime_error("Unsafe IPC file");
        if (create && ftruncate(fd, sizeof(Shared)) != 0) throw std::runtime_error("Cannot size IPC");
        if (!create && st.st_size != sizeof(Shared)) throw std::runtime_error("IPC size mismatch");
        void* p = mmap(nullptr, sizeof(Shared), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (p == MAP_FAILED) throw std::runtime_error("Cannot map IPC");
#endif
        data = static_cast<Shared*>(p);
        if (create) new (data) Shared();
        if (data->magic != Magic || data->version != 2) throw std::runtime_error("IPC version mismatch");
    }
    bool acquire() { uint32_t expected = 0; return data->lock.compare_exchange_strong(expected, 1, std::memory_order_acquire); }
    void release() { data->lock.store(0, std::memory_order_release); }
    void publish(const void* top, const void* bottom) {
        if (!data || !top || !bottom || !acquire()) return;
        memcpy(data->top, top, sizeof(data->top));
        memcpy(data->bottom, bottom, sizeof(data->bottom));
        data->frame.fetch_add(1, std::memory_order_relaxed);
        release();
    }
};
// packed touch: bit 31 pressed, x in bits 0..7, y in bits 8..15.
inline uint32_t touch(int x, int y, bool down) { return down ? 0x80000000u | uint32_t(x & 255) | uint32_t((y & 255) << 8) : 0; }
}
