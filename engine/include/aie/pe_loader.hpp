#pragma once

#include "aie/guest_memory.hpp"
#include "aie/pe_image.hpp"
#include "aie/x64_interpreter.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace aie::engine {

struct ImportedSymbol {
    bool by_ordinal{false};
    std::uint16_t ordinal{};
    std::string name;
};

using ImportResolver =
    std::function<std::optional<std::uint64_t>(const std::string&, const ImportedSymbol&)>;

struct LoadedPeImage {
    PeImageInfo info;
    std::uint64_t load_base{};
    std::vector<std::uint8_t> memory;

    std::uint64_t entry_point() const {
        return load_base + info.entry_point_rva;
    }
};

struct PeLoadResult {
    bool ok{false};
    LoadedPeImage image;
    std::string error;
};

struct GuestPeLoadResult {
    bool ok{false};
    PeImageInfo info;
    std::uint64_t load_base{};
    std::uint64_t entry_point{};
    std::size_t mapped_size{};
    std::string error;
};

// Maps headers/sections into a bounded byte-addressed image, applies supported base
// relocations, and resolves imports through a caller-supplied module registry.
// This is a data loader only: mapped bytes are never executed by this API.
PeLoadResult map_pe_image(const std::vector<std::uint8_t>& file,
                          std::uint64_t requested_base = 0,
                          const ImportResolver& resolver = {},
                          std::size_t maximum_image_size = 512U * 1024U * 1024U);

// Maps the relocated PE image into GuestMemory and applies per-page section
// permissions derived from IMAGE_SCN_MEM_READ/WRITE/EXECUTE. Writable+executable
// pages are rejected. Imports still require an explicit caller-supplied resolver.
GuestPeLoadResult map_pe_image_to_guest_memory(
    const std::vector<std::uint8_t>& file, GuestMemory& guest_memory,
    std::uint64_t requested_base = 0,
    const ImportResolver& resolver = {},
    std::size_t maximum_image_size = 512U * 1024U * 1024U);

// Runs only the currently implemented x86-64 instruction subset from a PE
// executable section. Code is first fetched through GuestMemory execute checks;
// data MOV operations use the same bounded GuestMemory instance. No Windows APIs
// are present and unsupported opcodes stop execution.
X64RunResult run_pe_entry_subset(const GuestPeLoadResult& image,
                                 GuestMemory& guest_memory,
                                 X64CpuState initial_state = {},
                                 std::size_t maximum_steps = 100000);

} // namespace aie::engine
