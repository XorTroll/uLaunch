
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

    class Lock {
        private:
            ::Mutex mutex;

        public:
            constexpr Lock() : mutex() {}

            inline void lock() {
                mutexLock(&this->mutex);
            }

            inline void unlock() {
                mutexUnlock(&this->mutex);
            }

            inline bool try_lock() {
                return mutexTryLock(&this->mutex);
            }
    };

    class RecursiveLock {
        private:
            ::RMutex mutex;

        public:
            constexpr RecursiveLock() : mutex() {}

            inline void lock() {
                rmutexLock(&this->mutex);
            }

            inline void unlock() {
                rmutexUnlock(&this->mutex);
            }

            inline bool try_lock() {
                return rmutexTryLock(&this->mutex);
            }
    };

    inline std::string JoinPath(const std::string &a, const std::string &b) {
        return a + "/" + b;
    }

}