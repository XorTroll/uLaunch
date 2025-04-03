<p align="center">
  <img alt="uLaunch" height="100" src="assets/Logo.png">
</p

***uLaunch*** is an open-source, customizable and homebrew-oriented replacement for the *Nintendo Switch HOME Menu*:

<p align="center">
  <img src="demos/uLaunch.gif"/>
</p>

<p align="center">
  <a title="Discord" href="https://discord.gg/3KpFyaH">
    <img alt="Discord" src="https://img.shields.io/discord/789833418631675954?label=Discord&logo=Discord&logoColor=fff&style=for-the-badge">
  </a>
  <a title="Downloads" href="https://github.com/XorTroll/uLaunch/releases/latest">
    <img alt="Downloads" src="https://img.shields.io/github/downloads/XorTroll/uLaunch/total?longCache=true&style=for-the-badge&label=Downloads&logoColor=fff&logo=GitHub">
  </a>
  <a title="License" href="https://github.com/XorTroll/uLaunch/blob/master/LICENSE">
    <img alt="License" src="https://img.shields.io/github/license/XorTroll/uLaunch?style=for-the-badge">
  </a>
</p>

<p align="center">
  <a title="Patreon" href="https://www.patreon.com/xortroll">
    <img alt="Patreon" src="https://img.shields.io/endpoint.svg?url=https%3A%2F%2Fshieldsio-patreon.vercel.app%2Fapi%3Fusername%3DXorTroll%26type%3Dpatrons&style=for-the-badge"/>
  </a>
  <a title="GitHub sponsors" href="https://github.com/sponsors/XorTroll">
    <img alt="GitHub sponsors" src="https://img.shields.io/github/sponsors/XorTroll?label=Sponsor&logo=GitHub&style=for-the-badge"/>
  </a>
  <a title="PayPal" href="https://www.paypal.com/donate/?hosted_button_id=PHQKFTY9AHPUU">
    <img alt="PayPal" src="https://img.shields.io/badge/Donate-PayPal-green.svg?style=for-the-badge"/>
  </a>
</p>

<p align="center">
  <a title="Ko-fi" href='https://ko-fi.com/xortroll' target='_blank'>
    <img alt="Ko-fi" height='35' style='border:0px;height:46px;' src='https://az743702.vo.msecnd.net/cdn/kofi3.png?v=0' border='0'/>
  </a>
</p>

### Want to find **themes** for uLaunch? Check the `ulaunch-themes` channel on our [Discord server](https://discord.gg/3KpFyaH)!

### Want to make your own uLaunch **themes**? Check our [web theme editor](https://xortroll.github.io/uLaunch/) or the [wiki](https://github.com/XorTroll/uLaunch/wiki)!

<p align="center">
  <img src="demos/uDesigner.gif"/>
</p>

- [Features](#features)
  - [Custom features](#custom-features)
  - [Implemented base features](#implemented-base-features)
  - [Unimplemented base features](#unimplemented-base-features)
    - [Planned to be implemented](#planned-to-be-implemented)
    - [Not planned to be implemented (at least for now)](#not-planned-to-be-implemented-at-least-for-now)
- [Setup](#setup)
  - [Installing uLaunch](#installing-ulaunch)
  - [Removing uLaunch](#removing-ulaunch)
- [FAQ](#faq)
- [Translating](#translating)
- [Components](#components)
  - [uSystem](#usystem)
  - [uMenu](#umenu)
  - [uLoader](#uloader)
  - [uManager](#umanager)
  - [uScreen](#uscreen)
    - [Windows](#windows)
    - [Linux](#linux)
    - [Mac](#mac)
  - [uDesigner](#udesigner)
- [Building](#building)
- [Credits](#credits)

## Features

### Custom features

List of unique extensions that the official HOME Menu lacks:

- User login system (login once, use that user for everything)

- Grid-like main menu, deeply inspired by the 3DS menu (and partially DSi/Wii menus as well), easier than ever to navigate and customize

  - The grid can be resized like in 3DS menus

  - Entries can be moved as desired

- Folders, subfolders...

- Homebrew directly launchable straight from main menu as an applet or an application (using applications as donors)

- Extensive themeing support

  - Backgrounds, icons, etc.

  - BGM and many sound effects

- Web browser easily accessible straight from main menu

- Mii editor easily accessible straight from main menu

- Screen capture to PC support via USB

- Display system version + Atmosphère version + EmuMMC presence independently, in a nicer way

- Better display when gamecard fails to mount

- 1080p resolution


### Implemented base features

List of implemented official HOME Menu features:

- Application launching, suspending and closing

- Applet launching and closing

- User page

- Controller support

- Settings (only a handful are so far implemented):

  - Console version

  - Atmosphère version

  - EmuMMC presence

  - Console nickname

  - Console timezone

  - WiFi connection name/WiFi settings

  - Console language

  - Console information upload (enable/disable)

  - Bluetooth (enable/disable)

  - NFC (enable/disable)

  - Automatic application download (enable/disable)

  - Automatic console update (enable/disable)

  - Wireless LAN (enable/disable)

  - Console serial number

  - Show console IP/MAC address

- General channel/applet messages (some of them aren't implemented yet):

  - HOME button detection

  - Power off, sleep, reboot

  - SD card removal

  - Gamecard failing to mount

### Unimplemented base features

List of not implemented official HOME Menu features:

#### Planned to be implemented

- Auto-sleep after a certain amount of time

- Several unimplemented settings

- Several unimplemented general channel/applet messages

- Console updating

#### Not planned to be implemented (at least for now)

- eShop functionality

- Parental control

- Application (game) updates

> Note that a lot of features could be considered in principle (savedata support, title removing, user management, activity log stuff...), but uLaunch's philosophy is to mainly avoid considering this features or, more generally, features already provided by existing homebrews or potentially providable by them. uLaunch is a custom HOME Menu, whose aim partially is to be extended with mostly HOME-menu-related features.

## Setup

### Installing uLaunch

1. Download the latest release ZIP.

> Note: this project is released and meant to be used with Atmosphère, so use it with different CFWs at your own risk.

2. Copy everything inside the ZIP to the root of your SD card.

   - If you have never used uLaunch or any kind of HOME Menu replacement (NXThemes don't count) you wouldn't need to overwrite any files.

   - You don't need to remove your normal HOME Menu themes (NXThemes) in order to install uLaunch. Those themes are at a `romfs` dir inside `contents/0100000000001000`, while uLaunch's only file in that folder is `contents/0100000000001000/exefs.nsp`.

3. Launch your CFW (using emuMMC or sysMMC shouldn't make a difference) and enjoy your new HOME Menu!

### Removing uLaunch

> Important: make sure you don't remove anything else but the stuff mentioned here, in order to avoid any potential trouble!

1. If you'd like to keep your custom themes, menu entries, config, etc. then just use the `uManager` tool to disable uLaunch. This doesn't remove any data, allowing you to re-enable it back any moment.

2. If you'd like to remove everything permanently, then you will have to remove `atmosphere/contents/0100000000001000` and `ulaunch` folders on the SD card. If you also wish to remove `uManager`, then remove `switch/uManager.nro`.

> Note: if you use any HOME Menu modification - like NXThemes - make sure you do not delete the entire `0100000000001000` folder, just the `exefs.nsp` file!

## FAQ

- uLaunch gives me a blackscreen. How can I fix it?

  - First of all, make sure you're using the latest release of uLaunch.

  - If new firmware updates or new Atmosphère versions have been released, you might need to wait for a new release to be dropped, and in the meantime you might be able to use dev/testing builds from our [Discord server](https://discord.gg/3KpFyaH).

  - The *log files* uLaunch's components generate inside `ulaunch` folder can be really helpful when sharing your issue in Discord or GitHub. Make sure you don't reboot again into uLaunch, since the log files will be overwritten! Otherwise, just replicate the bug and share/copy the log files before reloading anything.

  - In more extreme cases, uLaunch's blackscreens might also be caused due bad handling of invalid theme/entry JSON files. The JSONs might have been corrupted (due to ExFAT, other homebrews...) so try deleting them.

- Why can't I access the usual system settings, while I can access other normal system menus like the album, mii editor, user page, etc.?

  - This is an unfortunate technical issue. While the web browser, user page, album... are separate applets (separate programs, independent from the HOME Menu itself) system settings are *actually* part of HOME Menu itself. Therefore, we have to implement manually all of them... which requires its effort, so only a bunch of the available settings (plus a few extras) are currently available here, while the remaining settings are being reversed and implemented.

- Will using uLaunch get me banned online?

  - There have been some cases where using uLaunch has caused bans. Keep in mind that replacing the official HOME Menu's functionality is never a completely safe idea, so always use it at your own risk. Since uLaunch doesn't perform any telemetry or communications with Nintendo servers, they might be able to notice you are running something different from the original HOME Menu.

- Why does uLaunch (sometimes) feel slower than the official HOME Menu?

  - There are several possible reasons:

    - uLaunch loads more content than the official HOME Menu when loading. Most of the official HOME Menu's UI are solid colors, while uLaunch loads several images, etc. Being customizable comes with minor drawbacks, like this one.

    - Icons are lazily loaded, so for menus with many entries (essentially for people having a ton of games) navigating through the menu will be slightly laggy until everything loads, which will just take a few seconds. The 3DS menu has similar laggy moments, by the way ;)

    - Aside from the two excuses above, there is always room for further optimizations in uLaunch's code. Feel free to submit any issues of excessive lag/slowdowns, I'll do my best to improve it :)

## Translating

Translations for uLaunch are always welcome!

The files to be translated are [uMenu translations](https://github.com/XorTroll/uLaunch/blob/unew/projects/uMenu/romfs/lang/en.json) and [uManager translations](https://github.com/XorTroll/uLaunch/blob/unew/projects/uManager/romfs/lang/en.json). They are pretty straightforward to understand, just JSON files with English sentences meant to be translated to **any language officially supported by the Nintendo Switch**.

Character `\n` is a new-line escape (indicates a new line) and must remain unchanged. Punctuation marks (mainly dots at the end of sentences, questions, double dots) must be respected, with perhaphs the exception of commas in the middle of sentences.

Feel free to open pull requests for translations, but it's better to keep contact with me through [Discord](https://discord.gg/3KpFyaH).

As I work on new features and new strings are needed, I will manually add them as English on other language files to keep support.

## Components

### uSystem

uSystem is the *system applet process* replacement - the actual, literal HOME Menu replacement - and serves as a *backend* for the actual menu the user will interact with (uMenu).

uSystem handles essential HOME Menu functionality, like suspending/closing titles, closing applets, detecting power button or HOME button presses... it's very lightweight since it must always be running in the background.

It also communicates with uMenu for anything necessary via their `smi` communication system.

### uMenu

uMenu is a *library applet* (running temporarily over the *eShop applet* when launched) which is **the HOME Menu the user will see and interact with**.

uMenu is launched and terminated by uSystem when necessary. In order to deal with special HOME Menu functionality (closing titles, launching them...) it communicates with uSystem via their `smi` communication system.

### uLoader

uLoader is a custom [nx-hbloader](https://github.com/switchbrew/nx-hbloader/releases) implementation, which allows to easily *launch homebrew* as applets or applications or even to *choose homebrews* as some sort of file dialog.

### uManager

uManager is a homebrew NRO used for controlling key uLaunch aspects.

Only those tasks which cannot be performed outside uLaunch are controlled (like enabling or disabling uLaunch itself, hence why this is a separate NRO and not part of uMenu/etc), while everything else is controlled and managed on uLaunch itself.

### uScreen

uScreen is a PC tool that communicates via USB with uSystem in order to capture the screen. Requires Java 8 or higher, but versions above that are recommended.

> **IMPORTANT**! This should not be used for real-time recording due to the notorious lag! For those purposes use other tools like [SysDVR](https://github.com/exelix11/SysDVR). This feature is meant for capturing where other screen-recording tools do not work (HOME Menu, system settings and other similar places)

Note that USB screen capture is disabled by default, and has to be enabled in settings.

You also need to install libusbK drivers for USB to work fine:

#### Windows

The best way to install Java 9 in Windows (or a very simple one) is to install [AdoptOpenJDK 11 or higher](https://adoptopenjdk.net).

> Note: make sure that the JDK/JRE you choose contains JavaFX! You can always install it manually otherwise

After installing it, double-clicking the JAR should be enough to start it.

Otherwise, run ```java -jar uScreen.jar``` in the command prompt.

For the USB to get recognized, follow the following steps:

- Download [Zadig](https://zadig.akeo.ie)

- Boot your console with uLaunch (first ensure USB screen capture is enabled in settings), connect it to the PC via USB

- With Zadig, select the device named "Nintendo Switch"

- Install **libusbK** to that device (any other driver won't work fine)

#### Linux

Install OpenJDK 11 (or higher) in the terminal:

- Run ```sudo add-apt-repository ppa:openjdk-r/ppa```

- Run ```sudo apt-get update```

- Finally, run ```sudo apt-get install openjdk-11-jdk``` (if you just want the JRE, install `openjdk-11-jre` instead)

- Create the file ```/etc/udev/rules.d/99-switch.rules``` with the following contents: ```SUBSYSTEM=="usb", ATTRS{idVendor}=="057e", ATTRS{idProduct}=="3000", GROUP="plugdev"```

- Reload udev rules with: ```sudo udevadm control --reload-rules && sudo udevadm trigger```

Now you can run uScreen using ```java -jar uScreen.jar```.

#### Mac

Install OpenJDK 11 (or higher) in the terminal:

- Install brew ```/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install.sh)"```

- Run ```brew tap AdoptOpenJDK/openjdk```

- Run ```brew install adoptopenjdk11 --cask```

- Finally, run ```java -version``` to check the JDK version

Now you can run uScreen using ```java -jar uScreen.jar```.

Having done all this, the USB connection should work fine.

### uDesigner

uDesigner is an (experimental) [web theme editor](https://github.com/XorTroll/uLaunch/wiki) to make themeing more easier.

## Building

> Note that [workflows](https://github.com/XorTroll/uLaunch/actions) automatically build and upload nightly builds for every commit!

This project is, like [Goldleaf](https://github.com/XorTroll/Goldleaf), based on my [Plutonium](https://github.com/XorTroll/Plutonium) UI libraries.

You will need *devkitPro*, *devkitA64*, *libnx* and all SDL2 libraries for Switch development (make sure their packages are installed): `switch-sdl2 switch-freetype switch-glad switch-libdrm_nouveau switch-sdl2_gfx switch-sdl2_image switch-sdl2_ttf switch-sdl2_mixer`

> IMPORTANT: depending on the SDL2 package version (revision) used, audio issues will cause crashes when suspending certain games. The version (revision) [`2.28.5-3`](https://github.com/devkitPro/pacman-packages/tree/0ae8790f6e092cf8df937d143e70a785f7e27997/switch/SDL2) is an outdated version which internally uses `audout` services instead of `audren` (used by more recent SDL2 cersions), and it should be used to build uLaunch (the only difference between the current revision and this outdated one is the audio service used, so its not really old). Using modern versions with `audren` will [make the audio sysmodule crash](https://github.com/HookedBehemoth/sys-tune/issues/10) due to an issue with the service, when suspending specific games.

Clone **recursively** this repository and just enter `make` in the command line. It should build everything and generate a `SdOut` folder whose contents sould directly be copied to the root of a console SD card.

In order to only build a certain subproject, you can run `make` plus the subproject's name: `make usystem`, `make uloader`, `make umenu`, `make umanager`, `make uscreen`, `make udesigner`

## Credits

- SciresM for [Atmosphere-libs](https://github.com/Atmosphere-NX/Atmosphere-libs).

- Switchbrew team for [libnx](https://github.com/switchbrew/libnx) and [nx-hbloader](https://github.com/switchbrew/nx-hbloader), the base of uLoader, as well as their wonderful [wiki](https://switchbrew.org/wiki/Main_Page).

- C4Phoenix for the original design of this project's logo.

- [Iconos8](https://iconos8.es/), [WallpaperAccess](https://wallpaperaccess.com/), [Flaticon](https://www.flaticon.com/), [Freepik](https://www.freepik.com/) and [Icon Archive](https://www.iconarchive.com/) as the bases for most of the icons used by the default menu theme.

- Several scene developers for their help with small issues or features.

- uMenu/uManager translations: [DDinghoya](https://github.com/DDinghoya) for Korean, [NedcloarBR](https://github.com/NedcloarBR) for Brazilian Portuguese, [Gabriele73](https://github.com/Gabriele73) for Italian, **mokkori** for French, **KineticPie** for German

- Everyone from my Discord and other places whose suggestions made this project a little bit better! Specially all the testers for being essential in reporting bugs and helping a lot with the project's development <3
