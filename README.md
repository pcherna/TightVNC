# Peter's TightVNC Fork

A fork of TightVNC for Windows. A huge shout-out to Konstantin Kaplinskiy and GlavSoft LLC, for a fantastic product.

## Use of AI Coding Tools

The improvements are to the user-interface and not to the underlying remote access / control technology, avoiding those more sensitive areas. Claude Opus 5 was used in development.

## Download

Get the latest `tightvnc-viewer-<version>-x64.zip` from the [Releases page](https://github.com/pcherna/TightVNC/releases). Unzip it and run `tvnviewer.exe`. There is no installer and nothing to register. A 32-bit build is there too if you need one.

Releases carry the viewer only. Use the stock TightVNC server, which this fork does not change.

The exe is unsigned, so Windows SmartScreen warns you the first time. Click "More info", then "Run anyway". Check the SHA-256 hash against the release notes if you want to be sure of what you downloaded.

The viewer reads and writes the same registry keys as stock TightVNC, under `HKCU\Software\TightVNC\Viewer`. Your existing connection history and settings carry over. So do any changes you make here, if you go back to the stock viewer.

## Changes

* The File Transfer dialog remembers the last-visited local and remote folders, on a per-host basis.
* In the File Transfer dialog, the local and remote sides support a Places feature:
  * You can define places (e.g. "Logs"), backed by one or more folder locations. Selecting a place moves you to the first folder location that exists at the remote. This lets you have easy access to places that are deeply nested, or that have different physical locations on different groups of fielded systems. Each side puts its first four places on buttons above the path box. The rest, plus Edit Places, sit behind the arrow button at the end of that row. Up and Down in the Edit Places dialog set the order, so you choose which four get buttons.
* The File Transfer dialog has options that streamline the transfer process, accessible from the new gear-icon button:
  * An option to skip the "Do you wish to upload the selected files?"
  * An option to skip the "Do you wish to download the selected files?"
  * Support for one or more file patterns for which the Overwrite-confirmation dialog is skipped
* As part of this work, retargeted the build at Windows platform v141 (same but drops WinXP).

## Where the code came from

The starting point is the TightVNC 2.8.88 GPL source, downloaded from <https://www.tightvnc.com/download.php>, which is the unmodified upstream code by GlavSoft LLC.

That download is the first commit on `main`, tagged `upstream-2.8.88`. Nothing else is in that commit.

## Versions

Windows gives four version fields. This fork uses them as `major.minor.upstream-build.fork-build`, so `2.8.88.1` is the first fork release built on upstream 2.8.88. Release tags match, as `v2.8.88.1`.

`RELEASING.md` has the steps for cutting one.

## License

TightVNC is free software under the GNU General Public License, version 2 or
later. This fork therefore carries the same license.

The full license text is in `LICENSE.txt`.

Two bundled libraries keep their own licenses. See `zlib/LICENSE` for zlib, and the legal section of `libjpeg/README` for libjpeg.

This program comes with no warranty.

## Building

Windows and Visual Studio only.  Open `tightvnc2017.sln` and build the project you need.

Build `tvnviewer` on its own rather than the whole solution. Building everything also builds the installer, which needs the WiX v3 SDK.

The projects target the v141 toolset, which no longer ships with Visual Studio, so it will offer to retarget the solution on load. Decline, and install the "MSVC v141 build tools" component instead. The Windows SDK version lives in `Directory.Build.props` and defaults to `10.0.26100.0`.

## Upstream

* Project site: <https://www.tightvnc.com/>
* Source downloads: <https://www.tightvnc.com/download.php>

Report bugs in the local changes here. (Anything reproducible in stock TightVNC would need to go to the upstream instead.)
