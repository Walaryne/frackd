# frackd
### The File Reaction Daemon

> *frackd* is a usermode daemon for tracking edits of files and running a shell command/script/executable as a result.

> *frackd* uses **Linux** syscalls, it is therefore not compatible with FreeBSD, OpenBSD, OSX, or other Unix like operating systems. And before you ask, no, not Windows either.

> *frackd* requires >= glibc2.4, and a kernel version of >= 2.6.13

> *frackd* maintains a watchlist, created in a `.frackrc` file by the user.
> Each entry in the watchlist has a format of:
> 
> `<path of file to be watched>:<path of script, executable, or a shell command>`
>
> *frackd* runs the command pared with the file upon the save action of most editors.

> *frackd* will check for it's `.frackrc` file in three places (in order) before giving up in error.
>
> ```
> $HOME/.frackrc
> /etc/frackd/frackrc
> /etc/frackrc
> ```

> Sidenote, if $HOME is not defined, tilde (~) expansion in `.frackrc` will fail.


> #### .frackrc example
> ```
> /home/user/.local/myservice/config.txt:/usr/bin/myservice --restart
>
> ~/.local/myotherservice/config file.txt:/usr/bin/myotherservice --reload
> 
> /etc/afile:/usr/bin/myscript
> ```

### Invocation
> To invoke *frackd*, simply run it in a terminal, or at your leasure, an autorun service such as [systemd][1]'s usermode, [i3][0]'s process spawning function, etc. etc.

> *frackd* takes no command line arguments (at the time of writing).

> Simply use `frackd` if it's installed in your path, or the fullpath to it's current location.

> #### NOTE:
> *frackd* is **not** a systemd compliant daemon, as it would require linking against libsystemd, and since not everyone uses (or likes) systemd, and since linkage makes systemd a dependency, I have chosen not to integrate it.

### Issues
> *frackd* should **not** be run as **root** or other superuser comparable accounts, however running it as an account that is a **sudoer** or equivalent is acceptable (basically, don't do `sudo frackd`).

> running *frackd* with **root** privileges is dangerous, since at the time of writing, *frackd* does not check to see if the `.frackrc` file has the correct permissions, and could utilize malicious commands or scripts, and run them as root.

> At the time of writing, *frackd* will check to see if it is UID 0 (root) and forcefully exit if so, this will be changed later if root mode support is added.

### Examples of Usages
> *frackd* as far as I know, is a niche piece of software, I created it for my usage, but realized it may be of use to others.

> I personally use *frackd* for live reloads of my systemwide [pywal][2] theme, allowing a [nitrogen][3] wallpaper change (and subsequent nitrogen configuration change), to trigger a [perl][4] script to run, gathering the file path and feeding it to `wal -i`

> Other usages may include live reloads of server configurations (if the serving daemon supports running reloads), simply by saving the configuration file in your editor!

### Bugs
> *frackd* likely has bugs, and I don't have the foresight to program around all of them before they come into existance.

> If you happen to find a bug, or any other operational errata (such as failures with no errors) send me a report with basic information about your Linux based OS and the logs *frackd* generated, if any, a copy of your `.frackrc` file(s) and a short description of the situation under which the issue occured.

> Take care when submitting logs or `.frackrc` files with personally identifiable information, stay safe out there! (P.S This pretty much goes for any software, you wouldn't believe how much information could be in a syslog, such as WiFi access point names, MAC addresses, and many more things)

[0]:https://github.com/i3/i3 "i3wm GitHub"
[1]:https://github.com/systemd/systemd "systemd GitHub"
[2]:https://github.com/dylanaraps/pywal "Pywal GitHub"
[3]:https://github.com/l3ib/nitrogen "Nitrogen GitHub"
[4]:https://github.com/Perl/perl5 "Perl GitHub"
