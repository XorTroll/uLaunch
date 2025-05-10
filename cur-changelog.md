# Changelog for `v1.2.0`

## General

Before anything, as always: new language texts were added to reflect the changes below, and other minor internal fixes were made (which probably have very little impact on the user experience)

- Compiled with latest libnx changes and Atmosphère 1.9.0, supporting up to firmware 20.0.1

- Fixed issues where some theme files were not properly cache'd and thus would not load

- The default string (if all language files failed to load) is now `<unknown>` instead of an empty string.

- Invalid, previously selected homebrew takeover applications are reset (if you selected an application as homebrew takeover and then deleted the application, uLaunch would previously crash trying to launch homebrew over it,but now such invalid setting is automatically cleaned)

- The config is now unique per emuMMC or sysMMC!

  - This means that emu/sysmmC will have independent configs (yeah, that means different themes!)

  - uLaunch will automatically convert the global settings so far into sys/emu settings: they will be the same, copied from the global one, but from now on any change will only be made on the current MMC (sys/emu)!

- A minor homebrew entry cache bug was fixed, now if a homebrew is updated while uLaunch is active (homebrew that auto-updates, sending via hbmenu's NetLoader) it will be cache'd accordingly (prior to this, cache loading would fail and it would show as an invalid "???" entry)

## uMenu

- When pressing B on a dialog, the cancel button is briefly focused before fading out, to show more clearly that the dialog was cancelled

- The issue where uMenu opening applets (the keyboard, user page and so on) would show previous applet graphics in the background is finally fixed!

- Added support for button remapping and controller updating! These are now additional options in the "Controllers" special menu entry.

- Suspending applets has now a similar fade-out effect as suspending applications, which looks really nice IMO ;)

- Similar fixes were done when launching applets over uMenu (keyboard, controller) where previously a dark background was shown, now (as with regular HOME Menu) the uMenu screen is shown blurred in the background (little details that make everything nicer!)

- When launching applets, a fade-out effect is done with the underlying theme color (light or dark) to make a nicer transition from out colorful themes to the minimalistic dark/light style that applets use regardless ;)

- Added additional logging for future troubleshooting of load times

- Minor fix when showing the keyboard: now the cursor is properly initialized at the end (if there was some initial text, like when changing the console nickname)

## Main menu

- Menus are lazy-loaded now (only loaded when you get into it), which might improve performance (marginally?)

- Starting uLaunch doesn't show the lockscreen (if enabled), it goes directly to the startup menu as usual

- Some UI texts (title and author/version texts in main menu) no longer extend beyond their limits if too long (a new simple animation system is used display them)

- The long-standing audio-sysmodule crashes when suspending specific games is finally fixed! For technical details, check the repo README regarding building.

- Time and date are only re-rendered when the values actually change (again, marginal performance improvements?)

- Games that need an update are properly treated now (sometimes they can be launched and sometimes not, both cases are distinguished now)

- A simple but usual animation is present in the time text, where the dots ":" between minutes and seconds flicker occasionally (also in lockscreen menu)

- Special homebrew entries (hbmenu and uManager as of now) cannot be added again, since they are made to be always present in the menu (that is why they're special :P)

- A dedicated error message is shown when a theme fails to load (and the default theme was loaded because of that), or when a theme is outdated (but still was loaded)

- Fixed SFX being played constantly when spamming L/R for moving pages in the menu (now SFX only plays when it moves)

- Fixed annoying graphical behavior when spamming +/- for resizing the menu

- If selecting a game that is the homebrew takeover, the option to set as homebrew takeover is no longer shown (because it was pointless :P)

- Now pressing the HOME button inside a folder first rewinds it to the start, then a second press gets it back to the HOME menu root (previously it directly went back to the root menu)

- Menu layouts are now unique per user and sys/emuMMC! uLaunch will automatically copy the current (global) menu to all users, and from then, any menu modifications will be unique for the user ;)

- Fixed a bug where renaming a folder would not update the folder name text over the folder icon

- Game icons are no longer cache'd by uLaunch itself: the console already has its own cache system, and simply relying on it (like regular HOME Menu does) has always been faster than loading our cache'd icons from the SD card (just that it took me a few years to really notice); another consequence is that, when changing language in the console, icons will properly reflect the corresponding language (which previously did not always work as expected)

- Account/user icons are also no longer cache'd (for more or less the same reason as with games) hence when creating a new user / editing a user's icon, the change is properly reflected in the menu ;)

## Settings menu

- The setting that checks for system updates no longer freezes when its waiting for connection (a time-out prevents it now).

- The movements between setting menus were rewritten, now doing a much more smoother move ;) (you can also move with ZL/ZR and left/right D-pad as well)

- New uLaunch settings were added:

  - uLaunch version

  - Audio service used by uLaunch

  - Reset application used for homebrew takeover (there was no way to do this previously)

  - Launch homebrew as application by default (disabled by default, only takes effect if a takeover application is selected)

- Two new submenus are introduced... for Bluetooth audio device support! One submenu for (dis)connecting/unpairing already paired devices, and another one for searching for new devices

## Themes

- New theme version number: `3`

  - uLaunch is designed to try to make themes as backwards-compatible as possible, so previous themes should work fine regardless

- The extension for themes is now `.ultheme` instead of a plain `.zip` (it is still a ZIP file in contents, though)

  - uLaunch itself will rename valid `.zip` themes in the themes directory automatically :)

  - uDesigner will support loading `.zip` and `.ultheme` themes, but will save them as `.ultheme` from now on

- EntryIcons and OverIcons should be provided on the basis that menu icons are `384x384` size in the worst case (previously it was recommended to consider `256x256`).

- Text entries can be configured to have a maximum width, and animate their display otherwise (see UI text correction above):

  - Using `clamp_width`, `clamp_speed` and `clamp_delay` options

- New settings menu SFX was added: `SettingMenuMove.wav`

- New main menu SFX was added: `MenuIncrement.wav` and `MenuDecrement.wav`

- Now, a version of the default theme with DSi/Wii/3DS BGM/SFX will be included in the release! You can now download it and experience the default theme in a more immersive way ;)

### Default theme

- The main menu top part was reorganized to show game titles in a more convenient way

  - Icons were moved to the right part

  - The background colors (unique for app, folder, homebrew, etc) were changed to more visually appealing ones

- Updated EntryIcons, OverIcons and QuickIcons to reflect the maximum `384x384` size in icons. In some of them, the base icon design / colors were changed to more fitting ones ;)

> For previous versions, please visit their respective release pages
