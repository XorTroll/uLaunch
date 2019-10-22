# Theme system

## **IMPORTANT!** Don't use this system yet. Wait until this warning is removed (when the system is finally decided)

> Current theme format version: none (unreleased yet)

Themes consist on plain directories replacing RomFs content.

- Themes are place in `sd:/ulaunch/themes` directory.

- A valid theme must, at least, contain a `/theme` subdirectory and a `/theme/Manifest.json` within it (icon or actual modifications aren't neccessary)

Thus, a theme named `DemoTheme` should have its manifest JSON in `sd:/ulaunch/themes/DemoTheme/theme/Manifest.json`. Other assets (like UI) would go inside `sd:/ulaunch/themes/DemoTheme/ui`, `/sound`...

**Important note:** any content (sound or UI) not found in the theme will be loaded from the default config, thus there is no need to provide every file (for instance, when making UI-only or sound-only changes)

## Metadata

Metadata is stored inside `/theme` directory. It is required for the theme to be recognized as a valid one.

- `theme/Manifest.json` -> JSON containing theme information

   Demo JSON:

    ```json
    {
        "name": "My awesome theme",
        "format_version": 0,
        "release": "0.1",
        "description": "This is a really cool theme, check it out!",
        "author": "XorTroll"
    }
    ```

    Properties:

    - **name**: Theme name

    - **format_version**: Theme format version (qlaunch updates might introduce changes to themes, thus a new format version would be out).

    - **release**: Theme version string

    - **description**: Theme description

    - **author**: Theme author name(s)

## Sound

Sound consists on custom *background music* and *sound effects* via files inside `/sound`.

- `sound/BGM.mp3` -> MP3 file to replace with custom music

- `sound/BGM.json` -> JSON file with BGM settings

   Demo JSON (with default values):

    ```json
    {
        "loop": true,
        "fade_in_ms": 1500,
        "fade_out_ms": 500
    }
    ```

    Properties:

    - **loop**: Whether to replay the MP3 file again, after it finishes.

    - **fade_in_ms**: Time in milliseconds for the fade-in to be applied when the BGM starts playing. (0 = no fade-in)

    - **fade_out_ms**: Time in milliseconds for the fade-out to be applied when the BGM stops playing. (0 = no fade-out)

    Note: returning to/launching a title/applet and returning back to HOME menu will restart the music.

Sound effects consist on short WAV files.

- `sound/TitleLaunch.wav` -> Sfx played when a title/homebrew is launched.

- `sound/MenuToggle.wav` -> Sfx played when the menu is toggled (from main menu to homebrew mode and the opposite)

## UI

Can be customized via files in `/ui`.

- `ui/UI.json` -> JSON file with UI settings

   Demo JSON (with default values):

    ```json
    {
        "suspended_final_alpha": 80
    }
    ```

    Properties:

    - **suspended_final_alpha**: Final alpha the suspended title's capture will have (0 would be to fully hide it, default is 80)

## General

- `ui/Font.ttf` -> TTF font used for all the UI.

## Main menu

- `ui/AllTitles.png` -> 256x256 PNG for the "all titles" entry in the main menu.

- `ui/Folder.png` -> 256x256 PNG for folders in the main menu.

- `ui/Cursor.png` -> 296x296 PNG for the cursor pointing at items in the main menu.

- `ui/Background.png` -> 1280x720 PNG for the main menu's background.

- `ui/Hbmenu.png` -> 256x256 PNG for the option (in homebrew menu) to directly launch normal hbmenu NRO.

- `ui/Banner{type}.png` -> Different 1280x135 PNG banners for menu item types (BannerFolder, BannerHomebrew or BannerInstalled)

### Top menu

The top menu is the small bar shown at the top of the main menu, whose background and icons are customizable.

All the menu icons ({...}Icon.png) are 50x50 icons, except battery ones (BatteryNormalIcon and BatteryChargeIcon are 30x30)

- `ui/TopMenu.png` -> Background of menu (1220x85 PNG)

- Icon list (50x50): ConnectionIcon, NoConnectionIcon, SettingsIcon, ThemesIcon, USBIcon

> *TODO: more menu icons*