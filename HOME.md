# qlaunch

qlaunch is the name of the process officially known as **HOME menu**.

## Official

- qlaunch uses ~56MB memory from applet pool.

- qlaunch is a **system applet**, a special and unique type of applet.

  - There are 3 types of applets: **system applet** (qlaunch), **overlay applet** (overlay, another special title), and the rest, which are **library applets** (software keyboard, user selector, web applets...)

  - Development kits or Kiosk units may have different system and overlay titles: for instance, development consoles' overlay is a different one, some can launch DevMenu instead of normal qlaunch, and Kiosk units have special menus instead of qlaunch (known as Retail Interactive Display Menu)

  - Both qlaunch and overlayDisp are special, since they take care of unique and essential functionality, thus they have more access and privileges than any other applet/application.

  - Some of the privileges qlaunch has: title management (is the only one who can directly launch, suspend and close titles), special applet messages (like HOME button press, any other applet or application has these messages but qlaunch has special ones), the general channel (a channel where any applet/application can communicate to qlaunch to request things via storages and "SAMS" messages), foreground control...

- According to RE and left strings, it is qlaunch who handles periodical play report telemetry, aka **is the one who makes play reports and sends them to Nintendo**. Then, **prepo** system service is the one who queues the reports and uploads them when possible.

  - Those play reports contain **a lot** of information about the console, but it is worth to mention that, among them, there is the list of launched titles. If N detects that there is any kind of irregulatity with that list (custom installed titles, for instance), it will likely result in a guaranteed ban.

  - Note that reports are apparently saved even if there is no connection, so that they will be uploaded to N's servers as soon as connection is established. Thus, blocking connection to N's servers (via 90DNS, for example) is, as long as you did ban-baity stuff, a way to delay one's fate. Plain homebrew, aka not touching games or doing suspicious stuff, should not risk bans, but the actual range those have is still unclear.

- qlaunch doesn't just contain the main menu, it does also contain: **news**, **console settings**, **lock screen**... (did I forgot any?)

- Surprisingly, qlaunch's basic functionality (launching titles, detecting HOME press, systems other applets use to interact with it) hasn't (apparently) changed, at least not since 5.1.0 (oldest firmware I did tests related to qlaunch)

## Custom implementation: uLaunch

### Homebrew limitations

Problems arose when starting to work on a proper, serious custom qlaunch reimplementation project, which started by the temporary name **Project Home**, and later got renamed to **eQlipse**.

Homebrew libraries' base GPU libraries (mesa) require **a lot** of memory. Since normal homebrew is expected to run as a library applet or an application, it has never supposed any issues (by default, homebrew libraries tend to reserve as much mem as they can, which tends to be a few hundreds of MBs).

Speaking of a custom type of applet which must use ~50-60MB to not leave the applet pool dry, this supposes a problem.

In fact, this is why the idea of a qlaunch reimplementation was almost abandoned:

- eQlipse used ~180MB, which was the minimum to make the UI work (still sometimes struggling to render properly).

- After updating to latest SDL2 and Plutonium, UI wouldn't even work with more than that (and I was already using more than 3 times the memory I should use!)

- In addition to that, eQlipse's code was really bad organised, and changing a small thing could be really tedious.

### Workaround and re-start

I started to work on a new, well organised and optimized reimplementation before memory issues became unbearable, and this rewrite, after finding a workaround for the memory issue, would become the current project.

The final workaround for the reimplementation was, simply, to **separate the UI from the actual qlaunch functionality** in different processes:

- The actual qlaunch process would be **a daemon process, a backend**, which would make no use of anything UI-related (thus saving **A LOT** of memory, now using even way less than official qlaunch), and which would -**just perform what it is asked to do**, since it is the one with privileges.

- The menu the user would interact with would be a **separate library applet**. Instead of having it always opened, the applet is closed when an application or an applet is launched, and reopened when pressing HOME.

*Result*: using as much memory I want from the applet pool for the UI without being a problem, and the actual qlaunch process wasting ~40MB less memory than usual!

In fact, the fact that the menu gets closed and re-opened very often is helpful in order to update records, since normal qlaunch would have to listen to an event to see if any new one was added or removed, while this way there is no need to do that.

This is the list of some remarkable advantages this system has:

- Application records: when new records are added (new games installed) or removed (games removed) official qlaunch needs to wait for a system event on an extra thread in order to notice when there are changes, and then re-update its display. Since just by returning HOME the menu is always re-opened, if one installed or removed a title from homebrew, just returning (thus reopening) the menu, which always gets all records on startup, would show new ones or lack the removed ones.

- Communications security: while my first approach was to make the communication system use the **general channel** (storage system where other titles send requests to qlaunch: sleep, HOME press...), but that wouldn't be a good idea since that channel isn't meant for "send-receive" communications, and since the daemon-to-menu part would be handled via **library applet storages** (the system made for an applet and the one who launched it to communicate with each other) it would involve waiting, possible threading... In the end (for what it's worth) this latter system was used for both sides, since in the end it iss made for that purpose. Communications consist on 16KB byte blocks sent and received by and from both.

- qlaunch's essential threading: a regular qlaunch reimplementation does need threads, since it needs to host its special event handling (the aforementioned general channel, applet messages...) appart from the UI and user input. In uLaunch's case this responsibilities (back-end and front-end functionalities) are split into both the menu and the daemon. Despite being almost invisible, since the daemon (being the system applet in this case) has its unique functionality only it can use, the communication system between the menu library applet and the daemon consists on the menu asking the daemon to perform a certain privileged task (launching titles, detecting HOME menu...) and the response from the daemon with the result and/or the asked data.

### Homebrew and forwarding

A common practice to be able to easily launch homebrew, from the main menu and as application, is to take advantage of the *title override* feature most CFWs provide, or as a last resource, to install forwarder NSPs to target homebrew located at the SD card.

Nevertheless, the latter is ban-baity idea, not to mention the disapproval of a big part of homebrew developers due to its tight relation to piracy.

Since eQlipse, I've taken advantage of a lost gem in the switch's system memory: **flog**.

"flog" easter egg was a special application consisting on a NES emulator with a hardcoded ROM of the NES Golf game. Hackers discovered it and people were really impressed of the easter egg (related to Iwata by the way it had to be accessed), but N quickly spoiled the fun of the discovery, leaving it **stubbed** on 4.0.0.

It is important to note that it was stubbed, not removed. That means that the title is there, in your console's memory (on any version), but no longer launchable via the HOME menu trick, and the code would do nothing if it was launched.

However, the fact that it is still there means that **every console has an application title built-in in their systems**. In fact, flog is a special title kind: it is a **system application**, which is completely similar to a normal application, but the difference is that these kind of titles come within the system.

Theorerically, flog isn't the only system application, since **starter** (that special menu shown on the initial configuration of the console) seems to be one too.

The idea of a custom way to target homebrew started in eQlipse, where there were two custom titles: a library applet and a system application, both wrappers of nx-hbloader, which took advantage of library applets' argument storages to target homebrew.

For the rewrite I basically ported both, with some minor fixes and improvements, but the idea and functionality was the same.

Both of them replace certain console titles (eShop applet and flog system application) so launching those with certain input arguments can be used as an improved hbloader.