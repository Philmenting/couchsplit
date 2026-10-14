// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <atomic>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

namespace coop {
constexpr uint32_t Magic = 0x434F5031;
constexpr int Width = 256, Height = 192, Pixels = Width * Height;
// This ABI is Linux-only. Both processes are built with the same compiler/arch.
// A try-lock drops display frames rather than delaying emulation. Input is lock-free.
struct Shared {
    uint32_t magic = Magic;
    uint32_t version = 1;
    std::atomic<uint32_t> lock{0}, keys{0xFFF}, touch{0}, frame{0};
    std::atomic<uint32_t> peers{0}, role{0}, bridgeActive{0};
    uint32_t top[Pixels]{}, bottom[Pixels]{};
};
static_assert(std::atomic<uint32_t>::is_always_lock_free);
class Mapping {
    int fd = -1;
public:
    Shared* data = nullptr;
    Mapping() = default;
    Mapping(const Mapping&) = delete;
    Mapping& operator=(const Mapping&) = delete;
    ~Mapping() { if (data) munmap(data, sizeof(Shared)); if (fd >= 0) close(fd); }
    void open(const std::string& path, bool create) {
        if (data) throw std::runtime_error("IPC mapping already open");
        fd = ::open(path.c_str(), O_RDWR | O_CLOEXEC | O_NOFOLLOW | (create ? O_CREAT | O_EXCL : 0), 0600);
        if (fd < 0) throw std::runtime_error("Cannot open IPC: " + path);
        struct stat st{};
        if (fstat(fd, &st) != 0 || !S_ISREG(st.st_mode) || st.st_uid != getuid())
            throw std::runtime_error("Unsafe IPC file");
        if (create && ftruncate(fd, sizeof(Shared)) != 0) throw std::runtime_error("Cannot size IPC");
        if (!create && st.st_size != sizeof(Shared)) throw std::runtime_error("IPC size mismatch");
        void* p = mmap(nullptr, sizeof(Shared), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (p == MAP_FAILED) throw std::runtime_error("Cannot map IPC");
        data = static_cast<Shared*>(p);
        if (create) new (data) Shared();
        if (data->magic != Magic || data->version != 1) throw std::runtime_error("IPC version mismatch");
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
