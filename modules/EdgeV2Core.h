#pragma once
#include <stdint.h>
namespace edgev2 {
enum class Result { Applied, Duplicate, Stale, Unsafe, StorageFailure, Invalid };
inline const char* message(Result r) {
    switch(r) {
        case Result::Applied: return "applied"; case Result::Duplicate: return "duplicate_state_matches";
        case Result::Stale: return "stale_sequence"; case Result::Unsafe: return "unsafe_or_state_mismatch";
        case Result::StorageFailure: return "sequence_storage_failed"; default: return "invalid_command";
    }
}
struct IO {
    virtual uint64_t high(unsigned ch) = 0;
    virtual bool persist(unsigned ch, uint64_t seq) = 0;
    virtual bool state(unsigned ch) = 0;
    virtual bool write(unsigned ch, bool on) = 0;
    virtual void safeOff() = 0;
    virtual ~IO() = default;
};
inline Result execute(IO& io, unsigned count, unsigned ch, uint64_t seq, bool on, bool alloff) {
    if (!seq || !count || count > 4 || (!alloff && (ch < 1 || ch > count)) || (alloff && (on || ch))) return Result::Invalid;
    unsigned first = alloff ? 1 : ch, last = alloff ? count : ch;
    bool applied = false, duplicate = false, mismatch = false;
    for (unsigned i = first; i <= last; ++i) {
        uint64_t old = io.high(i);
        if (seq < old) { if (!alloff) return Result::Stale; continue; }
        if (seq == old) {
            if (io.state(i) != on) { if (!alloff) return Result::Unsafe; mismatch = true; continue; }
            duplicate = true; continue;
        }
        if (!io.persist(i, seq)) { io.safeOff(); return Result::StorageFailure; }
        if (!io.write(i, on)) { io.safeOff(); return Result::Unsafe; }
        applied = true;
    }
    if (mismatch) return Result::Unsafe;
    if (applied) return Result::Applied;
    return duplicate ? Result::Duplicate : Result::Stale;
}
}
