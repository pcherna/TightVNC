# Peter's TightVNC Fork

A fork of TightVNC for Windows. A huge shout-out to Konstantin Kaplinskiy and GlavSoft LLC, for a fantastic product.

## AI Tools

The improvements are to the user-interface and not to the underlying remote access / control technology. Claude Opus 5 was used.

## Changes

* The File Transfer dialog remembers the last-visited local and remote folders, on a per-host basis.
* In the File Transfer dialog, the local and remote sides support a Places feature where you can define places (e.g. "Log Folder"), backed by one or more folder locations. Selecting a place moves you to the first folder location that exists at the remote. This lets you have easy access to places that are deeply nested, or that have different physical locations on different groups of fielded systems. Each side puts its first four places on buttons above the path box. The rest, plus Edit Places, sit behind the arrow button at the end of that row. Up and Down in the Edit Places dialog set the order, so you choose which four get buttons.
* As part of this work, retargeted the build at Windows platform v141 (same but drops WinXP).

## Where the code came from

The starting point is the TightVNC 2.8.88 GPL source, downloaded from <https://www.tightvnc.com/download.php>. It is unmodified upstream code by GlavSoft LLC.

That download is the first commit on `main`, tagged `upstream-2.8.88`. Nothing else is in that commit. To see every local change since:

```shell
git diff upstream-2.8.88
```

Keeping the import separate means the fork's own work is always one command away, and a later upstream release can be imported the same way.

## License

TightVNC is free software under the GNU General Public License, version 2 or
later. This fork therefore is also.

The full license text is in `wix-installer/LICENSE.txt`.

Two bundled libraries keep their own licenses. See `zlib/LICENSE` for zlib, and the legal section of `libjpeg/README` for libjpeg.

This program comes with no warranty.

## Building

Windows and Visual Studio only.  Open `tightvnc2017.sln` and build the project you need.

Build `tvnviewer` on its own rather than the whole solution. Building everything also builds the installer, which needs the WiX v3 SDK.

The projects target the v141_xp toolset and the Windows 8.1 SDK. Neither ships with current Visual Studio, so it will offer to retarget the solution on load.

## Upstream

* Project site: <https://www.tightvnc.com/>
* Source downloads: <https://www.tightvnc.com/download.php>

Report bugs in the local changes here. (Anything reproducible in stock TightVNC would need to go to the upstream instead.)
