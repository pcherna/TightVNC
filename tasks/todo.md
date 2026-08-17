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

## Phase 2a: named places, configured in the registry

- [x] `client-config-lib/FtPlaces.{h,cpp}` for the global place definitions
- [x] Local resolver, a synchronous walk using `File::exists` and
      `File::isDirectory`
- [x] Remote resolver reusing the phase 1 chain, with the place name as both
      the chain description and the cache key
- [x] Per-host resolved-answer cache in `FtHostState`, under an `FtResolved`
      subkey
- [x] Places button per pane in `tvnviewer.rc`, growing the dialog by one
      button row and shifting the log combo, progress bar, and Cancel down
- [x] Popup menu built at click time, `TPM_NONOTIFY | TPM_RETURNCMD`
- [x] Rescan: delete the connected host's `FtResolved` subkey, log the count,
      do not navigate
- [x] Add the new files to both project formats
- [x] Build and run the phase 2a manual checks

## Phase 2b: the Edit Places dialog

- [x] Modal dialog, names on the left and candidates on the right, with
      reorderable candidates
- [x] `FtPlaces::save`, plus `copyTo` for taking a working copy
- [x] An Edit Places item in both Places menus
- [x] On OK, drop the edited places' cached answers across every host, by
      enumerating `History\*` with `getSubKeyNames`
- [x] Add the new files to both project formats
- [x] Build and run the phase 2b manual checks


Plan in `~/.claude/plans/i-want-to-streamline-temporal-duckling.md`.

- [x] `FileExistDialog::isSkipAll`, so a caller can stand down
- [x] Guard the download confirmation, track the running direction, and answer
      Overwrite on a pattern match in `onFtTargetFileExists`
- [x] Grow `IDD_CONFIGURATION` to 221x320 and add the File Transfer group
- [x] Add the new files to all three project formats
- [x] Prove the matcher against a table of cases
- [x] Make Enter act on the selection in all three text boxes, across the
      Configuration and Edit Places dialogs
- [x] Build and run the phase 3 manual checks


Written out here rather than left in the plan file, because the build happens
on a different machine from the one that wrote them.

run against 31 cases, so these checks cover the wiring around it, not the
matching itself.

- [x] Open Configuration. The File Transfer group renders without clipping and
      the OK and Cancel buttons sit below it.
- [x] Add `*.log` and `bill_202*.*`. Press OK. Check that
- [x] Reopen Configuration. Both patterns come back.
- [x] Add a pattern, then press Cancel. The registry is unchanged.
- [x] Connect, open file transfer, download a file with the checkbox off. The
      confirmation still appears. Turn the checkbox on. It does not.
- [x] Download `x.log` twice into a folder that already holds it. The second
      download overwrites silently. The file's timestamp changes.
- [x] Download `notes.txt` twice. The conflict dialog still appears.
- [x] Start a multi-file download holding `a.log` and two non-matching files.
      Press Skip All on the first conflict. `a.log` is skipped, not
      overwritten.
- [x] Upload a file that exists remotely. Both the confirmation and the
      conflict dialog still appear.
- [x] Remove every pattern and press OK. The registry key is empty and the
      conflict dialog returns.
- [x] Type a pattern and press Enter with no row selected. It is added and the
      dialog stays open.
- [x] Select a row, edit the text, press Enter. The row is replaced, not
      duplicated.
- [x] Press Enter with the pattern box empty. The dialog closes and saves.
- [x] Repeat the three Enter checks in Edit Places, for both the place name box
      (add, then rename) and the candidate path box (add, then replace).

## Phase 4: places on the toolbar

The first few places of each pane get a button of their own, on a row above
the path box. Everything else moves behind one button at the end of that row.

- [x] `FtPlaces` stores an `Order` value in each place's key, and `load` sorts
      by it
- [x] `save` writes the order from the list position
- [x] Up and Down for the places list in Edit Places, growing that dialog to
      320 x 210
- [x] Four place buttons and an overflow button per pane in `tvnviewer.rc`,
      on a new row above the path box
- [x] Grow the file transfer dialog to 503 x 359 and shift everything below
      the new row down by 19
- [x] Label the buttons when the dialog opens, and hide the empty slots
- [x] Menu holds only the places past the fourth, plus Rescan and Edit Places
- [ ] Build and run the phase 4 manual checks

### Phase 4 manual checks

- [x] Open file transfer with no places defined. Each pane shows the arrow
      button alone. Its menu says "(no places defined)" and offers Edit Places.
- [x] Define one place. It appears on the first button. The other three stay
      hidden. The menu holds no place entries and no leading separator.
- [ ] Define six places. Four sit on buttons and two are in the menu.
- [ ] Click each of the four buttons. Each navigates to its own place.
- [ ] Pick a place from the menu. It navigates to the right one, not to an
      entry four rows off.
- [ ] Select the fifth place in Edit Places, press Up twice, press OK. It
      moves onto the third button straight away.
- [ ] Check that a name of about twelve characters fits a button without
      clipping, now that the buttons are 50 units rather than 67.
- [x] Check that `HKCU\Software\TightVNC\Viewer\FtPlaces\Local\<name>` holds an
      `Order` value alongside the numbered candidate values.
- [ ] Add a place under that key by hand, with a candidate but no `Order`.
      Reopen the menu. The place appears last.
- [ ] Confirm the arrow glyph on the overflow button renders as a triangle and
      is not clipped by the 18 unit button.
- [ ] Check both panes. The buttons line up with the path box on the left and
      the arrow button meets the right edge.
- [ ] Start a transfer. Every place button greys with the rest of the controls
      and comes back when the transfer ends.
- [ ] Rescan still appears in the remote menu and not in the local one.
- [ ] Give a place a very long name. The button clips it and nothing else on
      the row moves.
- [ ] Reorder places in Edit Places and press OK. The remote pane still uses
      its cached folders, since order changes no candidate.
- [ ] Confirm the log combo, progress bar and Cancel button sit at the bottom
      of the taller window without clipping.
- [ ] Name a place `Support\BCF` and give it a candidate. The list shows
      `Support/BCF` as soon as you press Add. After OK it gets a button, and
      `FtPlaces\Local` holds one key named `Support/BCF` with no `Support` key
      beside it.
- [ ] Confirm the stray `Support` key left by the earlier build is gone after
      that save.
- [ ] Rename a place to one holding a backslash, where the forward slash form
      already exists. The duplicate warning appears.
- [ ] Add a place name, add no path, press OK. The warning names that place.
      Press Cancel. The dialog stays open with the place selected and the
      cursor in the path box.
- [ ] Type a path there and press Enter. It is added. Press OK. The dialog
      closes with no warning and the place gets a button.
- [ ] Add two places with no paths, press OK, press OK on the warning. Both
      are dropped and the rest still save.
- [ ] Press Enter on the warning box. It cancels rather than saves.
- [ ] Hover the arrow button on each pane. A "Places" tooltip appears on both.
- [ ] Start a transfer and hover it again. No tooltip, because the button is
      disabled.
- [ ] Close and reopen the file transfer dialog. The tooltip still works.

## Domain rules

Accreted as they surface. These are decisions, not guesses.

- Remote resolution hunts candidates. Local resolution hunts too, so that both
  sides share one storage shape and one editor. Local hunting is free.
- No path contains a variable component. No version numbers, usernames, or
  changing drive letters. Plain existence testing is enough.
- First candidate that exists wins. Order is significant, so the editor must
  allow reordering.
- A candidate that fails is a warning, not an error, because a later one may
  still work. Each failed candidate writes `Warning: <path> not found ...`.
  Only once every candidate has failed does the place raise a single
  `Error: no folder found ... for <place name>`, naming the place rather than
  any one path.
- `RemoteFileListOperation` writes its own error on a failed listing, which is
  right for a folder the user asked for directly and wrong mid-hunt. The
  dialog suppresses it while a chain is running and warns itself instead.
- A miss leaves the pane where it was.
- Candidate paths may be written with either slash. Loading rewrites them to
  the separator the pane uses, forward for remote and backward for local, so
  whoever edits the registry need not remember which side wants which.
- Resolved answers cache per host, but only for the remote pane. Local
  resolution costs a few file system calls, so caching it would buy nothing
  and could only go stale. The local menu therefore has no Rescan item.
- A cached answer is used as the first candidate of the hunt rather than
  replacing it. If the folder has gone, the chain carries on into the real
  candidates and whatever wins overwrites the stale entry.
- Rescan clears every cached answer for the connected host at once.
- Editing a place's candidates drops that place's cached answers on all hosts.
  Other places keep theirs.
- Places are global, not per host.
- Listen mode is out of scope. Reverse connections all share the registry key
  `.listen`, so per-host state is meaningless there.
- An empty string is a valid saved local folder. It is the "My Computer" root,
  so empty must not be read as unset.
- The editor writes nothing until OK, so Cancel leaves the registry untouched.
- Saving rewrites a side's definitions wholesale rather than merging, so a
  removed place disappears and no stale candidate numbering survives.
- A place with no candidates is not written. It could never resolve.
- The editor warns before dropping one, and names it. The name sits in the
  list looking saved, so losing it without a word is a trap. The rule is about
  what reaches the registry, not about how quietly it happens.
- Cancel on that warning returns to the dialog with the first offender
  selected and the path box focused, which is where the work is.
- Cancel is the default button on it. A stray Enter would otherwise discard
  the very names being warned about.
- The overflow arrow carries a "Places" tooltip. It shows no words of its own,
  so what sits behind it has to be said somewhere. The named buttons beside it
  get none, since each already reads as its own place.
- The tooltip window is owned by the dialog, so it dies with it. Reopening the
  file transfer dialog builds a fresh one.
- `TTF_SUBCLASS` lets the tooltip take the mouse messages itself. A dialog has
  no message loop of its own to relay them from.
- `StringStorage::operator =` returns void, so it is not assignable in the
  sense the standard containers ask for. Anything that would shift elements by
  assignment, `vector::erase` and whole-vector assignment both, is written as
  a rebuild through copy construction instead.
- Enter in a text box beside a list acts on the selection. It renames or
  replaces the highlighted row, and adds a row when none is highlighted.
  Selecting a row copies it into the box, so select, edit, Enter has to mean
  rename or replace. Adding there would leave the old row behind next to a
  near-duplicate.
- Enter in an empty box closes the dialog, which is what it does everywhere
  else in it. Enter is swallowed when the action refuses, a duplicate for
  instance, because closing on the back of a refusal is the same loss the fix
  exists to prevent.
- Places have an order the user sets, held in an `Order` value inside each
  place's key. Registry enumeration is alphabetical, which is not an order
  anyone chose, and the buttons make the first few places matter.
- The candidate values are numbers and `Order` is a word, so the two names
  cannot collide inside the same key.
- A place with no `Order` value sorts after every place that has one, keeping
  its alphabetical position among the others. A place added to the registry by
  hand appears at the end rather than displacing a button.
- Order is written from the list position, counted over the places actually
  written. A place skipped for having no candidates leaves no gap.
- The buttons act on the places as they were last read, without rereading
  first. A button always goes where its own label says.
- The menu is the only thing that rereads, and it relabels the buttons when it
  does. That keeps the promise that a registry edit shows up without a
  restart.
- An empty button slot is hidden, not greyed. A greyed button with no caption
  says a place is there but unavailable, which is not what an empty slot means.
- The menu holds only the places past the fourth. Repeating the ones already on
  buttons would make the row look like it had failed to take them.
- The menu has no separator when every place is on a button, because nothing
  sits above it to separate.
- The menu is right aligned on its button, which sits at the right edge of the
  pane. A left aligned menu would hang off the window.
- Place buttons are a fixed width and clip a name too long for them. Equal
  widths keep the two panes in step, and a clipped name is a naming problem the
  user can see and fix.
- Reordering places invalidates no cached resolution. The cache is keyed by
  place name and turns on the candidates, and order touches neither.
- A backslash in a place name becomes a forward slash. The registry API reads
  a backslash in a key name as a path separator and offers no escape, so
  "Support\BCF" became a key "Support" holding a key "BCF", neither carrying
  candidates. Both then vanished, because a place with no candidates is
  skipped on load.
- Forward slash is legal in a registry key name, unlike in a file name, so it
  is a real substitute rather than a mangling. It also reads the same way to a
  person.
- Names are normalised where they are typed, not only where they are saved.
  The list, the duplicate check and the per-host resolved-answer cache all key
  on the name, and a name spelled two ways would miss the cache.
- `save` normalises again rather than trusting its caller, because a backslash
  reaching `RegCreateKeyEx` produces a nested key and nothing downstream
  reports it.
- Saving deletes every existing subkey tree first, so a nested key left by an
  earlier build disappears the next time the editor writes.

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

### Phase 4

Built. The manual checks are still running.

The first check found a bug older than this phase. A place named `Support\BCF`
never appeared. The registry API reads a backslash in a key name as a path
separator, so `save` made a key `Support` holding a key `BCF`, and `load`
skipped both for having no candidates. Nothing reported it. Place names are now
rewritten to forward slash, which the registry does allow in a key name, at the
point they are typed and again in `save`.

Normalising at input rather than only at save is what keeps the per-host
resolved-answer cache working. That cache is keyed by place name, and holds it
as a value name rather than a key name, so a backslash was legal there and the
two spellings would have missed each other.

The same check turned up a second silent loss standing beside the first. A
place with no candidate paths is dropped on save, by a rule that is right in
itself, but it went without a word. The editor now names those places on OK
and offers to go back. The rule governs what reaches the registry, not how
quietly it happens.

Putting places on buttons forced a question the feature had dodged. There was
no order. `load` returned whatever the registry enumeration gave back, which is
alphabetical by name. Buttons make the first few places matter, so promoting a
place would have meant renaming it. An `Order` value per place and Up and Down
in the editor fix that, and the menu shows the same order.

The row carries four buttons. It started at three of 67 dialog units, which
left the pane wider than it needed to be. Four of 50 with two units between
them fill the same 206 units, and 50 units is about twelve characters. Only
`PLACE_BUTTON_COUNT`, the resource, and the command switch know the number.

The order is the list position rather than a field on `FtPlace`. `save` writes
the position out and `load` sorts by what it reads, so the editor gets
reordering for free from the vector it already holds.

Sorting needed care. `StringStorage` assignment returns void, so `std::sort`
cannot be pointed at a `vector<FtPlace>`. `load` sorts a vector of
(order, position read) pairs instead and rebuilds the list from that. Ties keep
the alphabetical order the registry handed back, which is what puts unordered
places at the end in a sensible sequence. `swapPlaces` in the editor rebuilds
through copy construction for the same reason `erasePlace` does.

Two things the layout change dragged in. The transfer arrows are centred
against the file lists, so they moved down by the same 19 units as everything
else. The overflow menu is right aligned now, since its button sits at the
right edge of the pane rather than the left.

The arrow glyph on the overflow button is the one thing that cannot be checked
from here. It is U+25BC in a UTF-16 resource file, and Ms Shell Dlg 2 should
have it, but the manual checks look at it.

[x] Local-side places buttons missing?
[x] Places arrow-dropdown needs a tooltip

[ ] In the "File already exists" dialog, X doesn't seem to do anything

[ ] Think about moving the overwrite options into the transfer window
[ ] Upload confirmation should also be gated on the overwrite setting
[ ] existing overwrite settings but add/replace/remove beside the list, whereas the Places puts them under, and uses Rename not Replace
