#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace aie::engine {

enum class PeArchitecture { X86, X86_64, Unsupported };
enum class PeFormat { PE32, PE32Plus };

struct PeSection {
    std::string name;
    std::uint32_t virtual_size{};
    std::uint32_t virtual_address{};
    std::uint32_t raw_size{};
    std::uint32_t raw_offset{};
    std::uint32_t characteristics{};
};

struct PeImageInfo {
    PeArchitecture architecture{PeArchitecture::Unsupported};
    PeFormat format{PeFormat::PE32};
    std::uint16_t machine{};
    std::uint16_t section_count{};
    std::uint32_t entry_point_rva{};
    std::uint64_t preferred_image_base{};
    std::vector<PeSection> sections;
};

struct PeParseResult {
    bool ok{false};
    PeImageInfo image{};
    std::string error;
};

// Parses and validates PE metadata only. It does not map, relocate, link, or execute code.
PeParseResult inspect_pe_image(const std::uint8_t* data, std::size_t size);
PeParseResult inspect_pe_image(const std::vector<std::uint8_t>& data);

} // namespace aie::engine
