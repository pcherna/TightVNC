# TightVNC fork

A fork of TightVNC for Windows, carrying local changes to the viewer.

## Where the code came from

The starting point is the TightVNC 2.8.88 GPL source, downloaded from
<https://www.tightvnc.com/download.php>. It is unmodified upstream code by
GlavSoft LLC.

That drop is the first commit on `main`, tagged `upstream-2.8.88`. Nothing else
is in that commit. To see every local change since:

```
git diff upstream-2.8.88
```

Keeping the import separate means the fork's own work is always one command
away, and a later upstream release can be imported the same way.

## License

TightVNC is free software under the GNU General Public License, version 2 or
later. This fork is too, and has to be. The GPL requires that anything derived
from GPL code carries the same license.

The full license text is in `wix-installer/LICENSE.txt`. All 998 source files
carry the GPL header and the GlavSoft copyright notice. Leave both in place.

Two bundled libraries keep their own licenses. See `zlib/LICENSE` for zlib, and
the legal section of `libjpeg/README` for libjpeg.

This program comes with no warranty.

## Building

Windows and Visual Studio only. There is no makefile and no CMake. Open
`tightvnc2017.sln` and build the project you need.

Build `tvnviewer` on its own rather than the whole solution. Building everything
also builds the installer, which needs the WiX v3 SDK.

The projects target the v141_xp toolset and the Windows 8.1 SDK. Neither ships
with current Visual Studio, so it will offer to retarget the solution on load.

## Upstream

- Project site: <https://www.tightvnc.com/>
- Source downloads: <https://www.tightvnc.com/download.php>

Report bugs in the local changes here. Send anything reproducible in stock
TightVNC to upstream instead.
