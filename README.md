<img width="200" src="uMenu/romfs/LogoLarge.png">

> Custom, open-source replacement/reimplementation for Nintendo Switch's HOME menu (qlaunch), extending it with amazing, homebrew-oriented functionality!

<img src="Screenshots/s1.png" alt="drawing" width="400"/> <img src="Screenshots/s2.png" alt="drawing" width="400"/>

<img src="Screenshots/s3.png" alt="drawing" width="400"/> <img src="Screenshots/s4.png" alt="drawing" width="400"/>

<img src="Screenshots/s5.png" alt="drawing" width="400"/> <img src="Screenshots/s6.png" alt="drawing" width="400"/>

uLaunch is a very ambitious project, consisting on two custom library applets, a custom system application and a custom system applet, in order to replace the console's **HOME menu** with a custom, homebrew-oriented one.

- This isn't any kind of HOME menu extension, injection, patch, etc. uLaunch is a **complete** reimplementation, 100% open-source, which also takes over eShop and Parental control applets and flog system title (all of them are pretty much useless with this reimpl) for its extended functionality.

- The project is licensed as **GPLv2**.

- For those who are interested in how the UI was done, this project is, like [Goldleaf](https://github.com/XorTroll/Goldleaf), a good example of how powerful [Plutonium libraries](https://github.com/XorTroll/Plutonium) can be in order to make beautiful UIs.

## Get it from [here](https://github.com/XorTroll/uLaunch/releases/latest)!

### Want to create **custom forwarders** (eg. RetroArch ones)? check **uViewer** tool in [latest releases](https://github.com/XorTroll/uLaunch/releases/latest)!

### Want to find **themes** for uLaunch? Check [r/uLaunchThemes subreddit](https://www.reddit.com/r/uLaunchThemes/)!

### For more detailed **information** about the whole project (*themeing* too), check its [wiki](https://github.com/XorTroll/uLaunch/wiki)!

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

  - **Console screen capturing** from PC itself (*Windows*-only) via USB-C cable and *uViewer* tool!

## Disclaimer

### Homebrew-as-application 'flog' takeover

uLaunch allows you to launch homebrew as an application, taking advantage of the system's '**flog**' built-in application title, which was stubbed but not removed, thus it's content can be overriden via LayeredFS and launched.

Since launching this title should be impossible, it might involve ban risk. uLaunch has this option **disabled by default**, so enable and use it **use it at your own risk**. Always make youre you're safe from bans (by using tools like 90DNS) before using uLaunch to avoid any possible risks.

## Compiling

You will need devkitPro, devkitA64, libnx and all SDL2 libraries for switch development.

Clone (**recursively!**) this repo (uses libstratosphere and Plutonium submodules) and hit `make` in root. It should build everything and generate a `titles` folder ready for use inside `SdOut`.

Using `make dev` instead of regular `make` will compile uLaunch in debug mode, which makes the backend process display a debug console and a special menu before usual boot to perform special tasks. This is only meant to be used by developers.

## Credits

- SciresM for [libstratosphere](https://github.com/Atmosphere-NX/libstratosphere).

- Switchbrew team for libnx and [nx-hbloader](https://github.com/switchbrew/nx-hbloader), the base of *QHbTarget processes (they're just simple wrappers of hbloader in the end)

- C4Phoenix for the amazing design of this project's logo.

- [Icons8](https://icons8.com/) website for a big part of the icons used by the default style.

- Several scene developers for their help with small issues or features.

- Everyone from Discord or other places whose suggestions made this project a little bit better :)
