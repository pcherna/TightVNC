# Cutting a release

Releases carry the viewer only. `tvnviewer.exe` is standalone, so the zip needs
nothing else. The installer is out of scope, and building one would need the WiX
v3 SDK.

## Versioning

Windows gives four version fields. This fork uses them as
`major.minor.upstream-build.fork-build`:

```
2.8.88.0   upstream TightVNC 2.8.88
2.8.88.1   first fork release
2.8.88.2   second fork release
```

Only the fourth field moves. The git tag matches, as `v2.8.88.1`.

Leave the third field alone. `win-system/VersionInfo.cpp` reads a third field
above 100 as a beta marker and rewrites the whole string.

## Steps

1. Bump `FILEVERSION`, `PRODUCTVERSION`, `FileVersion`, and `ProductVersion` in
   these five files:
   - `tvnviewer/tvnviewer.rc`
   - `tvnserver/tvnserver.rc`
   - `hookldr/hookldr.rc`
   - `screen-hooks/screenhooks.rc`
   - `setup-helper/setup-helper.rc`

   Only the viewer ships. Bump all five anyway, so a build from this tree can
   never be mistaken for a GlavSoft one.

   The `.rc` files are UTF-16LE and `.gitattributes` marks them binary. Git will
   not diff them and will not repair them. Edit them in the Visual Studio
   resource editor, or in VS Code with the encoding set to UTF-16 LE.

2. Commit as "Release 2.8.88.N".

3. Build the `tvnviewer` project for `Release|x64`, then for `Release|Win32`.
   Build the project on its own. A whole-solution build pulls in the installer.

   Output lands at `x64\Release\tvnviewer.exe` and `Release\tvnviewer.exe`.

4. Right-click each exe, then Properties, then Details. File version and Product
   version must both read 2.8.88.N.

5. Copy one exe on its own into an empty folder and run it. It must start with no
   missing-DLL error. Open Help > About and confirm the version and build date.

6. Build one zip per architecture, into `dist/`. That folder is git-ignored, as
   is any stray `.zip`, so build output never lands in a commit.

   ```
   tightvnc-viewer-2.8.88.N-x64.zip
     tvnviewer.exe
     README.md
     LICENSE.txt

   tightvnc-viewer-2.8.88.N-win32.zip
     tvnviewer.exe
     README.md
     LICENSE.txt
   ```

7. Record the hashes:

   ```
   certutil -hashfile tightvnc-viewer-2.8.88.N-x64.zip SHA256
   certutil -hashfile tightvnc-viewer-2.8.88.N-win32.zip SHA256
   ```

8. Tag and push:

   ```
   git tag v2.8.88.N
   git push origin main v2.8.88.N
   ```

9. Write the notes from the template below, then publish:

   ```
   gh release create v2.8.88.N dist/*.zip --title "Viewer 2.8.88.N" --notes-file notes.md
   ```

10. Download one zip from the published release. Check its hash, then run the exe.

## Release notes template

```markdown
Viewer-only build of the fork, based on TightVNC 2.8.88.

## What's in it

- The File Transfer dialog remembers the last local and remote folder for each host.
- Places: named shortcuts to folders, on buttons above each path box. A remote
  place can list several candidate paths and lands on the first one that exists.
- A gear button holds options to skip the upload and download confirmations, and
  to name file patterns that overwrite without asking.

## Install

Unzip and run `tvnviewer.exe`. There is no installer.

The exe is unsigned, so SmartScreen will warn you the first time. Verify the hash
below if you want to check the download.

## Files

| File | SHA-256 |
|---|---|
| tightvnc-viewer-2.8.88.N-x64.zip | `...` |
| tightvnc-viewer-2.8.88.N-win32.zip | `...` |

## Source

Built from tag `v2.8.88.N`. TightVNC is free software under the GNU General
Public License, version 2 or later, and so is this fork. The complete source is
this repository.
```

Keep the source paragraph. It is what satisfies section 3 of the GPL. A public
repository with the exact build tagged is enough.

## If you ever ship an installer

Two things must change first, both in `wix-installer/product_definitions.wxi`:

- `UpgradeCode` is still upstream's `B1F272B0-5B47-46f0-9AF2-705E64EB1A69`. A
  fork MSI using it would silently upgrade or collide with a genuine TightVNC
  install. Generate a fresh GUID.
- `ProductNameStub` and `Manufacturer` still say TightVNC and GlavSoft LLC.
