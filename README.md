<p align="center">
  <img alt="uLaunch" height="100" src="projects/uMenu/romfs/Logo.png">
</p

**uLaunch** is an open-source replacement for the *Nintendo Switch HOME menu* with a custom, homebrew-oriented one:

![Screenshot](screenshot.jpg)

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

### Want to find **themes** for uLaunch? Check the [r/uLaunchThemes subreddit](https://www.reddit.com/r/uLaunchThemes/)!

### Want to make **themes** yourself? Want to know more about the **technical** side of the project? Check [our wiki](https://github.com/XorTroll/uLaunch/wiki)!

<h3>
Table of contents
</h3>

- [Features](#features)
- [Building](#building)
- [Simple FAQ](#simple-faq)
- [Credits](#credits)

## Features

- Basic HOME menu aspects

  - Foreground and background management: launch, suspend and close applications and library applets

  - General channel handling (basic functionality): sleep, shutdown, reboot, HOME menu press detection... 

- *Settings*

  - Various reimplemented system settings (several still need to be implemented)

  - A few uLaunch-specific settings

- *Users*

  - PC-like login: select a user once in the startup menu, use it for everything afterwards!

  - Create new users on the startup menu

  - Show user page (in order to edit nickname/icon, browse friends...)

- *Homebrew support*

  - Launch homebrew as applets (no need of using the *album*)

  - Launch homebrew as *applications* (requires selecting a *donor/takeover application*)

  - Add homebrew entries to the main menu (thus making homebrew or even custom entries easily accessible, no more need of *forwarders*!)

- *UI*

  - Grid-like menu, deeply inspired by the 3DS menu (and partially DSi/Wii menus as well), easier than ever to navigate and customize

  - **Themes** (our own, different to official HOME menu themes, way more vibrant and colorful!)

    - Custom icons, menu assets and graphics: custom backgrounds, images, colors, sizes, positions...

    - Custom *background music** and *sound effects*

  - **Folders** (and *subfolders*) in order to keep your main menu neatly organized

  - Special menu entries, similar to old Nintendo console menus: settings, user page, album, mii editor, controllers, web browser...

- *Miscellaneous extras*

  - Browse the Internet (via the normally hidden web-applet) directly from the main menu!

  - Toggle between uLaunch and the original HOME menu (no permanent removal), easily update uLaunch and more using our `uManager` homebrew tool!

  - Stream the screen via USB (although at low speeds, about ~9 FPS) via `uScreen`! (mostly useful for taking quick screenshots, specially since uLaunch is able to capture more than SysDVR or usual game capture)

- uLaunch is a 100% open-source **entire** reimplementation (of key components): this isn't some kind of HOME menu extension, injection, patch, etc.

## Building

This project is, like [Goldleaf](https://github.com/XorTroll/Goldleaf), based on my [Plutonium](https://github.com/XorTroll/Plutonium) UI libraries.

You will need *devkitPro*, *devkitA64*, *libnx* and all SDL2 libraries for Switch development (make sure their packages are installed): `switch-sdl2 switch-freetype switch-glad switch-libdrm_nouveau switch-sdl2_gfx switch-sdl2_image switch-sdl2_ttf switch-sdl2_mixer`

Clone **recursively** this repo and just enter `make` in the command line. It should build everything and generate a `SdOut` folder whose contents sould directly be copied to the root of a console SD card.

In order to only build a certain subproject, you can run `make` plus the subproject's name: `make usystem`, `make uloader`, `make umenu`, `make umanager`

## Simple FAQ

- Why can't I access the usual system settings, while I can access other normal system menus like the album, mii editor, user page, etc.?

  - This is an unfortunate technical issue. While the web browser, user page, album... are separate applets (separate programs, independent from the HOME menu itself) system settings are *actually* part of HOME menu itself. Therefore, we have to implement manually all of them... which requires its effort, so only a bunch of the available settings (plus a few extras) are currently available here, while the remaining settings are being reversed and implemented.

- Will using uLaunch get me banned online?

  - There have been some cases where using uLaunch may have caused bans. Keep in mind that replacing the official HOME menu's functionality is never a completely safe idea, so always use it at your own risk. Since uLaunch doesn't perform any telemetry or communications with Nintendo servers, they might be able to notice you are running something different from the original HOME menu.

- Why does uLaunch (sometimes) feel slower than the official HOME menu?

  - There are several possible reasons:

    - uLaunch loads more content than the official HOME menu when loading. Most of the official HOME menu's UI are solid colors, while uLaunch loads several images, etc. Being customizable comes with minor drawbacks, like this one.

    - Icons are lazily loaded, so for menus with many entries (essentially for people having a ton of games) navigating through the menu will be slightly laggy until everything loads, which will just take a few seconds. The 3DS menu has similar laggy moments, by the way ;)

    - Aside from the two excuses above, there is always room for further optimizations in uLaunch's code. Feel free to submit any issues of excessive lag/slowdowns, I'll do my best to improve it :)

## Credits

- SciresM for [Atmosphere-libs](https://github.com/Atmosphere-NX/Atmosphere-libs).

- Switchbrew team for [libnx](https://github.com/switchbrew/libnx) and [nx-hbloader](https://github.com/switchbrew/nx-hbloader), the base of `uLoader`.

- C4Phoenix for the original design of this project's logo.

- [Iconos8](https://iconos8.es/), [WallpaperAccess](https://wallpaperaccess.com/), [Flaticon](https://www.flaticon.com/), [Iconfinder](https://www.iconfinder.com/) and [Icon Archive](https://www.iconarchive.com/) as the bases for most of the icons used by the default menu theme.

- Several scene developers for their help with small issues or features.

- `uMenu` translations: [DDinghoya]() for Korean translations

- Everyone from my Discord and other places whose suggestions made this project a little bit better! Specially all the testers for being essential in reporting bugs and helping a lot with the project's development <3
