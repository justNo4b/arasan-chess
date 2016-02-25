// Copyright 2016 by Jon Dart. All Rights Reserved.
#include "syzygy.h"
#include "constant.h"
#include "debug.h"
#include "bitboard.h"

// custom popcount implementation - overrides the one in
// the Syzygy code.
unsigned popcount(uint64_t x) 
{
   return (unsigned)Bitboard(x).bitCount();
}

// custom lsb implementation - overrides the one in
// the Syzygy code.
unsigned lsb(uint64_t x) 
{
   return (unsigned)Bitboard(x).firstOne();
}

static const int CURSED_SCORE = 5;

static const int valueMap[5] = {-Constants::TABLEBASE_WIN, -CURSED_SCORE, 0, CURSED_SCORE, Constants::TABLEBASE_WIN};

static PieceType getPromotion(unsigned res) 
{
      switch (TB_GET_PROMOTES(res)) {
      case TB_PROMOTES_QUEEN:
         return Queen;
      case TB_PROMOTES_ROOK:
         return Rook;
      case TB_PROMOTES_BISHOP:
         return Bishop;
      case TB_PROMOTES_KNIGHT:
         return Knight;
      default:
         return Empty;
      }
}

int SyzygyTb::initTB(const string &path) 
{
   bool ok = syzygy_tb_init(path.c_str());
   if (!ok)
      return 0;
   else
      return TB_LARGEST;
}

int SyzygyTb::probe_root(const Board &b, int &score, set<Move> &rootMoves) 
{
   score = 0;
   unsigned results[TB_MAX_MOVES];
   Bitboard king_bits;
   king_bits.set(b.kingSquare(White));
   king_bits.set(b.kingSquare(Black));
   unsigned result = tb_probe_root((uint64_t)(b.occupied[White]),
                                   (uint64_t)(b.occupied[Black]),
                                   (uint64_t)king_bits,
                                   (uint64_t)(b.queen_bits[Black] | b.queen_bits[White]),
                                   (uint64_t)(b.rook_bits[Black] | b.rook_bits[White]),
                                   (uint64_t)(b.bishop_bits[Black] | b.bishop_bits[White]),
                                   (uint64_t)(b.knight_bits[Black] | b.knight_bits[White]),
                                   (uint64_t)(b.pawn_bits[Black] | b.pawn_bits[White]),
                                   b.state.moveCount,
                                   (int)b.castleStatus(White)<3 ||
                                   (int)b.castleStatus(Black)<3,
                                   b.enPassantSq()==InvalidSquare ? 0 : b.enPassantSq(),
                                   b.sideToMove() == White,
                                   results);
   
   if (result == TB_RESULT_FAILED) {
      return 0;
   }
   
   unsigned wdl = TB_GET_WDL(result);
   ASSERT(wdl<5);
   score = valueMap[wdl];
   const unsigned moveWdl = TB_GET_WDL(result);
   unsigned res;
   for (int i = 0; (res = results[i]) != TB_RESULT_FAILED; i++) {
      if (moveWdl >= wdl) {
         // move is ok, i.e. preserves WDL value
         unsigned ep = TB_GET_EP(res);
         // Note: castling not possible
         rootMoves.insert(CreateMove(TB_GET_FROM(res),
                                     TB_GET_TO(res),
                                     TypeOfPiece(b[TB_GET_FROM(res)]),
                                     getPromotion(res),
                                     ep ? Pawn : TypeOfPiece(b[TB_GET_TO(res)]),
                                     ep ? EnPassant : Normal));
      }
   }
   return 1;
}
