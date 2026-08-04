#pragma once

#include "position.hpp"
#include "util/types.hpp"
#include <string_view>

namespace Clockwork::tb {

enum class InitStatus {
    Failed,
    NoneFound,
    Success,
};

InitStatus init(std::string_view path);
void       free();

[[nodiscard]] u32 dtz_count();
[[nodiscard]] u32 wdl_count();

[[nodiscard]] u32 max_pieces();

enum class WDL {
    None,
    Win,
    Draw,
    Loss,
};

[[nodiscard]] WDL probe_wdl(const Position& pos);

}
