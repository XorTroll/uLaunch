# Theme system

## **IMPORTANT!** Don't use this system yet. Wait until this warning is removed (when the system is finally decided)

> Current theme format version: none (unreleased yet)

Themes consist on plain directories replacing RomFs content.

A valid theme must, at least, contain a `/theme` subdirectory and a `/theme/Manifest.json` within it (icon or actual modifications aren't neccessary)

**Important note:** any content (sound or UI) not found in the theme will be loaded from the default config, thus there is no need to provide every file (for instance, when making UI-only or sound-only changes)

## Metadata

Metadata is stored inside `/theme` directory. It is required for the theme to be recognized as a valid one.

- `theme/Manifest.json` -> JSON containing theme information

   Demo JSON:

    ```json
    {
        "name": "My awesome theme",
        "version": 0,
        "release": "0.1",
        "description": "This is a really cool theme, check it out!",
        "author": "XorTroll"
    }
    ```

    Properties:

    - **name**: Theme name

    - **version**: Theme format version (qlaunch updates might introduce changes to themes, thus a new format version would be out).

    - **release**: Theme version string

    - **description**: Theme description

    - **author**: Theme author name(s)

- `theme/Icon(.png?)` -> Icon (which size and format shall we use?)

## Sound

Sound consists on custom *background music* and *sound effects* via files inside `/sound`.

- `sound/BGM.mp3` -> MP3 file to replace with custom music

- `sound/BGM.json` -> JSON file with BGM settings

   Demo JSON:

    ```json
    {
        "loop": true,
        "fade_in": true,
        "fade_out": false
    }
    ```

    Properties:

    - **loop**: Whether to replay the MP3 file again, after it finishes

    - **fade_in**: Whether starting the music should apply a fade-in effect.

    - **fade_out**: Whether stopping the music should apply a fade-out effect.

    Note: returning to/launching a title/applet and returning back to HOME menu will restart the music.

> *TODO: Sound effects*

## UI

Can be customized via files in `/ui`.

- `ui/Font.ttf` -> TTF font used for all the UI.

- `ui/AllTitles.png` -> 256x256 PNG for the "all titles" entry in the main menu.

- `ui/Folder.png` -> 256x256 PNG for folders in the main menu.

- `ui/Cursor.png` -> 296x296 PNG for the cursor pointing at items in the main menu.

> *TODO: more customizable stuff*