#ifndef COMMON_H
#define COMMON_H

#include <arpa/inet.h>
#include <sys/socket.h>

#define ROWS 6
#define COLS 7
#define MAX_GAMES 100

/*
 * Cell values:
 * -1 = empty
 *  0 = player 0 / X
 *  1 = player 1 / O
 */
struct board {
    int cells[ROWS * COLS];
};

struct game {
    int gameId;

    struct board board;

    int turn;

    int player_count;

    int finished;

    int winner;

    struct sockaddr_in players[2];

    int player_connected[2];
};

struct gameDB {
    struct game games[MAX_GAMES];

    int game_count;
};
enum message_type {
    MSG_CREATE = 1,
    MSG_JOIN,
    MSG_MOVE,
    MSG_STATE,
    MSG_ERROR
};

struct message {
    int type;

    int gameId;
    int player;
    int move;

    int board[ROWS * COLS];

    int turn;
    int player_count;
    int winner;
    int finished;
};
#endif