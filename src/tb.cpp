#include "tb.hpp"
#include <string>

// MUST BE INCLUDED BEFORE tbprobe.h
#include "bb_attacks.hpp"

// MUST BE INCLUDED AFTER bb_attacks.h
#include <tbprobe.h>

namespace Clockwork::tb {

InitStatus init(std::string_view path) {
    const std::string path_str{path};

    if (!tb_init(path_str.c_str())) {
        return InitStatus::Failed;
    }

    if (TB_LARGEST == 0) {
        return InitStatus::NoneFound;
    }

    return InitStatus::Success;
}

void free() {
    tb_free();
}

u32 dtz_count() {
    return static_cast<u32>(TB_NUM_DTZ);
}

u32 wdl_count() {
    return static_cast<u32>(TB_NUM_WDL);
}

u32 max_pieces() {
    return static_cast<u32>(TB_LARGEST);
}

WDL probe_wdl(const Position& pos) {
    const auto ep_square = pos.en_passant();
    const u32  ep_idx    = ep_square.is_valid() ? ep_square.raw : 0;

    const auto piece_type_bb = [&](PieceType piece_type) {
        const auto bb =
          pos.bitboard_for(Color::White, piece_type) | pos.bitboard_for(Color::Black, piece_type);
        return bb.value();
    };

    const auto wdl = tb_probe_wdl(pos.board().get_color_bitboard(Color::White).value(),
                                  pos.board().get_color_bitboard(Color::Black).value(),
                                  piece_type_bb(PieceType::King), piece_type_bb(PieceType::Queen),
                                  piece_type_bb(PieceType::Rook), piece_type_bb(PieceType::Bishop),
                                  piece_type_bb(PieceType::Knight), piece_type_bb(PieceType::Pawn),
                                  ep_idx, pos.active_color() == Color::White);

    switch (wdl) {
    case TB_RESULT_FAILED:
        return WDL::None;
    case TB_WIN:
        return WDL::Win;
    case TB_LOSS:
        return WDL::Loss;
    default:
        // Cursed wins and blessed losses are both functionally draws
        return WDL::Draw;
    }
}

}
