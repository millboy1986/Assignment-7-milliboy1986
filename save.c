#include <fcntl.h>
#include <unistd.h>
#include <stddef.h>

#include "save.h"


static int write_all(int fd, const void *buffer, size_t count) {

    const char *ptr = buffer;

    while (count > 0) {

        ssize_t written =
            write(fd, ptr, count);

        if (written <= 0) {
            return -1;
        }

        ptr += written;
        count -= (size_t)written;
    }

    return 0;
}


static int read_all(int fd, void *buffer, size_t count) {

    char *ptr = buffer;

    while (count > 0) {

        ssize_t n =
            read(fd, ptr, count);

        if (n <= 0) {
            return -1;
        }

        ptr += n;
        count -= (size_t)n;
    }

    return 0;
}


int savegame(struct gameDB* game, char* filepath) {

    if (game == NULL || filepath == NULL) {
        return -1;
    }

    int fd =
        open(
            filepath,
            O_WRONLY | O_CREAT | O_TRUNC,
            0644
        );

    if (fd < 0) {
        return -1;
    }

    int result =
        write_all(
            fd,
            game,
            sizeof(struct gameDB)
        );

    close(fd);

    return result;
}


int loadgame(struct gameDB* game, char* filepath) {

    if (game == NULL || filepath == NULL) {
        return -1;
    }

    int fd =
        open(
            filepath,
            O_RDONLY
        );

    if (fd < 0) {
        return -1;
    }

    int result =
        read_all(
            fd,
            game,
            sizeof(struct gameDB)
        );

    close(fd);

    return result;
}