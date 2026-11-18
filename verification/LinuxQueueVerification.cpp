#include "prx/libc/include/Api.hpp"
#include "prx/libc/include/General.hpp"
#include "prx/libc/include/RunReport.hpp"
#include "prx/libc/include/GuestArena.hpp"
#include "prx/libc/include/specifics/linux/GuestFaultCopy.hpp"
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <string_view>

int QueueGuardMain(int argc, char** argv);

static std::size_t probes = 0;
static std::size_t writes = 0;

extern "C" void LibcRunReportMissingFunction_nid_no_patch(const char*, const char*, int) noexcept {}

extern "C" bool __real_GuestWritable_nid_no_patch(void* destination, std::size_t bytes);

extern "C" bool __wrap_GuestWritable_nid_no_patch(void* destination, std::size_t bytes) {
    ++probes;
    return __real_GuestWritable_nid_no_patch(destination, bytes);
}

static void CountWrite(std::uintptr_t, std::size_t) {
    ++writes;
}

struct Links {
    void* forward;
    void* back;
};

int main(int argc, char** argv) {
    if (argc < 2 || std::string_view(argv[1]) != "volume") return QueueGuardMain(argc, argv);
    const bool optimized = argc == 3 && std::string_view(argv[2]) == "optimized";
    const bool fast = GuestFaultCopy::Ready();
    Links previous{}, next{}, element{};
    previous.forward = &next;
    next.back = &previous;
    GuestArena::GuestArenaSetHostWriteHook_nid_no_patch(CountWrite);
    constexpr unsigned count = 1000;
    std::size_t insertProbes = 0, insertWrites = 0, removeProbes = 0, removeWrites = 0;
    for (unsigned i = 0; i < count; ++i) {
        probes = 0;
        writes = 0;
        errno = 73;
        insque_nid_postfix(&element, &previous);
        insertProbes += probes;
        insertWrites += writes;
        if (errno != 73 || previous.forward != &element || next.back != &element) return 2;
        probes = 0;
        writes = 0;
        remque_nid_postfix(&element);
        removeProbes += probes;
        removeWrites += writes;
        if (errno != 73 || previous.forward != &next || next.back != &previous) return 3;
    }
    GuestArena::GuestArenaSetHostWriteHook_nid_no_patch(nullptr);
    std::printf("mode=%s fault_copy=%d operations=%u insque_probes=%zu insque_host_writes=%zu remque_probes=%zu remque_host_writes=%zu\n",
        optimized ? "optimized" : "baseline", fast, count, insertProbes, insertWrites, removeProbes, removeWrites);
    if (optimized && fast) return insertProbes == 0 && insertWrites == count * 3 && removeProbes == 0 && removeWrites == count * 2 ? 0 : 4;
    return insertProbes == count * 3 && insertWrites == count * 6 && removeProbes == count * 2 && removeWrites == count * 4 ? 0 : 5;
}
