# ![logo](https://raw.githubusercontent.com/azerothcore/azerothcore.github.io/master/images/logo-github.png) AzerothCore
## XP Weekend module
- Latest build status with azerothcore: [![Build Status](https://github.com/azerothcore/mod-weekend-xp/workflows/core-build/badge.svg?branch=master&event=push)](https://github.com/azerothcore/mod-weekend-xp)



## Description

This module is a xp module that will allow the server owner/host to select how much XP players can recieve on the weekend via a config file.

During the weekend event it can also boost profession and weapon skill-ups,
reputation and money (looted gold and quest rewards). `XPWeekend.Enabled` turns
the module on or off, `XPWeekend.xpAmount` sets the rate of every bonus
(1 = no bonus, 2 = double, 3 = triple, …), and each bonus, XP included, can be
turned on or off on its own in `mod-double-xp-weekend.conf.dist`. Only the XP
bonus is on by default.

By default the event runs all day on Friday, Saturday and Sunday (server local
time). The days and the daily start/end hour can be changed with
`XPWeekend.Days`, `XPWeekend.StartHour` and `XPWeekend.EndHour` in
`mod-double-xp-weekend.conf`.

With `XPWeekend.AnnounceStartEnd = 1` (off by default), online players are
told when the event starts and ends, and players who log in during the event
are told it is active (not while `XPWeekend.AlwaysEnabled = 1`). Each player
gets the texts in their client language. They are stored in the world database
tables `module_string` and `module_string_locale` (module `mod-weekend-xp`,
ids 16 and 17), where they can be translated or changed (apply with
`.reload module_string`). An empty `module_string` text turns that message
off. The module's SQL file resets these rows each time it changes, so redo
such edits after updating the module. The `XPWeekend.AnnounceStartEnd` comment
in the config file explains the details. The old `XPWeekend.StartMessage` and
`XPWeekend.EndMessage` options were removed and are ignored if still set: move
a custom or empty text into ids 16 and 17.


## How to use ingame

Apply the module and Just log in on the weekend there you will see an announcement upon login to see if the module has been activated.

## Requirements

My new module requires:

- AzerothCore v1.0.1+


## Installation

```
1) Simply place the module under the `modules` directory of your AzerothCore
   source.
2) Re-run cmake and launch a clean build of AzerothCore.
3) Start the worldserver. The module's SQL (`data/sql/db-world/`: module texts
   and the `.weekendxp` commands) is needed, and the worldserver applies it to
   the world database automatically at startup, as long as world database
   updates are enabled (`Updates.EnableDatabases` in `worldserver.conf`, on by
   default).
```

## Edit module configuration (optional)

If you need to change the module configuration, go to your server configuration folder (where your `worldserver` or `worldserver.exe` is), copy `mod-double-xp-weekend.conf.dist` to `mod-double-xp-weekend.conf` and edit that new file.


## Credits

* [Me](https://github.com/Talamortis) (author of the module)

AzerothCore: [repository](https://github.com/azerothcore) - [website](http://azerothcore.org/) - [discord chat community](https://discord.gg/PaqQRkd)
