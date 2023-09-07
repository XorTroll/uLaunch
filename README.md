<p align="center">
  <img alt="uLaunch" src="projects/uMenu/romfs/LogoLarge.png">
</p

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

> TODO: screenshots!

uLaunch is a project which aims to replace the console's **HOME menu** with a custom, homebrew-oriented one.

- This isn't some kind of HOME menu extension, injection, patch, etc.: uLaunch is an **entire** reimplementation (of key components), 100% open-source, which also (temporarily) takes over eShop and Parental control applets (by default, since all of them are pretty much useless here) for its extended functionality.

- For those who are interested in how the UI was done, this project is, like [Goldleaf](https://github.com/XorTroll/Goldleaf), based on [Plutonium](https://github.com/XorTroll/Plutonium) UI libraries.

<h3>
Table of contents
</h3>

- [Features](#features)
- [Building](#building)
- [FAQ](#faq)
- [Credits](#credits)

### Want to find **themes** for uLaunch? Check the [r/uLaunchThemes subreddit](https://www.reddit.com/r/uLaunchThemes/)!

### Want to make **themes** yourself? Want to know more about the **technical** side of the project? Check [our wiki](https://github.com/XorTroll/uLaunch/wiki)!

## Features

> TODO: add more I'm probably missing

- Proper launching and foreground management: launch, suspend and close applications and library applets

- General channel handling (basic functionality): sleep, shutdown, reboot, HOME menu press detection... 

- System settings:

  - Show connected WiFi network's name, MAC and IP address
  
  - Open connections menu (connection applet), in case you wish to change network settings

  - Check and change the console's language

  - Check and change the console's nickname

- User features:

  - Create new users on the startup menu

  - Show user's page (in order to edit nickname/icon, browse friends...)

- *Homebrew support*

  - Launch homebrew as applets (no need of the **album**!)

  - Launch homebrew as **applications** (requires selecting a **donor title**!)

  - Add homebrew entries to the main menu (thus making homebrew or even custom entries easily accessible, no more need of **forwarders**!)

- *UI*

  - **Themes** (our own, different to official HOME menu themes, way more vibrant and colorful!)

    - Custom icons, menu assets and graphics: custom backgrounds, images, colors, sizes, positions...

    - Custom **background music** and **sound effects**

  - **Folders** (and **subfolders**) in order to keep your main menu neatly organized

  - Grid, 3DS-like menu layout for easier browsing

- *Users*

  - PC-like login on startup: select a user once, use it for everything afterwards!

- *Miscellaneous extras*

  - Browse the Internet (via the normally hidden web-applet) directly from the main menu!

  - Toggle between uLaunch and the original HOME menu (no permanent removal) and/or easily update uLaunch using our `uManager` homebrew tool!

  - Stream the screen via USB (although at low speeds, about ~9 FPS) via `uScreen`! (can be useful for taking quick screenshots, specially since uLaunch is able to capture more than SysDVR or usual game capture)

## Building

You will need *devkitPro*, *devkitA64*, *libnx* and all SDL2 libraries for Switch development (make sure their packages are installed): `switch-sdl2 switch-freetype switch-glad switch-libdrm_nouveau switch-sdl2_gfx switch-sdl2_image switch-sdl2_ttf switch-sdl2_mixer`

Clone **recursively** this repo and just enter `make` in the command line. It should build everything and generate a `SdOut` folder whose contents sould directly be copied to the root of a console SD card.

In order to only build a certain subproject, you can run `make` plus the subproject's name: `make usystem`, `make uloader`, `make umenu`, `make umanager`

## FAQ

- Why can't I access the usual system settings, while I can access other normal system menus like the album, mii editor, user page, etc.?

  - This is a technical issue. While all these mentioned menus are separate applets (separate programs, independent from the HOME menu itself, which can be launched at will) system settings are actually part of HOME menu itself; therefore, we have to implement manually all of them... which requires its effort, so only a bunch of the available settings (plus a few extras) are currently available here, while the remaining settings are being implemented one by one.

- Will using uLaunch get me banned online?

  - While no bans have been reported related to using uLaunch, replacing the retail HOME menu's functionality is never a completely safe idea, so always use it at your own risk. Keep in mind that uLaunch doesn't perform any telemetry or communications with N's servers whatsoever, thus they might be able to notice you're running something different from the original HOME menu.

## Credits

- SciresM for [Atmosphere-libs](https://github.com/Atmosphere-NX/Atmosphere-libs).

- Switchbrew team for [libnx](https://github.com/switchbrew/libnx) and [nx-hbloader](https://github.com/switchbrew/nx-hbloader), the base of `uLoader`.

- C4Phoenix for the amazing design of this project's logo.

- [Icons8](https://icons8.com/) website for a big part of the icons used by the default menu theme.

- Several scene developers for their help with small issues or features.

- Everyone who has halped translating the menu strings to other languages.

- Everyone from Discord or other places whose suggestions made this project a little bit better! Specially all the testers, for reporting bugs and helping a lot with the project's development <3