#include <termios.h>
#include <errno.h>

int tcsetattr(int fd, int optional_actions, const struct termios *termios_p) {
    errno = EINVAL;
    return -1;
}

int tcgetattr(int fd, struct termios *termios_p) {
    errno = EINVAL;
    return -1;
}