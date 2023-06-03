
#ifndef __DLFCN_H__
#define __DLFCN_H__


void* dlopen(const char* __filename, int __flag);
int dlclose(void* __handle);
char* dlerror(void);
void* dlsym(void* __handle, const char* __symbol);

#define RTLD_LOCAL    0
#define RTLD_LAZY     0x00001
#define RTLD_NOW      0x00002
#define RTLD_NOLOAD   0x00004
#define RTLD_GLOBAL   0x00100
#define RTLD_NODELETE 0x01000

#endif