#include <dlfcn.h>
#include <stddef.h>

void *dlopen(const char *__filename, int __flag) {
    return NULL;
}

int dlclose(void *__handle) {
    return 0;
}

char *dlerror(void) {
    return "dlfcn is stubbed!";
}

void *dlsym(void *__handle, const char *__symbol) {
    return NULL;
}