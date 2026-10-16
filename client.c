#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "common.h"

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080


/*
 * Print the 7 x 6 Connect Four board.
 *
 * -1 = empty
 *  0 = X
 *  1 = O
 */
void print_board(int board[]) {

    printf("\n");

    for (int col = 0; col < COLS; col++) {
        printf(" %d", col);
    }

    printf("\n");

    for (int row = 0; row < ROWS; row++) {

        printf("|");

        for (int col = 0; col < COLS; col++) {

            int cell =
                board[row * COLS + col];

            if (cell == 0) {
                printf("X|");
            }
            else if (cell == 1) {
                printf("O|");
            }
            else {
                printf(".|");
            }
        }

        printf("\n");
    }

    /*
     * One blank line after the board.
     */
    printf("\n");
}


/*
 * Print the current game state.
 */
static void print_game(struct message *msg) {

    printf("Current Game\n");
    printf("---------------------\n");
    printf("GameID: %d\n", msg->gameId);

    print_board(msg->board);
}


/*
 * Send one message to the server.
 */
static int send_message(
    int sock_fd,
    struct sockaddr_in *server_addr,
    struct message *msg
) {

    ssize_t sent =
        sendto(
            sock_fd,
            msg,
            sizeof(struct message),
            0,
            (struct sockaddr *)server_addr,
            sizeof(*server_addr)
        );

    if (sent < 0) {
        return -1;
    }

    return 0;
}


/*
 * Wait for one message from the server.
 */
static int receive_message(
    int sock_fd,
    struct sockaddr_in *server_addr,
    struct message *msg
) {

    socklen_t server_len =
        sizeof(*server_addr);

    ssize_t received =
        recvfrom(
            sock_fd,
            msg,
            sizeof(struct message),
            0,
            (struct sockaddr *)server_addr,
            &server_len
        );

    if (received <= 0) {
        return -1;
    }

    return 0;
}


int main(void) {

    /*
     * test.sh redirects stdout into client1.out,
     * client2.out, etc.
     *
     * Disable buffering so every line appears
     * immediately in the output file.
     */
    setvbuf(
        stdout,
        NULL,
        _IONBF,
        0
    );


    /*
     * Create UDP socket.
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
     * Server address.
     */
    struct sockaddr_in server_addr;

    memset(
        &server_addr,
        0,
        sizeof(server_addr)
    );

    server_addr.sin_family =
        AF_INET;

    server_addr.sin_port =
        htons(SERVER_PORT);

    if (inet_pton(
            AF_INET,
            SERVER_IP,
            &server_addr.sin_addr
        ) <= 0) {

        close(sock_fd);
        return 1;
    }


    /*
     * Opening menu.
     */
    printf("Connect 4\n");
    printf("  1) Create a new game\n");
    printf("  2) Join a game\n");
    printf("> ");


    int choice = 0;

    if (scanf("%d", &choice) != 1) {

        close(sock_fd);

        return 1;
    }


    struct message request;
    struct message response;

    memset(
        &request,
        0,
        sizeof(request)
    );

    memset(
        &response,
        0,
        sizeof(response)
    );


    int my_player = -1;
    int my_game = -1;


    /*
     * =========================================================
     * CREATE NEW GAME
     * =========================================================
     */
    if (choice == 1) {

        /*
         * Reference output expects the line after:
         *
         * >
         *
         * to begin Current Game.
         */
        printf("\n");

        request.type =
            MSG_CREATE;


        if (send_message(
                sock_fd,
                &server_addr,
                &request
            ) != 0) {

            close(sock_fd);

            return 1;
        }


        if (receive_message(
                sock_fd,
                &server_addr,
                &response
            ) != 0) {

            close(sock_fd);

            return 1;
        }


        if (response.type != MSG_STATE) {

            close(sock_fd);

            return 1;
        }


        my_player =
            response.player;

        my_game =
            response.gameId;


        /*
         * Show the newly created empty game.
         */
        print_game(&response);


        /*
         * Player 0 normally creates the game first
         * and waits for Player 1 to join.
         */
        if (response.player_count < 2) {

            printf(
                "waiting for other player...\n\n"
            );


            /*
             * Server sends another state after
             * Player 1 joins.
             */
            if (receive_message(
                    sock_fd,
                    &server_addr,
                    &response
                ) != 0) {

                close(sock_fd);

                return 1;
            }


            print_game(&response);
        }
    }


    /*
     * =========================================================
     * JOIN EXISTING GAME
     * =========================================================
     */
    else if (choice == 2) {

        /*
         * Notice that there is intentionally NO newline
         * between the initial "> " and "Game ID? >".
         *
         * The reference output expects:
         *
         * > Game ID? >
         */
        printf("Game ID? >");


        int gameId = -1;

        if (scanf("%d", &gameId) != 1) {

            close(sock_fd);

            return 1;
        }


        /*
         * After reading the Game ID, Current Game
         * begins on the next line.
         */
        printf("\n");


        /*
         * test.sh launches the clients almost at the
         * same time.
         *
         * Client 2 can therefore ask to join Game 0
         * slightly before Client 1 has created it.
         *
         * Retry until the game becomes available.
         */
        int joined = 0;

        while (!joined) {

            memset(
                &request,
                0,
                sizeof(request)
            );


            request.type =
                MSG_JOIN;

            request.gameId =
                gameId;


            if (send_message(
                    sock_fd,
                    &server_addr,
                    &request
                ) != 0) {

                close(sock_fd);

                return 1;
            }


            if (receive_message(
                    sock_fd,
                    &server_addr,
                    &response
                ) != 0) {

                close(sock_fd);

                return 1;
            }


            if (response.type ==
                MSG_STATE) {

                joined = 1;
            }
            else {

                /*
                 * Game probably does not exist yet.
                 * Wait 0.1 second and retry.
                 */
                usleep(100000);
            }
        }


        my_player =
            response.player;

        my_game =
            response.gameId;


        print_game(&response);
    }


    /*
     * Invalid menu choice.
     */
    else {

        close(sock_fd);

        return 1;
    }


    /*
     * =========================================================
     * MAIN GAME LOOP
     * =========================================================
     */
    while (1) {

        /*
         * Game may already have ended.
         */
        if (response.finished) {

            if (response.winner ==
                my_player) {

                printf("YOU WIN!\n");
            }
            else {

                printf("YOU LOSE!\n");
            }

            break;
        }


        /*
         * =====================================================
         * MY TURN
         * =====================================================
         */
        if (response.turn ==
            my_player) {

            /*
             * print_game() already generated one blank line.
             *
             * The reference output contains another blank
             * line before Input Move Row.
             */
            printf("\n");
            printf("Input Move Row: > ");


            int move = -1;

            if (scanf("%d", &move) != 1) {

                close(sock_fd);

                return 1;
            }


            /*
             * Input is piped by test.sh and therefore is not
             * echoed. Explicitly move to the next line.
             */
            printf("\n");


            memset(
                &request,
                0,
                sizeof(request)
            );


            request.type =
                MSG_MOVE;

            request.gameId =
                my_game;

            request.player =
                my_player;

            request.move =
                move;


            if (send_message(
                    sock_fd,
                    &server_addr,
                    &request
                ) != 0) {

                close(sock_fd);

                return 1;
            }


            /*
             * Receive updated board after my move.
             */
            if (receive_message(
                    sock_fd,
                    &server_addr,
                    &response
                ) != 0) {

                close(sock_fd);

                return 1;
            }


            print_game(&response);


            /*
             * Did my move win?
             */
            if (response.finished) {

                if (response.winner ==
                    my_player) {

                    printf("YOU WIN!\n");
                }
                else {

                    printf("YOU LOSE!\n");
                }

                break;
            }


            /*
             * Do NOT print waiting here.
             *
             * The next loop iteration sees that it is
             * the opponent's turn and prints waiting
             * exactly once.
             */
        }


        /*
         * =====================================================
         * OPPONENT'S TURN
         * =====================================================
         */
        else {

            printf(
                "waiting for other player...\n\n"
            );


            /*
             * Wait for server to send the board after
             * the opponent makes a move.
             */
            if (receive_message(
                    sock_fd,
                    &server_addr,
                    &response
                ) != 0) {

                close(sock_fd);

                return 1;
            }


            print_game(&response);


            /*
             * Did the opponent win?
             */
            if (response.finished) {

                if (response.winner ==
                    my_player) {

                    printf("YOU WIN!\n");
                }
                else {

                    printf("YOU LOSE!\n");
                }

                break;
            }
        }
    }


    close(sock_fd);

    return 0;
}