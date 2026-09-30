// Bounded cartridge SRAM handoff and UI-thread file operations.
// Copyright 2026 Christian Kauten
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#ifndef RACKNES_SRAM_TRANSFER_HPP
#define RACKNES_SRAM_TRANSFER_HPP

#include <array>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include "nes/emulator.hpp"
#ifdef _WIN32
#include <windows.h>
#include <fcntl.h>
#include <io.h>
#include <sys/stat.h>
#else
#include <unistd.h>
#endif

/// One UI producer/consumer and one engine consumer/producer. No worker.
/// Preparing and Done belong to the UI; Pending belongs to the engine.
/// Release/acquire state transitions publish bytes and acknowledge reuse.
/// Shared ownership keeps dialogs safe after module destruction; close() never
/// touches the slot. Rack serializes engine callbacks with module destruction.
class SRAMTransfer {
 public:
    static constexpr std::size_t SIZE = 8192;
    enum class State { Idle, Preparing, Pending, Done };
    enum class Result { Success, Stale, Unsupported };

 private:
    std::atomic<State> state{State::Idle};
    /// Low bit is support, remaining bits are the cartridge generation.
    std::atomic<uint64_t> context{0};
    std::atomic<bool> alive{true};
    uint64_t requested_context = 0;
    bool importing = false;
    Result result = Result::Stale;
    std::array<NES::NES_Byte, SIZE> bytes = {};

 public:
    /// UI-only destination retained until the engine acknowledges an export.
    std::string destination;

    /// Called only by the engine or exclusive Rack lifecycle callback.
    void publish(const NES::Emulator& emulator) {
        context.store((emulator.get_cartridge_generation() << 1) |
            (emulator.persistent_size() == SIZE), std::memory_order_release);
    }
    void close() { alive.store(false, std::memory_order_release); }
    bool supported() const {
        return alive.load(std::memory_order_acquire) &&
            (context.load(std::memory_order_acquire) & 1);
    }
    bool busy() const { return state.load(std::memory_order_acquire) != State::Idle; }
    bool current() const {
        return alive.load(std::memory_order_acquire) &&
            context.load(std::memory_order_acquire) == requested_context;
    }
    /// Reserve before opening a dialog; a second action fails without queuing.
    bool begin(bool import) {
        State expected = State::Idle;
        if (!state.compare_exchange_strong(expected, State::Preparing,
                std::memory_order_acquire)) return false;
        requested_context = context.load(std::memory_order_acquire);
        importing = import;
        if (!(requested_context & 1) || !current()) {
            release();
            return false;
        }
        return true;
    }
    /// UI access only while Preparing or Done; never while Pending.
    std::array<NES::NES_Byte, SIZE>& buffer() { return bytes; }
    void submit() { state.store(State::Pending, std::memory_order_release); }
    /// Acknowledged result or abandoned preparation; UI only.
    void release() { state.store(State::Idle, std::memory_order_release); }
    bool ready() const { return state.load(std::memory_order_acquire) == State::Done; }
    Result outcome() const { return result; }
    bool is_import() const { return importing; }

    /// Bounded engine operation after controls/expanders, including while hung.
    void process(NES::Emulator& emulator) {
        publish(emulator);
        if (state.load(std::memory_order_acquire) != State::Pending) return;
        result = Result::Stale;
        if (current()) {
            const bool success = importing ?
                emulator.import_sram(bytes.data(), bytes.size()) :
                emulator.export_sram(bytes.data(), bytes.size());
            result = success ? Result::Success : Result::Unsupported;
        }
        state.store(State::Done, std::memory_order_release);
    }
};

// The supported Rack platforms must not implement engine atomics with locks.
static_assert(ATOMIC_LLONG_LOCK_FREE == 2 && ATOMIC_LONG_LOCK_FREE == 2 &&
    ATOMIC_INT_LOCK_FREE == 2 &&
    ATOMIC_BOOL_LOCK_FREE == 2, "SRAM handoff requires lock-free atomics");

namespace SRAMFiles {
#ifdef _WIN32
/// Windows file APIs take UTF-16 paths, while Rack dialogs return UTF-8.
inline std::wstring wide_path(const std::string& path) {
    const int size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
        path.c_str(), -1, nullptr, 0);
    if (!size) return {};
    std::vector<wchar_t> buffer(size);
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
        path.c_str(), -1, buffer.data(), size);
    return buffer.data();
}
#endif

/// Read exactly one raw domain; no partial import, padding, or private header.
inline bool read(const std::string& path, SRAMTransfer& transfer) {
#ifdef _WIN32
    FILE* file = _wfopen(wide_path(path).c_str(), L"rb");
#else
    FILE* file = std::fopen(path.c_str(), "rb");
#endif
    if (!file) return false;
    auto& bytes = transfer.buffer();
    const bool full = std::fread(bytes.data(), 1, bytes.size(), file) == bytes.size();
    const bool exact = std::fgetc(file) == EOF && !std::ferror(file);
    const bool closed = std::fclose(file) == 0;
    return full && exact && closed;
}

/// Write a unique sibling, check write/flush/close, then atomically replace.
/// All paths, temporary storage, and I/O belong to the UI. A stale generation
/// cancels before replacement. No remove-then-rename fallback is permitted.
inline bool write(const std::string& path, SRAMTransfer& transfer) {
    if (!transfer.current()) return false;
#ifdef _WIN32
    const auto target = wide_path(path);
    std::wstring temporary = target + L".tmp-XXXXXX";
    std::vector<wchar_t> name(temporary.begin(), temporary.end());
    name.push_back(0);
    if (_wmktemp_s(name.data(), name.size()) != 0) return false;
    const int fd = _wopen(name.data(), _O_CREAT | _O_EXCL | _O_WRONLY | _O_BINARY,
        _S_IREAD | _S_IWRITE);
    if (fd < 0) return false;
    FILE* file = _fdopen(fd, "wb");
#else
    const std::string temporary = path + ".tmp-XXXXXX";
    std::vector<char> name(temporary.begin(), temporary.end());
    name.push_back(0);
    const int fd = mkstemp(name.data());
    if (fd < 0) return false;
    FILE* file = fdopen(fd, "wb");
#endif
    bool success = false;
    if (file) {
        const auto& bytes = transfer.buffer();
        success = std::fwrite(bytes.data(), 1, bytes.size(), file) == bytes.size();
        if (std::fflush(file) != 0) success = false;
#ifdef _WIN32
        if (_commit(fd) != 0) success = false;
#else
        if (fsync(fd) != 0) success = false;
#endif
        if (std::fclose(file) != 0) success = false;
    } else {
#ifdef _WIN32
        _close(fd);
#else
        close(fd);
#endif
    }
    success = success && transfer.current();
#ifdef _WIN32
    if (success) success = MoveFileExW(name.data(), target.c_str(),
        MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
    if (!success) _wremove(name.data());
#else
    if (success) success = std::rename(name.data(), path.c_str()) == 0;
    if (!success) std::remove(name.data());
#endif
    return success;
}
}  // namespace SRAMFiles

#endif  // RACKNES_SRAM_TRANSFER_HPP
