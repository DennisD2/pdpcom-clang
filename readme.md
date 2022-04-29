
# termtest.c

```
Raw Terminal Dump
:CR:1
:
:LF:1
:0:30:1
:0:30:1
:0:30:1
:0:30:1
:0:30:1
:0:30:1
:CR:1
:
:LF:1
:@:40:1
```

## /dev/tty* permissions

Add file /etc/udev/rules.d/60-extra-acl.rules, there
```
KERNEL=="ttyUSB[0-9]*", GROUP="users", MODE="0660"
```
Details: https://superuser.com/questions/431843/configure-permissions-for-dev-ttyusb0

## RAW / Noncanonical terminal IO and CLion 
In CLion Run/Debug window, that mode does not work as expected.
To have real experience, the app must be run in a standard terminal,
not in CLion.
So if behaviour is strange, try out app in terminal first.