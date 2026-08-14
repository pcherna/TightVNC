# File transfer dialog: folder memory and named places

Branch `ft-presets`, off tag `upstream-2.8.88`. Design detail in `NOTES.md`.
Full plan in `~/.claude/plans/vast-toasting-haven.md`.

## Phase 1: per-host folder memory

- [x] `client-config-lib/FtHostState.{h,cpp}` storing last local and last
      remote folder under `Software\TightVNC\Viewer\History\<host>`
- [x] Make the `FileTransferCore` state constants public, so the interface
      implementation can decode the `state` it already receives in
      `onFtOpFinished`
- [x] Pass the host name into `FileTransferMainDialog` from
      `ViewerWindow.cpp:776`
- [x] Candidate chain state machine on the dialog, with the generation guard
- [x] Gate `enableControls` on the chain rather than on each operation
- [x] Route user-initiated remote navigation through `navigateRemoteFolder`
      so a chain in flight cannot move the pane afterwards
- [x] Restore both panes on open, remote as a two-candidate chain
- [x] Save each pane on successful navigation
- [x] Add `FtHostState` to `client-config-lib.vcproj`, `.vcxproj`, `.filters`
- [x] Compile `tvnviewer` on Windows. Clean on the first attempt, with the
      toolset and SDK retargeted
- [x] Confirm the build still works now that the SDK version moved into
      `Directory.Build.props`
- [x] Run the four phase 1 manual checks against a real server. All pass,
      including the fallback when a remembered remote folder is gone

## Phase 2: named places

- [ ] `client-config-lib/FtPlaces.{h,cpp}` for the global place definitions
- [ ] Local resolver, a synchronous `GetFileAttributes` loop
- [ ] Remote resolver reusing the phase 1 chain, with the place name as the
      chain description
- [ ] Per-host resolved-answer cache in `FtHostState`, under an `FtResolved`
      subkey
- [ ] Places button per pane in `tvnviewer.rc`, growing the dialog by one
      button row and shifting the log combo, progress bar, and Cancel down
- [ ] Popup menu built at click time, `TPM_NONOTIFY | TPM_RETURNCMD`
- [ ] Rescan: delete the connected host's `FtResolved` subkey, log the count,
      do not navigate
- [ ] Edit Places modal dialog, names on the left and candidates on the right,
      reorderable candidates
- [ ] On OK, drop the edited places' cached answers across every host
- [ ] Add the new files to both project formats
- [ ] Build both solutions and run the phase 2 manual checks

## Domain rules

Accreted as they surface. These are decisions, not guesses.

- Remote resolution hunts candidates. Local resolution hunts too, so that both
  sides share one storage shape and one editor. Local hunting is free.
- No path contains a variable component. No version numbers, usernames, or
  changing drive letters. Plain existence testing is enough.
- First candidate that exists wins. Order is significant, so the editor must
  allow reordering.
- A miss writes one line to the log combo and leaves the pane where it was.
  Per-candidate failures are already logged by `RemoteFileListOperation`.
- Resolved answers cache per host. Rescan clears every cached answer for the
  connected host at once.
- Editing a place's candidates drops that place's cached answers on all hosts.
  Other places keep theirs.
- Places are global, not per host.
- Listen mode is out of scope. Reverse connections all share the registry key
  `.listen`, so per-host state is meaningless there.
- An empty string is a valid saved local folder. It is the "My Computer" root,
  so empty must not be read as unset.

## Review

Written at the end of each phase.

### Phase 1

Implementation complete, not yet compiled. It needs a Windows VM with Visual
Studio, since the tree is MSVC-only.

Two things found while reading the code that were worth confirming rather than
assuming:

`StringStorage::operator =` returns void, so `vector<StringStorage>` is not
assignable in the way the standard containers ask for. The chain copies its
candidates element by element instead.

Firing the next candidate from inside the reply handler looked like it might
race with `executeOperation` deleting the operation that just finished. It does
not. `FileTransferMessageProcessor::processRfbMessage` holds its listener lock
across the whole dispatch, and `executeOperation` blocks on that same lock
before it deletes. The delete cannot overlap the notify.
