<img width="200" src="LibraryAppletQMenu/RomFs/LogoLarge.png">

> Custom, open-source replacement/reimplementation for Nintendo Switch's HOME Menu (qlaunch), extending it with amazing, homebrew-orienteed functionality!

<img src="Screenshots/s1.png" alt="drawing" width="400"/> <img src="Screenshots/s2.png" alt="drawing" width="400"/>

<img src="Screenshots/s2.png" alt="drawing" width="400"/> <img src="Screenshots/s4.png" alt="drawing" width="400"/>

<img src="Screenshots/s5.png" alt="drawing" width="400"/> <img src="Screenshots/s6.png" alt="drawing" width="400"/>

uLaunch is a very ambicious project, consisting on two custom library applets, a custom system application and a custom system applet, in order to replace the console's **HOME menu** with a custom, homebrew-orienteed one.

No, this isn't any kind of HOME menu extension, injection, patch, etc. uLaunch is a **complete** reimplementation, 100% open-source, which also takes over eShop and Mii applets and flog system title, for its extended functionality.

## **NOTE:** the project is still a work-in-progress. Check what's left to do before release [here](TODO.md)!

- The project is licensed as **GPLv2**.

- For those who are interested in how the UI was done, this project is, like [Goldleaf](https://github.com/XorTroll/Goldleaf), a good example of how powerful [Plutonium libraries](https://github.com/XorTroll/Plutonium) can be in order to make beautiful UIs.

## Are you looking for help with themes? Check [this document](Themes.md) for more info!

1. [Features](#features)
2. [Disclaimer](#disclaimer)
3. [Project and subprojects](#project-and-subprojects)
4. [Custom menu entries](#custom-menu-entries)
5. [Translations](#translations)
6. [Installing and removing](#installing-and-removing)
7. [Compiling](#compiling)
8. [Errors](#errors)
9. [Credits](#credits)

## Features

This is the amount of features uLaunch contains, compared to the original HOME menu:

- Homebrew support

  - Launching as applets (no need of **Album**!)

  - Launching as **applications** (no need of **any titles** to do so!)

  - Custom basic homebrew menu

  - Option to add custom NRO accessors to main menu (homebrew or custom items easily accessible, no more need of **forwarders**!)

- Miscellaneous extras

  - Web browsing (via web-applet) directly from the main menu!

  - **Foreground capturing** from PC itself (*Windows*-only) via USB-C cable and *QForegroundViewer*!

- UI

  - **Themes** (different to official HOME menu themes/NXThemes)

    - Custom icons, menu gfx, background...

    - Custom **background music** and **sound effects**!

  - **Folders** in order to keep your main menu organized!

  - Option to show the suspended title in the **background** (like 3DS's HOME menu did!)

- Users

  - PC-like login on startup (select user and use it for everything, log off, register/change/remove password...)

  - **User password** support! (up to 15 characters)

## Disclaimer

### Homebrew-as-application 'flog' takeover

uLaunch launches homebrew present (added) to main menu as an application, taking advantage of the system's '**flog**' built-in application title, which was stubbed but not removed, thus it's content can be overriden via LayeredFS and launched.

Since launching this title should be impossible, it might involve ban risk. uLaunch has this option **disabled by default**, so enable and use it **use it at your own risk**.

## Project and subprojects

uLaunch is split, as mentioned above, into several sub-projects:

### QDaemon (SystemAppletQDaemon)

> This sub-project replaces qlaunch, aka title 0100000000001000.

This is the technically actual qlaunch reimplementation. In fact, it is used as a back-end for the project, which just does what the actual menu (QMenu) tells it to do. This had to be like that, since qlaunch is the only one who can access certain essential functionality like title launching, so QMenu needs to access it, so it communicates with this back-end to execute it and to obtain its outcome.

### QMenu (LibraryAppletQMenu)

> This sub-project replaces "cabinet" library applet, aka title 010000000000100B.

This is the HOME menu the user will see and interact with. It contains all the UI and sounc functionality, password, themes...

### QHbTarget

This is the name for two related projects, whose aim is to target and launch homebrew NROs in a simple way. It can be considered as a simple and helpful wrapper around [nx-hbloader](https://github.com/switchbrew/nx-hbloader)'s code:

#### System application (SystemApplicationQHbTarget)

> This sub-project replaces "flog" system application, aka title 01008BB00013C000.

This is the process which runs instead of flog, which is used to launch homebrew as applications.

#### Library applet (LibraryAppletQHbTarget)

> This sub-project replaces "dataErase" library applet, aka title 0100000000001001.

This is the same process but, like in normal HOME menu and Album, it runs homebrew as an applet. However, exiting homebrew here will exit to HOME menu instead of exiting to hbmenu.

### QForegroundViewer

*QDaemon* daemon process, appart from usual (and special) HOME menu functions, it also sends the console's foreground display via USB as a raw RGBA8 1280x720 ~3MB buffer. **QForegroundViewer** is the PC tool (WinForms) which intercepts those sent buffers and renders them. It is a bit laggy, but can be *very* useful for taking quick screenshots!

Unlike the other projects, this one isn't essential and uLaunch would be perfectly usable ignoring this sent data.

For more technical information about uLaunch and qlaunch, check [this](HOME.md).

## Custom menu entries

uLaunch supports adding custom shortcuts. This means you can show entries with a custom icon, which launch a NRO with certain argv, what could be potentially used for stuff like emulator ROM shorcuts.

Entries consist on JSON files (one per entry) located at `sd:/ulaunch/entries`. They are used for two main purposes:

- Adding homebrew accesses (as stated above) to the main menu

- Specifying that a homebrew access or a system title go inside a folder

If you want to make your own access, create a JSON file in the entries folder (the name doesn't matter) like this:

```json
{
  "type": 2,
  "nro_path": "<path-to-nro>",
  "nro_argv": "<custom-argv>",
  "icon": "<custom-icon-jpeg>",

  "name": "My custom entry",
  "author": "Second text for whatever I want",
  "version": "Third text"
}
```

Note that type needs to be **always 2**, and that `type`, `nro_path` **must be present**. The others are all optional.

Note that if `name`, `author` or `version` fields aren't set they will be loaded from the NRO, thus it would show the RetroArch core's name, etc., same with the icon.

## Translations

uLaunch is in English by default. However, you can make translations by editing the [LangDefault.json](https://github.com/XorTroll/uLaunch/blob/master/LibraryAppletQMenu/RomFs/LangDefault.json) translation file's strings and placing it into `sd:/ulaunch/langs` named as the language's code ("en-US" for American English, "es" for Spanish, "fr" for French... proper list [here](https://switchbrew.org/wiki/Settings_services#LanguageCode) - example: `sd:/ulaunch/lang/es.json` for custom strings when the console's language is Spanish)

## Installing and removing

In order to check uLaunch's installation, you will need to pay attention to the `titles` directory of the CFW you're using.

### How do I know whether it is installed?

Check if folders `0100000000001000`, `010000000000100B`, `0100000000001001` and `01008BB00013C000` exist. (and that they aren't empty, or at least contain a `exefs.nsp` file)

### How do I remove it?

1 - Delete the following folders: `010000000000100B`, `0100000000001001` and `01008BB00013C000`.

2 - Delete **only `exefs.nsp` file** from `0100000000001000` directory (if there is a `romfs` folder present it could be a normal HOME menu theme)

## Compiling

You will need devkitPro, devkitA64, libnx and all SDL2 libraries for switch development.

Clone (**recursively!**) this repo (uses libstratosphere and Plutonium submodules) and hit `make` in root. It should build everything and generate a `titles` folder ready for use inside `SdOut`.

## Errors

If you get a crash using uLaunch, please check:

- If the crash's title is `0100000000001000`, `010000000000100B`, `0100000000001001` or `01008BB00013C000` and **you know for sure you have this project in your SD**, then it is very likely related to this project.

## Credits

- Several scene developers for help with small issues or features.

- SciresM for [libstratosphere](https://github.com/Atmosphere-NX/libstratosphere).

- Switchbrew team for libnx and [hbloader](https://github.com/switchbrew/nx-hbloader), the base of *QHbTarget projects (they're some useful wrappers of hbloader in the end)

- C4Phoenix for the amazing design of this project's logo.

- [Icons8](https://icons8.com/) website for a big part of the icons used by the default style.