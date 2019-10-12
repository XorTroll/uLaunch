# (Unnamed) qlaunch reimplementation

> Custom, open-source replacement for Nintendo Switch's HOME Menu (qlaunch)

{qlaunch-reimpl} is a very ambicious project, consisting on two custom library applets, a custom system application and a custom system applet, in order to replace the console's **HOME menu** with a custom, homebrew-orienteed one.

- Note: *{qlaunch-reimpl}* will be replaced with the actual project's name when it is decided.

1. [Features](#features)
2. [Disclaimer](#disclaimer)
3. [How does it work?](#how-does-it-work?)
4. [Installing and removing](#installing-and-removing)
5. [Compiling](#compiling)

## Features

This is the amount of features {qlaunch-reimpl} has compared to official qlaunch:

- Homebrew support

  - Launching as applets (no need of **Album**!)

  - Launching as **applications** (no need of **any titles** to do so!)

  - Custom basic homebrew menu

  - Option to add homebrew accessors to main menu (no more need of **forwarders**!)

- UI

  - Themeable (not the same way official qlaunch themes work)

    - Custom icons, background...

    - Custom **background music** and **sound effects**!

  - Option to show the suspended title in the **background** (like 3DS's HOME menu did!)

- Users

  - PC-like login on startup (select user and use it for everything)

  - **Password** support! (up to 15 characters)

> TODO: add more later...?

## Disclaimer

### Homebrew as applications

{qlaunch-reimpl} launches homebrew present (added) to main menu as an application, taking advantage of **flog**'s built-in application title, which was stubbed but not removed, thus it's content can be overriden via LayeredFS and launched.

Since launching this title should be impossible, it might involve ban risk, so **use it at your own risk**. {qlaunch-reimpl} **warns on launching titles this way**.

## How does it work?

{qlaunch-reimpl} is split, as mentioned above, into several sub-projects:

### QDaemon (SystemAppletQDaemon)

This is the technically actual qlaunch reimplementation. However, to avoid memory issues it does not use any kind of UI (except console for development debug, which is removed for releases), and thus it uses 16MB of heap, while official HOME menu uses 56MB.

Instead, it uses [QMenu custom library applet](#QMenu%20(LibraryAppletQMenu)) (launches and communicates with it) in order to display a proper menu UI.

But, if all the functionality is handled by QMenu, why is this daemon process necessary? Well, mainly since the system applet process has a lot of unique code it's the only one who can use, specially, related to title launching, focus managing...

**TL;DR**: this is a simple daemon process which stays communicated with QMenu to perform tasks it is asked to so, specially with code it is the only one that can access.

### QMenu (LibraryAppletQMenu)

This is the HOME menu the user will see and interact with. It contains all the UI and sounc functionality, password, themes...

### QHbTarget

This is the name for two related projects, whose aim is to target and launch homebrew NROs in a simple way. It can be considered as a simple and helpful wrapper around [nx-hbloader](https://github.com/switchbrew/nx-hbloader)'s code:

#### System application (SystemApplicationQHbTarget)

This is the process which runs instead of flog, which is used to launch homebrew as applications.

#### Library applet (LibraryAppletQHbTarget)

This is the same process but, like in normal HOME menu and Album, it runs homebrew as an applet. However, exiting homebrew here will exit to HOME menu instead of exiting to hbmenu.

## Installing and removing

In order to check {qlaunch-reimpl}'s installation, you will need to pay attention to the `titles` directory of the CFW you're using.

### How do I know whether it is installed?

Check if folders `0100000000001000`, `010000000000100B`, `0100000000001009` and `01008BB00013C000` exist. (and that they aren't empty, or at least contain a `exefs.nsp` file)

### How do I remove it?

1 - Delete the following folders: `010000000000100B`, `0100000000001009` and `01008BB00013C000`.

2 - Delete **only `exefs.nsp` file** from `0100000000001000` directory (if there is a `romfs` folder present it could be a normal HOME menu theme)

## Compiling

You will need devkitPro, devkitA64, libnx and all SDL2 libraries for switch development.

Clone or download this repo and hit `make` in root. It should build everything and generate a `titles` folder ready for use inside `SdOut`.

## Credits

- Several scene developers for help with small issues or features.

- SciresM for [libstratosphere](https://github.com/Atmosphere-NX/libstratosphere).

- Switchbrew team for libnx and the building toolchain.