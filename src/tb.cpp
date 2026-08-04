#include "tb.hpp"
#include "bb_attacks.hpp"
#include <algorithm>
#include <array>
#include <string>
#include <tbprobe.h>


#include <iostream>

namespace Clockwork::tb {

namespace {

[[nodiscard]] u64 piece_type_bb(const Position& pos, PieceType piece_type) {
    const auto bb =
      pos.bitboard_for(Color::White, piece_type) | pos.bitboard_for(Color::Black, piece_type);
    return bb.value();
}

}

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

bool probe_root(const Position& pos, std::span<Search::RootMove> root_moves) {
    const auto move_from_tb = [&](PyrrhicMove tb_move) {
        static constexpr std::array PROMO_PIECE_FLAGS = {
          MoveFlags::Normal,      MoveFlags::PromoQueen,  MoveFlags::PromoRook,
          MoveFlags::PromoBishop, MoveFlags::PromoKnight,
        };

        const Square from{static_cast<u8>(PYRRHIC_MOVE_FROM(tb_move))};
        const Square to{static_cast<u8>(PYRRHIC_MOVE_TO(tb_move))};

        auto flags = PROMO_PIECE_FLAGS[PYRRHIC_MOVE_FLAGS(tb_move) & PYRRHIC_MASK_PROMO_FLAGS];

        if (PYRRHIC_MOVE_IS_ENPASS(tb_move)) {
            // an ep move cannot be a promotion
            flags = MoveFlags::EnPassant;
        }

        if (pos.piece_at(to) != PieceType::None) {
            flags |= MoveFlags::CaptureBit;
        }

        return Move{from, to, flags};
    };

    const auto wdl_from_tb = [](i32 tb_rank) {
        static constexpr i32 MAX_DTZ = 262144;

        static constexpr i32 WIN_BOUND  = MAX_DTZ - 100;
        static constexpr i32 DRAW_BOUND = -MAX_DTZ + 101;

        if (tb_rank >= WIN_BOUND) {
            return Search::WDL::Win;
        } else if (tb_rank >= DRAW_BOUND) {
            return Search::WDL::Draw;
        } else {
            return Search::WDL::Loss;
        }
    };

    TbRootMoves tb_root_moves{};
    bool        dtz_succeeded = true;

    const auto ep_square = pos.en_passant();
    const u32  ep_idx    = ep_square.is_valid() ? ep_square.raw : 0;

    auto wdl = tb_probe_root_dtz(
      pos.board().get_color_bitboard(Color::White).value(),
      pos.board().get_color_bitboard(Color::Black).value(), piece_type_bb(pos, PieceType::King),
      piece_type_bb(pos, PieceType::Queen), piece_type_bb(pos, PieceType::Rook),
      piece_type_bb(pos, PieceType::Bishop), piece_type_bb(pos, PieceType::Knight),
      piece_type_bb(pos, PieceType::Pawn), pos.get_50mr_counter(), ep_idx,
      pos.active_color() == Color::White, false /* TODO */, &tb_root_moves);

    if (!wdl) {
        dtz_succeeded = false;
        wdl           = tb_probe_root_wdl(
          pos.board().get_color_bitboard(Color::White).value(),
          pos.board().get_color_bitboard(Color::Black).value(), piece_type_bb(pos, PieceType::King),
          piece_type_bb(pos, PieceType::Queen), piece_type_bb(pos, PieceType::Rook),
          piece_type_bb(pos, PieceType::Bishop), piece_type_bb(pos, PieceType::Knight),
          piece_type_bb(pos, PieceType::Pawn), pos.get_50mr_counter(), ep_idx,
          pos.active_color() == Color::White, true, &tb_root_moves);
    }

    if (!wdl || tb_root_moves.size == 0) {
        return dtz_succeeded;
    }

    const auto get_root_move = [&](Move move) -> Search::RootMove* {
        for (auto& root_move : root_moves) {
            if (root_move.pv.first_move() == move) {
                return &root_move;
            }
        }
        return nullptr;
    };

    for (usize i = 0; i < tb_root_moves.size; i++) {
        const auto [tb_move, tb_rank] = tb_root_moves.moves[i];

        const auto move = move_from_tb(tb_move);
        const auto wdl  = wdl_from_tb(tb_rank);

        auto* root_move = get_root_move(move);
        if (!root_move) {
            std::cout << "failed to find root move for " << move << std::endl;
            continue;
        }

        root_move->set_tb_status(wdl, tb_rank);
    }

    std::ranges::stable_sort(root_moves, [](const Search::RootMove& a, const Search::RootMove& b) {
        return a.tb_rank > b.tb_rank;
    });

    return dtz_succeeded;
}

Search::WDL probe_wdl(const Position& pos) {
    const auto ep_square = pos.en_passant();
    const u32  ep_idx    = ep_square.is_valid() ? ep_square.raw : 0;

    const auto wdl = tb_probe_wdl(
      pos.board().get_color_bitboard(Color::White).value(),
      pos.board().get_color_bitboard(Color::Black).value(), piece_type_bb(pos, PieceType::King),
      piece_type_bb(pos, PieceType::Queen), piece_type_bb(pos, PieceType::Rook),
      piece_type_bb(pos, PieceType::Bishop), piece_type_bb(pos, PieceType::Knight),
      piece_type_bb(pos, PieceType::Pawn), ep_idx, pos.active_color() == Color::White);

    switch (wdl) {
    case TB_RESULT_FAILED:
        return Search::WDL::None;
    case TB_WIN:
        return Search::WDL::Win;
    case TB_LOSS:
        return Search::WDL::Loss;
    default:
        // Cursed wins and blessed losses are both functionally draws
        return Search::WDL::Draw;
    }
}

}
