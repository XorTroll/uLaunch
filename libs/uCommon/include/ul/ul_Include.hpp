
#pragma once
#include <switch.h>
#include <vector>
#include <string>
#include <unistd.h>
#include <cstdio>

namespace ul {

    constexpr const char DefaultThemePath[] = "romfs:/default";
    constexpr const char DefaultLanguagePath[] = "romfs:/en.json";

    constexpr const char ConfigPath[] = "sdmc:/ulaunch/config.cfg";
    constexpr const char ThemesPath[] = "sdmc:/ulaunch/themes";
    constexpr const char EntriesPath[] = "sdmc:/ulaunch/themes";
    constexpr const char TitleCachePath[] = "sdmc:/ulaunch/titles";
    constexpr const char LanguagesPath[] = "sdmc:/ulaunch/lang";
    constexpr const char AccountCachePath[] = "sdmc:/ulaunch/user";
    constexpr const char HomebrewCachePath[] = "sdmc:/ulaunch/nro";
    constexpr const char DefaultHomebrewIconPath[] = "sdmc:/ulaunch/default_hb_icon.jpg";
    constexpr const char DefaultHomebrewNacpPath[] = "sdmc:/ulaunch/default_hb_nacp.nacp";
    constexpr const char AssertionLogFile[] = "sdmc:/ulaunch/assert.log";

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

    inline std::string JoinPath(const std::string &a, const std::string &b) {
        return a + "/" + b;
    }

}