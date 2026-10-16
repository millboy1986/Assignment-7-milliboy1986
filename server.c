#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "game.h"
#include "save.h"

#define PORT 8080
#define SAVE_FILE "games.dat"


/*
 * Copy the current game information into a message
 * that can be sent to a client.
 */
static void fill_state_message(
    struct message *msg,
    struct game *g
) {
    memset(
        msg,
        0,
        sizeof(struct message)
    );

    msg->type = MSG_STATE;

    msg->gameId =
        g->gameId;

    msg->turn =
        g->turn;

    msg->player_count =
        g->player_count;

    msg->winner =
        g->winner;

    msg->finished =
        g->finished;


    for (int i = 0;
         i < ROWS * COLS;
         i++) {

        msg->board[i] =
            g->board.cells[i];
    }
}


/*
 * Send the current game state to one player.
 */
static void send_state(
    int sock_fd,
    struct game *g,
    int player
) {
    /*
     * Make sure the player exists.
     */
    if (player < 0 ||
        player > 1 ||
        !g->player_connected[player]) {

        return;
    }


    struct message response;

    fill_state_message(
        &response,
        g
    );


    /*
     * Tell the client which player number
     * belongs to it.
     */
    response.player =
        player;


    sendto(
        sock_fd,
        &response,
        sizeof(response),
        0,
        (struct sockaddr *)
            &g->players[player],
        sizeof(g->players[player])
    );
}


/*
 * Send the same updated game state
 * to both connected players.
 */
static void send_state_to_players(
    int sock_fd,
    struct game *g
) {
    send_state(
        sock_fd,
        g,
        0
    );

    send_state(
        sock_fd,
        g,
        1
    );
}


/*
 * Send an error response to a client.
 */
static void send_error(
    int sock_fd,
    struct sockaddr_in *client_addr,
    socklen_t client_len
) {
    struct message response;

    memset(
        &response,
        0,
        sizeof(response)
    );

    response.type =
        MSG_ERROR;


    sendto(
        sock_fd,
        &response,
        sizeof(response),
        0,
        (struct sockaddr *)client_addr,
        client_len
    );
}


int main(
    int argc,
    char *argv[]
) {

    /*
     * ---------------------------------------------------------
     * Create UDP socket
     * ---------------------------------------------------------
     */
    int sock_fd =
        socket(
            AF_INET,
            SOCK_DGRAM,
            0
        );

    if (sock_fd < 0) {
        perror("socket");
        return 1;
    }


    /*
     * ---------------------------------------------------------
     * Set up server address
     * ---------------------------------------------------------
     */
    struct sockaddr_in server_addr;

    memset(
        &server_addr,
        0,
        sizeof(server_addr)
    );

    server_addr.sin_family =
        AF_INET;

    server_addr.sin_addr.s_addr =
        INADDR_ANY;

    server_addr.sin_port =
        htons(PORT);


    /*
     * ---------------------------------------------------------
     * Bind socket to port 8080
     * ---------------------------------------------------------
     */
    if (bind(
            sock_fd,
            (struct sockaddr *)
                &server_addr,
            sizeof(server_addr)
        ) < 0) {

        perror("bind");

        close(sock_fd);

        return 1;
    }


    /*
     * ---------------------------------------------------------
     * Game database
     * ---------------------------------------------------------
     */
    struct gameDB db;

    memset(
        &db,
        0,
        sizeof(db)
    );


    /*
     * ---------------------------------------------------------
     * Recovery mode
     *
     * ./server 1
     *
     * Load the previous server state from disk.
     * ---------------------------------------------------------
     */
    if (argc > 1 &&
        strcmp(argv[1], "1") == 0) {

        if (loadgame(
                &db,
                SAVE_FILE
            ) != 0) {

            /*
             * If loading fails, start with
             * an empty database.
             */
            memset(
                &db,
                0,
                sizeof(db)
            );

            db.game_count = 0;
        }
    }


    /*
     * ---------------------------------------------------------
     * Normal startup
     *
     * ./server
     *
     * Start fresh and remove any old save file.
     * ---------------------------------------------------------
     */
    else {

        memset(
            &db,
            0,
            sizeof(db)
        );

        db.game_count = 0;

        /*
         * Important:
         * Old test runs should not affect a new game.
         */
        unlink(SAVE_FILE);
    }


    printf(
        "Message board server listening on port %d\n",
        PORT
    );


    /*
     * ---------------------------------------------------------
     * Main server loop
     * ---------------------------------------------------------
     */
    while (1) {

        struct message request;

        struct sockaddr_in client_addr;

        socklen_t client_len =
            sizeof(client_addr);


        memset(
            &request,
            0,
            sizeof(request)
        );

        memset(
            &client_addr,
            0,
            sizeof(client_addr)
        );


        /*
         * Wait for a UDP message.
         */
        ssize_t received =
            recvfrom(
                sock_fd,
                &request,
                sizeof(request),
                0,
                (struct sockaddr *)
                    &client_addr,
                &client_len
            );


        if (received <= 0) {
            continue;
        }


        /*
         * =====================================================
         * CREATE
         * =====================================================
         */
        if (request.type ==
            MSG_CREATE) {

            /*
             * Make sure there is space
             * for another game.
             */
            if (db.game_count >=
                MAX_GAMES) {

                send_error(
                    sock_fd,
                    &client_addr,
                    client_len
                );

                continue;
            }


            /*
             * Game IDs increase:
             *
             * 0, 1, 2, ...
             */
            int gameId =
                db.game_count;


            struct game *g =
                &db.games[gameId];


            /*
             * Initialize the new game.
             */
            new_game(
                g,
                gameId
            );


            /*
             * The creator becomes
             * Player 0 / X.
             */
            g->players[0] =
                client_addr;

            g->player_connected[0] =
                1;

            g->player_count =
                1;


            /*
             * Add the game to the DB.
             */
            db.game_count++;


            /*
             * Save immediately.
             *
             * This also preserves the UDP address
             * of Player 0 for recovery.
             */
            savegame(
                &db,
                SAVE_FILE
            );


            /*
             * Send the new empty game back
             * to the creator.
             */
            send_state(
                sock_fd,
                g,
                0
            );


            continue;
        }


        /*
         * =====================================================
         * JOIN
         * =====================================================
         */
        if (request.type ==
            MSG_JOIN) {

            int gameId =
                request.gameId;


            /*
             * Requested game does not exist yet.
             *
             * client.c will briefly wait and retry.
             */
            if (gameId < 0 ||
                gameId >= db.game_count) {

                send_error(
                    sock_fd,
                    &client_addr,
                    client_len
                );

                continue;
            }


            struct game *g =
                &db.games[gameId];


            /*
             * Add Player 1 / O.
             */
            if (!g->player_connected[1]) {

                g->players[1] =
                    client_addr;

                g->player_connected[1] =
                    1;

                g->player_count =
                    2;
            }


            /*
             * Save after JOIN.
             *
             * This is important for recovery because
             * the saved game now contains both client
             * UDP addresses.
             */
            savegame(
                &db,
                SAVE_FILE
            );


            /*
             * Tell both players the game is ready.
             */
            send_state_to_players(
                sock_fd,
                g
            );


            continue;
        }


        /*
         * =====================================================
         * MOVE
         * =====================================================
         */
        if (request.type ==
            MSG_MOVE) {

            int gameId =
                request.gameId;

            int player =
                request.player;

            int col =
                request.move;


            /*
             * Validate game ID.
             */
            if (gameId < 0 ||
                gameId >= db.game_count) {

                continue;
            }


            /*
             * Validate player number.
             */
            if (player < 0 ||
                player > 1) {

                continue;
            }


            struct game *g =
                &db.games[gameId];


            /*
             * Do not process moves after
             * a game has finished.
             */
            if (g->finished) {
                continue;
            }


            /*
             * Try to place the piece.
             *
             * make_move() also checks:
             * - valid column
             * - correct player's turn
             * - full column
             */
            if (make_move(
                    g,
                    player,
                    col
                ) != 0) {

                continue;
            }


            /*
             * Check whether the move created
             * four-in-a-row.
             */
            int winner =
                check_win(g);


            if (winner != -1) {

                g->winner =
                    winner;

                g->finished =
                    1;
            }


            /*
             * -------------------------------------------------
             * IMPORTANT FOR CRASH RECOVERY
             *
             * Save the updated game BEFORE sending
             * the result to the clients.
             *
             * If the server dies after this point,
             * ./server 1 can reload this state.
             * -------------------------------------------------
             */
            savegame(
                &db,
                SAVE_FILE
            );


            /*
             * Send the new board to both clients.
             */
            send_state_to_players(
                sock_fd,
                g
            );


            continue;
        }
    }


    /*
     * Normally unreachable because the server
     * runs forever.
     */
    close(sock_fd);

    return 0;
}