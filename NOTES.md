# File transfer dialog — design notes

Working notes for two planned features in the TightVNC viewer's file transfer
dialog. Written during exploration, before any code changes. Base is pristine
TightVNC 2.8.88 GPL source.

## The two features

1. Remember the last folder visited on each side, per remote host.
2. Named preset locations for each pane. A remote preset holds several candidate
   paths and resolves to the first one that exists.

The second feature is really logical path resolution. A name like "Log Folder"
resolves to whatever layout that machine happens to use. Three or four layouts
exist in the field.

## Decisions already made

- No listen mode. Reverse connections are out of scope.
- Remote presets hunt through candidates. Local presets are a single path each.
- No variable components in any path. No version numbers, usernames, or drive
  letters that change. Plain existence testing is enough.
- When no candidate exists, write a message to the log combo box. Leave the pane
  where it was.
- A rescan action forces re-resolution and discards the cached answer.

## Cache the resolution per host

A resolved answer is stable. Once "Log Folder" resolves to
`C:/ProgramData/Acme/logs` on a given machine, store that result against the
host. The next visit costs one round trip instead of four.

This shares storage with feature 1. Both need a per-host string map: last local
folder, last remote folder, and one entry per resolved preset.

Per-host storage already exists. `ConnectionConfigSM` writes to
`HKCU\...\Viewer\History\<host>`, see `client-config-lib/ConnectionConfigSM.cpp:33`.
`ViewerWindow` holds one as `m_ccsm` and already calls
`m_conConf->saveToStorage(&m_ccsm)` in eight places. `SettingsManager` has
`getString` and `setString`, see `config-lib/SettingsManager.h:41`.

Two caveats. The registry key is the host string as typed, so `myhost`,
`myhost:5900`, and an IP address are three separate entries. And
`ConnectionConfig` has no string fields today, everything is bool or int.

## Four things that will bite

### The chain spans multiple operations

`onMessageReceived:217` calls `enableControls(true)` and `setProgress(0.0)` every
time an operation finishes. A four candidate hunt is four operations. So the UI
re-enables between candidates and the user can click mid-hunt.

Gating must be chain aware. Stay disabled until the whole chain settles.

### No request/reply correlation

`m_lastSentFileListPath` is a single slot. `onFtOpFinished(state, result)` says a
file list operation finished, but not which one. Click one preset then another
quickly, and replies from the first chain advance the second chain's index.

Put a generation counter on the chain and check it when the reply lands.

### enableControls is a hand-written list

`FileTransferMainDialog.cpp:719` is a sequence of `setEnabled` calls, not a loop
over a container. New controls get no gating unless added by hand. The comment at
line 156 refers to an `m_controlsToBlock` member that does not exist.

### The dialog cannot see which host it is talking to

`ViewerWindow.cpp:776` constructs the dialog as
`FileTransferMainDialog(m_fileTransfer->getCore())`. That is all it gets. No host
name, no `ConnectionConfig`, no settings manager. `ViewerWindow` holds `m_ccsm`
and `m_conConf` but passes neither.

Both features need per-host storage, so both need this constructor changed.

## How remote navigation works

`tryListRemoteFolder` at `FileTransferMainDialog.cpp:854` sets
`m_lastSentFileListPath` and fires a request into the RFB stream. It does not
wait. The reply arrives later on another thread.

The failure signal is already plumbed. `RemoteFileListOperation::onLastRequestFailedReply`
sets `m_isOk = false`. `FileTransferCore::onUpdateState:303` gates on
`result == 0` and skips `setNothingState()`, so a failed listing leaves the pane
on its old path. That gives a clean "this candidate did not work" hook.

A path that exists but denies access returns the same failure. Skipping to the
next candidate is the right response, so this does not need special handling.

## Restore is just another chain

`onInitDialog:89-90` hardcodes `tryListRemoteFolder("/")` and
`tryListLocalFolder("")`. Restoring a saved remote folder can fail too, so the
restore is a two candidate chain: saved path, then `/`.

Build one chain mechanism and restore comes free. Treat it as a special case and
the logic gets written twice.

## Open question: buttons or a menu

`ftclient_mainDialog` in `tvnviewer/tvnviewer.rc:75` is 503 by 320 dialog units,
fixed size, every control hard positioned. Each pane is 226 units wide and the
existing four button row fills it exactly. Vertical slack between that row and
the log combo is about 13 units, and a button needs 15.

A row of preset buttons means growing the dialog and shifting the combo, progress
bar, and Cancel button down. That caps out at roughly four presets per pane.

The alternative is one "Places" button per pane that pops a menu. `gui/Menu.h`
already wraps `TrackPopupMenu`. It costs one control slot, scales to any number
of presets, and leaves room for readable names. Adding a preset later becomes a
config change instead of a dialog redesign. Rescan fits as a menu item, which
beats a hidden modifier key.

Buttons only win if the preset list stays at three or four forever.

## Build environment

The tree is MSVC only. 35 projects, `tightvnc.sln`. No makefile, no CMake. Cross
compiling from macOS is possible with llvm-mingw but means writing the build
system from scratch, and the result cannot be tested without Windows. Use a
Windows VM with Visual Studio.

Upstream targeted the `v141_xp` toolset and the Windows 8.1 SDK. Neither ships
with Visual Studio 2026, so the projects now use plain `v141` and Windows SDK
`10.0.26100.0`. Install the "MSVC v141 build tools" individual component
alongside the C++ desktop workload.

The SDK version is pinned rather than set to the bare `10.0` shorthand. v141
does not honour that shorthand and fails with "The Windows SDK version 10.0 was
not found" even when an SDK is installed. Building on a machine with a different
SDK means changing this value, which "Retarget solution" does.

Stay on v141 rather than a newer toolset. The tree has 298 dynamic exception
specifications across 81 files, which C++17 removed from the language. v141
defaults to C++14, where they still compile.

Build the `tvnviewer` project on its own, not the whole solution. It carries
project references to all 17 libraries it needs. Building everything also pulls
in `setup-helper`, which wants `wcautil.h` and `msi.lib` from the WiX v3 SDK.

Output lands in `Debug\` or `Release\` at the repo root for Win32, and under
`x64\` for 64 bit.
