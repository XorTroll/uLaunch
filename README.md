<img width="200" src="LibraryAppletQMenu/RomFs/LogoLarge.png">

> Custom, open-source replacement/reimplementation for Nintendo Switch's HOME menu (qlaunch), extending it with amazing, homebrew-orienteed functionality!

<img src="Screenshots/s1.png" alt="drawing" width="400"/> <img src="Screenshots/s2.png" alt="drawing" width="400"/>

<img src="Screenshots/s3.png" alt="drawing" width="400"/> <img src="Screenshots/s4.png" alt="drawing" width="400"/>

<img src="Screenshots/s5.png" alt="drawing" width="400"/> <img src="Screenshots/s6.png" alt="drawing" width="400"/>

uLaunch is a very ambitious project, consisting on two custom library applets, a custom system application and a custom system applet, in order to replace the console's **HOME menu** with a custom, homebrew-orienteed one.

- This isn't any kind of HOME menu extension, injection, patch, etc. uLaunch is a **complete** reimplementation, 100% open-source, which also takes over eShop and Parental control applets and flog system title (all of them are pretty much useless with this reimpl) for its extended functionality.

- The project is licensed as **GPLv2**.

- For those who are interested in how the UI was done, this project is, like [Goldleaf](https://github.com/XorTroll/Goldleaf), a good example of how powerful [Plutonium libraries](https://github.com/XorTroll/Plutonium) can be in order to make beautiful UIs.

## Are you looking for help with themes? Check [this documentation](Themes.md) for everything you need!

## Having trouble with uLaunch? Check the [FAQ](#faq) section for support!

### **Table of contents**

1. [Features](#features)
2. [Disclaimer](#disclaimer)
3. [FAQ](#faq)
4. [Project and subprojects](#project-and-subprojects)
5. [Custom menu entries](#custom-menu-entries)
6. [Translations](#translations)
7. [Installing and removing](#installing-and-removing)
8. [Compiling](#compiling)
9. [Errors](#errors)
10. [Credits](#credits)

## Features

**List of HOME menu features uLaunch has**:

- Proper launching and foreground management: launch, suspend and close titles and applets

- Proper general channel handling (some of it might be not implemented): sleep, shutdown, reboot, HOME menu press detection... 

- Settings:

  - Show connected WiFi network's name
  
  - Open connection applet in case user wants to change network settings

  - Change console's language and show active one

  - Change console's nickname and show current one

- User features:

  - Allow creating user on the startup menu

  - Show user's page (in order to edit nickname, icon, friends...)

**List of not (yet) implemented HOME menu features**:

- Controller managing

- Album

- ~~Periodical play report sending (so long, telemetry!)~~

This is the amount of features uLaunch contains, compared to the original HOME menu:

- *Homebrew support*

  - Launching as applets (no need of **Album**!)

  - Launching as **applications** (no need of **any titles** to do so!)

  - Custom basic homebrew menu

  - Option to add custom NRO accessors to main menu (homebrew or custom items easily accessible, no more need of **forwarders**!)

- *UI*

  - **Themes** (different to official HOME menu themes/NXThemes)

    - Custom icons, menu assets and graphics (custom images, colors, sizes, positions...), background...

    - Custom **background music** and **sound effects**!

  - **Folders** in order to keep your main menu organized!

- *Users*

  - PC-like login on startup (select user and use it for everything, log off, register/change/remove password...)

  - **User password** support! (up to 15 characters)

- *Miscellaneous extras*

  - Web browsing (via web-applet) directly from the main menu!

  - **Foreground capturing** from PC itself (*Windows*-only) via USB-C cable and *QForegroundViewer*!

## Disclaimer

### Homebrew-as-application 'flog' takeover

uLaunch allows you to launch homebrew as an application, taking advantage of the system's '**flog**' built-in application title, which was stubbed but not removed, thus it's content can be overriden via LayeredFS and launched.

Since launching this title should be impossible, it might involve ban risk. uLaunch has this option **disabled by default**, so enable and use it **use it at your own risk**. Always make youre you're safe from bans (by using tools like 90DNS) before using uLaunch to avoid any possible risks.

## FAQ

- *How do I uninstall installed titles? How do I delete user accounts?*

  - uLaunch doesn't (and won't) support these, since there is already existing homebrew serving as title or user managers, like [Goldleaf](https://github.com/XorTroll/Goldleaf).

- *How can I change my controllers, access eShop or view the album?*

  - These are all TODOs for uLaunch (not yet supported), except eShop, which for what it's worth, won't be supported. If you want to use eShop, remove uLaunch and use the original HOME menu.

> TODO: add more FAQs here

## Project and subprojects

uLaunch is split, as mentioned above, into several sub-projects:

### QDaemon (SystemAppletQDaemon)

> This sub-project replaces qlaunch, aka title 0100000000001000.

This is the technically actual qlaunch reimplementation. In fact, it is used as a back-end for the project, which just does what the actual menu (QMenu) tells it to do. This had to be like that, since qlaunch is the only one who can access certain essential functionality like title launching, so QMenu needs to access it, so it communicates with this back-end to execute it and to obtain its outcome.

### QMenu (LibraryAppletQMenu)

> This sub-project uses eShop's library applet as a donor title, aka 010000000000100B.

This is the "actual" HOME menu the user will see and interact with. It contains all the UI and sounc functionality, password, themes...

### QHbTarget

This is the name for two related projects, whose aim is to target and launch homebrew NROs in a simple way. It can be considered as a simple and helpful wrapper around [nx-hbloader](https://github.com/switchbrew/nx-hbloader)'s code:

#### System application (SystemApplicationQHbTarget)

> This sub-project replaces "flog" system application, aka title 01008BB00013C000.

This is the process which runs instead of flog, which is used to launch homebrew as applications.

#### Library applet (LibraryAppletQHbTarget)

> This sub-project replaces "dataErase" library applet, aka title 0100000000001001.

This is the same process but, like in normal HOME menu and Album, it runs homebrew as an applet. However, exiting homebrew here will exit to HOME menu instead of exiting to hbmenu.

### QForegroundViewer

*QDaemon* backend process, appart from handling HOME menu functionality, also sends the console's foreground display via USB (if enabled on settings) as a raw RGBA8888 1280x720 ~3.5MB buffer. **QForegroundViewer** is the PC tool (Windows Forms) which intercepts those sent buffers and renders them. It is a bit laggy, but can be *very, very* useful for taking quick screenshots!

This option comes disabled by default because homebrew using USB (like Goldleaf or nxmtp) would interfer with the connection. If you really want to use it, enable it from settings.

This tool was used to take the screenshots shown above, all in full 1280x720 quality.

Unlike the other projects, this one isn't essential and uLaunch would be perfectly usable ignoring this sent data.

For more technical information about uLaunch and qlaunch, check [this](HOME.md).

## Custom menu entries

uLaunch supports adding custom shortcuts. This means you can show entries with a custom icon, which launch a NRO with certain argv, what could be potentially used for stuff like emulator ROM shorcuts.

Entries consist on JSON files (one per entry) located at `sd:/ulaunch/entries` (name doesn't matter, but the extension must be `*.json`!). They are used for two main purposes:

- Adding homebrew accesses (as stated above) to the main menu

- Specifying that a homebrew access or a normal, installed game/application go inside a folder

If you want to make your own access, create a JSON file in the entries folder (the name doesn't matter) like this:

```json
{
  "type": 2,
  "nro_path": "<path-to-nro>",
  "nro_argv": "<custom-argv>",
  "icon": "<custom-icon-256x256>",

  "name": "Name (511 bytes max.)",
  "author": "Author (255 bytex max.)",
  "version": "Version (15 bytes max.)"
}
```

Note that:

- The type needs to be **always 2** (1 = installed title, other values are invalid), and that `type` and `nro_path` **must be always present**. The other parameters are all optional.

- If `name`, `author` or `version` fields aren't set they will be loaded from the NRO, thus it would show the RetroArch core's name, etc., same with the icon.

- The argv string must not include the NRO path. Usual homebrew behaviour would be `<nro-path> <arg-1> <arg-2> <...>`, but uLaunch itself appends the NRO path before the specified argv when launching the menu entry, so you should just provide `<arg-1> <arg-2> <...>`

## Translations

uLaunch is in English by default. However, you can make translations by editing the [LangDefault.json](https://github.com/XorTroll/uLaunch/blob/master/LibraryAppletQMenu/RomFs/LangDefault.json) translation file's strings and placing it into `sd:/ulaunch/langs` named as the language's code ("en-US" for American English, "es" for Spanish, "fr" for French... proper list [here](https://switchbrew.org/wiki/Settings_services#LanguageCode)

Example: `sd:/ulaunch/lang/es.json` for custom strings (which would be displayed if  the console's language is Spanish)

## Installing and removing

In order to check uLaunch's installation, you will need to pay attention to the `titles` directory of the CFW you're using.

### How do I know whether it is installed?

Check if folders `0100000000001000`, `010000000000100B`, `0100000000001001` and `01008BB00013C000` exist. (and that they aren't empty, or at least contain a `exefs.nsp` file)

### How do I remove it?

1 - Delete the following folders: `010000000000100B`, `0100000000001001` and `01008BB00013C000`.

2 - Delete **only the `exefs.nsp` file** from `0100000000001000` directory (if there is a `romfs` folder present it could be a normal HOME menu theme)

## Compiling

You will need devkitPro, devkitA64, libnx and all SDL2 libraries for switch development.

Clone (**recursively!**) this repo (uses libstratosphere and Plutonium submodules) and hit `make` in root. It should build everything and generate a `titles` folder ready for use inside `SdOut`.

Using `make dev` instead of regular `make` will compile uLaunch in debug mode, which makes the backend process display a debug console and a special menu before usual boot to perform special tasks. This is only meant to be used by developers.

## Errors

If you get a crash using uLaunch, please check:

- If the crash's title is `0100000000001000`, `010000000000100B`, `0100000000001001` or `01008BB00013C000` and **you know for sure you have this project in your SD**, then it is very likely related to this project.

- Note that `0100000000001001` and `01008BB00013C000` are the IDs of the processes used by uLaunch to target homebrew, so if you get a crash from these using uLaunch it probably means a crash from the homebrew you were using.

## Credits

- Several scene developers for help with small issues or features.

- SciresM for [libstratosphere](https://github.com/Atmosphere-NX/libstratosphere).

- Switchbrew team for libnx and [hbloader](https://github.com/switchbrew/nx-hbloader), the base of *QHbTarget projects (they're some useful wrappers of hbloader in the end)

- C4Phoenix for the amazing design of this project's logo.

- [Icons8](https://icons8.com/) website for a big part of the icons used by the default style.

- Everyone from Discord or other places whose suggestions made this project a little bit better :)