# Changelog

## uLaunch

- Updated with last libnx and Atmosphere.

- Now, instead of always overriding certain applets (if you had uLaunch on your SD), uLaunch makes use of ECS to launch its processes over a certain applet, so that the applet can be used normally when the process isn't launched.

- The internal comunication system between uLaunch's processes has changed internally and made more fast and efficient.

## USB support

- USB support is back (it was temporarily removed in 0.2.1 due to weird technical issues)

- USB now supports an alternative system, which is available under certain circumstances (having patches enabling it being on >10.0.0)

## uMenu

- When launching homebrew as applications, uMenu won't make use of the internal flog system application it used to use (which might have been risky for potential bans). Instead, making use of ECS (mentioned above), after a donor title has been selected by pressing up, homebrew can easily be launched over that application like normal Atmosphere does. Note that you won't be able to launch homebrew over an application unless you select that application as the donor application.

- HOME menu pressing is properly supported on the different menus. For instance, pressing the HOME button inside the settings menu will make it return to the main menu.