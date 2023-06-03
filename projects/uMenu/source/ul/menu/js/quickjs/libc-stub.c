#include <stdio.h>
#include <errno.h>

FILE *popen(const char *command, const char *type) {
    return NULL;
}

int pclose(FILE *stream) {
    errno = ENOSYS;
    return -1;
}

long sysconf(int name) {
    errno = EINVAL;
    return -1;
}

int execv(const char *path, char *const argv[]) {
    errno = ENOSYS;
    return -1;
}

int execvp(const char *file, char *const argv[]) {
    errno = ENOSYS;
    return -1;
}

int pipe(int fds[2]) {
    errno = ENOSYS;
    return -1;
}

char *realpath(const char *path, char *resolved_path) {
    errno = EINVAL;
    return NULL;
}

pid_t waitpid(pid_t pid, int *status, int options) {
    errno = ENOSYS;
    return -1;
}

ssize_t readlink(const char *pathname, char *buf, size_t bufsiz) {
    errno = EINVAL;
    return -1;
}

int symlink(const char *path1, const char *path2) {
    errno = EINVAL;
    return -1;
}