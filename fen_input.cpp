#include "global_header.h"
#include "support_files.h"
#include "global_constants.h"
#include "fen_input.h"

Chess fen_input(std::string piece_placement_data, std::string active_color, std::string castling_availability, std::string en_passant_target_square, std::string halfmove_clock, std::string fullmove_number) {

    std::string ppd_sub[8], parse_string, temp_string, castle_codes[16] {"KQkq", "KQk", "KQq", "KQ", "Kkq", "Kk", "Kq", "K", "Qkq", "Qk", "Qq", "Q", "kq", "k", "q", "-"};
    std::string en_passant_codes[17] {"a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3", "a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6", "-"}; 
    size_t found;
    int number_of_sub {0}, gap {0}, rank_count {0}, castle_NOK {1}, en_passant_NOK {1}, halfmove {0};
    char piece_data, castling_data;
    std::array <uint64_t, 9> board_setup {};
    std::vector <uint64_t> en_passant_squares {a3, b3, c3, d3, e3, f3, g3, h3, a6, b6, c6, d6, e6, f6, g6, h6};
    uint64_t traveling_one {1UL << 63};
    std::array<uint16_t, 64> mailbox_start {};
    Chess status;
    status.bitboard.fill(0);
    status.mailbox = mailbox_start;
    status.pit = false;
    status.tpt.depth = 0;
    status.tpt.move = 0;
    status.tpt.value = 0;
    status.tpt.zobrist_key_64 = 0UL;
     

    // check for legitimate characters 
    found = piece_placement_data.find_first_not_of("prbnqkPRNBQK/12345678");
    if (found!=std::string::npos) {
        std::cout << "info string A non standard English algebraic, or FEN, notation found " << piece_placement_data[found] << " \n";
        return Chess{};
    }
    
    found = active_color.find_first_not_of("wb");
    if (found!=std::string::npos) {
        std::cout << "info string A non standard FEN notation found for Active Color " << active_color[found] << " \n";
        return Chess{};
    }
    
    found = castling_availability.find_first_not_of("kqKQ-");
    if (found!=std::string::npos) {
        std::cout << "info string A non standard FEN notation found for Castling Availability " << castling_availability[found] << " \n";
        return Chess{};
    }

    found = en_passant_target_square.find_first_not_of("abcdefgh36-");
    if (found!=std::string::npos) {
        std::cout << "info string A non standard FEN notation found for En Passant Target Square " << en_passant_target_square[found] << " \n";
        return Chess{};
    }

    found = halfmove_clock.find_first_not_of("0123456789");
    if (found!=std::string::npos) {
        std::cout << "info string A non standard FEN notation found for Halfmove Clock " << halfmove_clock[found] << " \n";
        return Chess{};
    }

    found = fullmove_number.find_first_not_of("0123456789");
    if (found!=std::string::npos) {
        std::cout << "info string A non standard FEN notation found for Fullmove Number " << fullmove_number[found] << " \n";
        return Chess{};
    }
    
    // Piece Placement decoding and correct format
    for (size_t i = 0; i < piece_placement_data.size(); ++i) {

        temp_string = piece_placement_data.at(i); 

        if (temp_string == "/" and i == 0) {
            std::cout << "info string A non standard English algebraic, or FEN, notation found " << " \n";
            return Chess{};
        }

        if (temp_string == "/") {
            ++ number_of_sub;
            if (number_of_sub > 7 or ppd_sub[number_of_sub - 1].size() == 0 ) {
                std::cout << "info string A non standard English algebraic, or FEN, notation found " << " \n";
                return Chess{};
            }
            continue;
        }

        ppd_sub[number_of_sub].append(temp_string);
    }

    if (number_of_sub != 7) {
        std::cout << "info string A non standard English algebraic, or FEN, notation found " << " \n";
        return Chess{};
    }

    for (int i = 0; i < 8; ++i) {
        gap = 0;
        rank_count = 0;
        for (int j = ppd_sub[i].size() - 1; j >= 0; --j) {
            piece_data = ppd_sub[i].at(j);

            switch (piece_data) {
                case 'p': 
                    board_setup[1] |= traveling_one;
                    board_setup[2] |= traveling_one;
                    status.mailbox[__builtin_ctzl(traveling_one)] = 2;
                    ++ rank_count;
                    break;
                case 'n':
                    board_setup[1] |= traveling_one;
                    board_setup[3] |= traveling_one;
                    status.mailbox[__builtin_ctzl(traveling_one)] = 3;
                    ++ rank_count;
                    break;
                case 'b':
                    board_setup[1] |= traveling_one;
                    board_setup[4] |= traveling_one;
                    status.mailbox[__builtin_ctzl(traveling_one)] = 4;
                    ++ rank_count;
                    break;
                case 'r':
                    board_setup[1] |= traveling_one;
                    board_setup[5] |= traveling_one;
                    status.mailbox[__builtin_ctzl(traveling_one)] = 5;
                    ++ rank_count;
                    break;
                case 'q':
                    board_setup[1] |= traveling_one;
                    board_setup[6] |= traveling_one;
                    status.mailbox[__builtin_ctzl(traveling_one)] = 6;
                    ++ rank_count;
                    break;
                case 'k':
                    board_setup[1] |= traveling_one;
                    board_setup[7] |= traveling_one;
                    status.mailbox[__builtin_ctzl(traveling_one)] = 7;
                    ++ rank_count;
                    break;
                case 'P': 
                    board_setup[0] |= traveling_one;
                    board_setup[2] |= traveling_one;
                    status.mailbox[__builtin_ctzl(traveling_one)] = 2;
                    ++ rank_count;
                    break;
                case 'N':
                    board_setup[0] |= traveling_one;
                    board_setup[3] |= traveling_one;
                    status.mailbox[__builtin_ctzl(traveling_one)] = 3;
                    ++ rank_count;
                    break;
                case 'B':
                    board_setup[0] |= traveling_one;
                    board_setup[4] |= traveling_one;
                    status.mailbox[__builtin_ctzl(traveling_one)] = 4;
                    ++ rank_count;
                    break;
                case 'R':
                    board_setup[0] |= traveling_one;
                    board_setup[5] |= traveling_one;
                    status.mailbox[__builtin_ctzl(traveling_one)] = 5;
                    ++ rank_count;
                    break;
                case 'Q':
                    board_setup[0] |= traveling_one;
                    board_setup[6] |= traveling_one;
                    status.mailbox[__builtin_ctzl(traveling_one)] = 6;
                    ++ rank_count;
                    break;
                case 'K':
                    board_setup[0] |= traveling_one;
                    board_setup[7] |= traveling_one;
                    status.mailbox[__builtin_ctzl(traveling_one)] = 7;
                    ++ rank_count;
                    break;
                default:
                    gap = piece_data - '0'; 
                    traveling_one >>= (gap - 1);
            }
            if (gap > 0) {
                for (int fill = (__builtin_ctzl(traveling_one)); fill < __builtin_ctzl(traveling_one) + gap; ++fill) status.mailbox[fill] = 0;
            }
            rank_count += gap;
            gap = 0;
            traveling_one >>= 1;
        }
        if (rank_count != 8) {
            std::cout << "info string A non standard English algebraic, or FEN, notation found " << " \n";
            return Chess{};
        }
    }
    // Active Color decoding
    if (active_color.size() != 1) {
        std::cout << "info string A non standard FEN notation found for Active Color " << " \n";
        return Chess{};
    }
    if (active_color == "w") {
        status.pit = false;
    }
    else {
        status.pit = true;
    }
    // Castling Availability decoding
    for (int i = 0; i < 16; ++i) {
        castle_NOK &= bool(castling_availability.compare(castle_codes[i]));
    }
    if (castle_NOK) {
        std::cout << "info string A non standard FEN notation found for Castling Availability " << " \n";
        return Chess{};
    }
    for (size_t i = 0; i < castling_availability.size(); ++i) {
        castling_data = castling_availability.at(i);
        switch(castling_data) {

            case 'K':
            board_setup[8] |= h1;
            break;
            case 'Q':
            board_setup[8] |= a1;
            break;
            case 'k':
            board_setup[8] |= h8;
            break;
            case 'q':
            board_setup[8] |= a8;
            break;
            case '-':
            break;
        }
    }
    for (int i = 0; i < 17; ++i) {
        en_passant_NOK &= bool(en_passant_target_square.compare(en_passant_codes[i]));
    }
    if (en_passant_NOK) {
        std::cout << "info string A non standard FEN notation found for En Passant Target Square " << " \n";
        return Chess{};
    }
    if (en_passant_target_square != "-") {
        for (int i = 0; i < 16; ++ i) {
            if (en_passant_target_square.compare(en_passant_codes[i]) == 0) board_setup[8] |= en_passant_squares[i];
        }
    }

    auto hm = parse_int(halfmove_clock);
    if (!hm) { std::cout << "info string A non standard FEN notation found for Halfmove Clock\n"; return Chess{}; }
    halfmove = std::min(*hm, 100);   // the status word holds 7 bits; >= 100 already means "fifty-move draw"
    board_setup[8] |= _pdep_u64(halfmove, halfmove_clock_mask); // unpacks halfmove to bit 25 - 31 in [status]

    status.bitboard = board_setup;

    return status;
}