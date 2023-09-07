
#pragma once
#include <switch.h>
#include <vector>
#include <string>
#include <unistd.h>
#include <cstdio>

namespace ul {

    constexpr const char DefaultThemePath[] = "romfs:/default";
    constexpr const char DefaultLanguagePath[] = "romfs:/lang";
    constexpr const char DefaultLanguage[] = "en-US";

    constexpr const char RootPath[] = "sdmc:/ulaunch";

    constexpr const char ConfigPath[] = "sdmc:/ulaunch/config.cfg";

    constexpr const char ThemesPath[] = "sdmc:/ulaunch/themes";

    constexpr const char MenuPath[] = "sdmc:/ulaunch/menu";
    constexpr const char MenuLanguagesPath[] = "sdmc:/ulaunch/lang/uMenu";

    constexpr const char RootCachePath[] = "sdmc:/ulaunch/cache";
    constexpr const char ApplicationCachePath[] = "sdmc:/ulaunch/cache/app";
    constexpr const char HomebrewCachePath[] = "sdmc:/ulaunch/cache/hb";
    constexpr const char AccountCachePath[] = "sdmc:/ulaunch/cache/acc";

    constexpr const char ManagerLanguagesPath[] = "sdmc:/ulaunch/lang/uManager";

    constexpr const char OldMenuPath[] = "sdmc:/ulaunch/entries";
    constexpr const char OldApplicationCachePath[] = "sdmc:/ulaunch/titles";
    constexpr const char OldHomebrewCachePath[] = "sdmc:/ulaunch/nro";
    constexpr const char OldAccountCachePath[] = "sdmc:/ulaunch/user";

    constexpr const char HbmenuPath[] = "sdmc:/hbmenu.nro";
    constexpr const char ManagerPath[] = "sdmc:/switch/uManager.nro";
    constexpr const char RootHomebrewPath[] = "sdmc:/switch";

    class Mutex {
        private:
            ::Mutex mutex;

        public:
            constexpr Mutex() : mutex() {}

            inline void Lock() {
                mutexLock(&this->mutex);
            }

            inline void Unlock() {
                mutexUnlock(&this->mutex);
            }

            inline bool TryLock() {
                return mutexTryLock(&this->mutex);
            }
    };

    class RecursiveMutex {
        private:
            ::RMutex mutex;

        public:
            constexpr RecursiveMutex() : mutex() {}

            inline void Lock() {
                rmutexLock(&this->mutex);
            }

            inline void Unlock() {
                rmutexUnlock(&this->mutex);
            }

            inline bool TryLock() {
                return rmutexTryLock(&this->mutex);
            }
    };

    template<typename T>
    class ScopedLock {
        private:
            T &lock;
        
        public:
            ScopedLock(T &lock) : lock(lock) {
                lock.Lock();
            }

            ~ScopedLock() {
                this->lock.Unlock();
            }
    };

}