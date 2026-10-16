#include "game.h"
#include <stdlib.h>
#include <stddef.h>

/*
Takes as input a pointer to a connect4 game board, a player (0/1), and a int column.
Places a new piece in `col` on the game board.
*/
int make_move(struct game* g, int player, int col) {

    if (g == NULL) {
        return -1;
    }

    if (col < 0 || col >= COLS) {
        return -1;
    }

    if (player != g->turn) {
        return -1;
    }

    /*
     * Start from the bottom row and move upward.
     * The first empty cell is where the piece lands.
     */
    for (int row = ROWS - 1; row >= 0; row--) {

        int index = row * COLS + col;

        if (g->board.cells[index] == -1) {

            g->board.cells[index] = player;

            /*
             * Switch to the other player's turn.
             */
            g->turn = 1 - player;

            return 0;
        }
    }

    /*
     * No empty cell was found, so the column is full.
     */
    return -1;
} 
/*
Takes as input a pointer to a connect4 game board.
Returns 0 or 1 if player 0 or 1 has won respectively.
Returns -1 if there is no winner.
*/
int check_win(struct game* g) {

    if (g == NULL) {
        return -1;
    }

    /*
     * Check horizontal wins.
     */
    for (int row = 0; row < ROWS; row++) {

        for (int col = 0; col <= COLS - 4; col++) {

            int p = g->board.cells[row * COLS + col];

            if (p != -1 &&
                g->board.cells[row * COLS + col + 1] == p &&
                g->board.cells[row * COLS + col + 2] == p &&
                g->board.cells[row * COLS + col + 3] == p) {

                return p;
            }
        }
    }

    /*
     * Check vertical wins.
     */
    for (int row = 0; row <= ROWS - 4; row++) {

        for (int col = 0; col < COLS; col++) {

            int p = g->board.cells[row * COLS + col];

            if (p != -1 &&
                g->board.cells[(row + 1) * COLS + col] == p &&
                g->board.cells[(row + 2) * COLS + col] == p &&
                g->board.cells[(row + 3) * COLS + col] == p) {

                return p;
            }
        }
    }

    /*
     * Check diagonal wins going down-right.
     */
    for (int row = 0; row <= ROWS - 4; row++) {

        for (int col = 0; col <= COLS - 4; col++) {

            int p = g->board.cells[row * COLS + col];

            if (p != -1 &&
                g->board.cells[(row + 1) * COLS + col + 1] == p &&
                g->board.cells[(row + 2) * COLS + col + 2] == p &&
                g->board.cells[(row + 3) * COLS + col + 3] == p) {

                return p;
            }
        }
    }

    /*
     * Check diagonal wins going down-left.
     */
    for (int row = 0; row <= ROWS - 4; row++) {

        for (int col = 3; col < COLS; col++) {

            int p = g->board.cells[row * COLS + col];

            if (p != -1 &&
                g->board.cells[(row + 1) * COLS + col - 1] == p &&
                g->board.cells[(row + 2) * COLS + col - 2] == p &&
                g->board.cells[(row + 3) * COLS + col - 3] == p) {

                return p;
            }
        }
    }

    /*
     * Nobody has connected four yet.
     */
    return -1;
}
 
/*
Initializes an existing game with an empty board.
*/
void new_game(struct game* g, int gameId) {

    g->gameId = gameId;

    g->turn = 0;

    g->player_count = 0;

    g->finished = 0;

    g->winner = -1;

    g->player_connected[0] = 0;
    g->player_connected[1] = 0;

    for (int i = 0; i < ROWS * COLS; i++) {
        g->board.cells[i] = -1;
    }
}


