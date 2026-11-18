#pragma once

#include "aie/pe_image.hpp"

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

// Maps headers/sections into a bounded byte-addressed image, applies supported base
// relocations, and resolves imports through a caller-supplied module registry.
// This is a data loader only: mapped bytes are never executed by this API.
PeLoadResult map_pe_image(const std::vector<std::uint8_t>& file,
                          std::uint64_t requested_base = 0,
                          const ImportResolver& resolver = {},
                          std::size_t maximum_image_size = 512U * 1024U * 1024U);

} // namespace aie::engine
