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

## Phase 3: streamlined downloads

Plan in `~/.claude/plans/i-want-to-streamline-temporal-duckling.md`.

- [x] `client-config-lib/FtAutoOverwrite.{h,cpp}` storing filename patterns
      under `Software\TightVNC\Viewer\FtAutoOverwrite`, plus the glob matcher
- [x] `ViewerConfig` gains `SkipDownloadConfirm`, defaulting to off
- [x] `FileExistDialog::isSkipAll`, so a caller can stand down
- [x] Guard the download confirmation, track the running direction, and answer
      Overwrite on a pattern match in `onFtTargetFileExists`
- [x] Grow `IDD_CONFIGURATION` to 221x320 and add the File Transfer group
- [x] Add the new files to all three project formats
- [x] Prove the matcher against a table of cases
- [x] Make Enter act on the selection in all three text boxes, across the
      Configuration and Edit Places dialogs
- [x] Build and run the phase 3 manual checks

### Phase 3 manual checks

Written out here rather than left in the plan file, because the build happens
on a different machine from the one that wrote them.

The matcher is already proved. It was extracted from `FtAutoOverwrite.cpp` and
run against 31 cases, so these checks cover the wiring around it, not the
matching itself.

- [x] Open Configuration. The File Transfer group renders without clipping and
      the OK and Cancel buttons sit below it.
- [x] Add `*.log` and `bill_202*.*`. Press OK. Check that
      `HKCU\Software\TightVNC\Viewer\FtAutoOverwrite` holds values `0` and `1`.
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
- [x] Build and run the phase 4 manual checks

### Phase 4 manual checks

- [x] Open file transfer with no places defined. Each pane shows the arrow
      button alone. Its menu says "(no places defined)" and offers Edit Places.
- [x] Define one place. It appears on the first button. The other three stay
      hidden. The menu holds no place entries and no leading separator.
- [x] Define six places. Four sit on buttons and two are in the menu.
- [x] Click each of the four buttons. Each navigates to its own place.
- [x] Pick a place from the menu. It navigates to the right one, not to an
      entry four rows off.
- [x] Select the fifth place in Edit Places, press Up twice, press OK. It
      moves onto the third button straight away.
- [x] Check that a name of about twelve characters fits a button without
      clipping, now that the buttons are 50 units rather than 67.
- [x] Check that `HKCU\Software\TightVNC\Viewer\FtPlaces\Local\<name>` holds an
      `Order` value alongside the numbered candidate values.
- [x] Add a place under that key by hand, with a candidate but no `Order`.
      Reopen the menu. The place appears last.
- [x] Confirm the arrow glyph on the overflow button renders as a triangle and
      is not clipped by the 18 unit button.
- [x] Check both panes. The buttons line up with the path box on the left and
      the arrow button meets the right edge.
- [x] Start a transfer. Every place button greys with the rest of the controls
      and comes back when the transfer ends.
- [x] Rescan still appears in the remote menu and not in the local one.
- [x] Give a place a very long name. The button shows the start of it followed
      by an ellipsis, and nothing else on the row moves.
- [x] Give a place a name that only just overflows. One or two characters go
      and the ellipsis appears, rather than half the name.
- [x] Give a place a one-word name that fits. It stays centred with no
      ellipsis.
- [x] Give a place a name of a single very wide character. The button shows an
      ellipsis rather than an empty face.
- [x] Hover a shortened button. The tooltip shows the whole name.
- [x] Hover a button whose name fits. No tooltip appears.
- [x] Rename that place to something long, press OK, hover it again. The
      tooltip appears with the new name.
- [x] Rename it back to something short and hover again. The tooltip is gone.
- [x] Remove enough places that a button hides, then hover where it was. No
      tooltip appears from the empty slot.
- [x] Reorder places in Edit Places and press OK. The remote pane still uses
      its cached folders, since order changes no candidate.
- [x] Confirm the log combo, progress bar and Cancel button sit at the bottom
      of the taller window without clipping.
- [x] Name a place `Support\BCF` and give it a candidate. The list shows
      `Support/BCF` as soon as you press Add. After OK it gets a button, and
      `FtPlaces\Local` holds one key named `Support/BCF` with no `Support` key
      beside it.
- [x] Confirm the stray `Support` key left by the earlier build is gone after
      that save.
- [x] Rename a place to one holding a backslash, where the forward slash form
      already exists. The duplicate warning appears.
- [x] Add a place name, add no path, press OK. The warning names that place.
      Press Cancel. The dialog stays open with the place selected and the
      cursor in the path box.
- [x] Type a path there and press Enter. It is added. Press OK. The dialog
      closes with no warning and the place gets a button.
- [x] Add two places with no paths, press OK, press OK on the warning. Both
      are dropped and the rest still save.
- [x] Press Enter on the warning box. It cancels rather than saves.
- [x] Hover the arrow button on each pane. A "Places" tooltip appears on both.
- [x] Start a transfer and hover it again. No tooltip, because the button is
      disabled.
- [x] Close and reopen the file transfer dialog. The tooltip still works.

## Phase 5: a dialog of its own for transfer options

The file transfer settings sat in a group at the bottom of the viewer
configuration dialog, which is reached from the tray icon and is nowhere near
a transfer in progress. They move to a dialog of their own, opened by a gear
button in the file transfer window.

- [x] `tvnviewer/FtOptionsDialog.{h,cpp}`, holding the pattern editor moved
      out of `ConfigurationDialog` unchanged
- [x] `ViewerConfig` gains `SkipUploadConfirm`, defaulting to off
- [x] Gate the upload confirmation on it, the way the download one is gated
- [x] New `ftclient_optionsDialog` at 244x190, grouped by direction: an
      Upload group holding one checkbox, and a Download group holding the
      checkbox and the pattern editor
- [x] Shrink `IDD_CONFIGURATION` back to 221x216 and drop the File Transfer
      group, restoring `ConfigurationDialog.{h,cpp}` to their pre-phase-3 state
- [x] Draw `res/gear.ico` at 16, 24, 32 and 48
- [x] Gear button under the two transfer arrows, `BS_ICON` with the icon set
      through `BM_SETIMAGE`
- [x] A "Transfer Options" tooltip on it, sharing the places tooltip window
- [x] Add the new files to all three project formats
- [x] Build and run the phase 5 manual checks

### Phase 5 manual checks

- [x] Open Configuration from the tray icon. The File Transfer group is gone,
      the window is short again, and OK and Cancel sit right below Logging.
- [x] Open file transfer. A gear button sits below the two transfer arrows and
      shows a gear, not an empty face or a box.
- [x] Hover the gear. A "Transfer Options" tooltip appears.
- [x] Press it. The Transfer Options dialog opens over the file transfer
      window.
- [x] Both checkboxes come back holding what the registry says. The pattern
      list comes back filled.
- [x] Add a pattern, press OK, reopen. The pattern is there and
      `HKCU\Software\TightVNC\Viewer\FtAutoOverwrite` holds it.
- [x] Add a pattern, press Cancel, reopen. Nothing was kept.
- [x] Tick "before uploading", press OK. Check that
      `HKCU\Software\TightVNC\Viewer\SkipUploadConfirm` is 1.
- [x] Upload a file. The Yes/No box does not appear.
- [x] Upload a file that exists remotely. The conflict dialog still appears,
      even for a name matching a pattern.
- [x] Untick it. The upload confirmation comes back.
- [x] Tick "before downloading" and confirm downloads behave as they did.
- [x] Add a pattern while a transfer is running, press OK, then start a new
      download. The new pattern applies.
- [x] Press the gear during a transfer. It is greyed with the other controls.
- [x] Type a pattern and press Enter with no row selected. It is added and the
      dialog stays open.
- [x] Select a row, edit the text, press Enter. The row is replaced.
- [x] Press Enter with the pattern box empty. The dialog closes and saves.
- [x] Check that the Upload group holds one checkbox, and that the Download
      group holds the other checkbox and the whole pattern editor.
- [x] Tab through the dialog. The order runs upload checkbox, download
      checkbox, list, the three buttons, pattern box, OK, Cancel.
- [x] Check the dialog at 125 and 150 percent scaling. The gear stays sharp
      and neither group clips.

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
- Both download shortcuts apply to downloads only. An upload that would replace
  a remote file still asks, because the file at risk belongs to the other
  machine.
- A matching pattern always overwrites. There is no per-pattern skip action, so
  the list reads as one rule rather than a rule set.
- Patterns match the bare file name, not the path, and ignore case as the
  Windows file system does.
- A dot carries no special meaning in a pattern. `*.log` wants a name ending in
  `.log` and `*.*` wants a name containing a dot. `PathMatchSpec` was rejected
  for inheriting the DOS rule that `*.*` also matches a name with no extension,
  and for adding a link dependency.
- Skip All outranks the patterns. It is an explicit instruction about the whole
  batch, said out loud during the transfer. Overwrite All needs no such check,
  since it already reaches the same answer.
- The file name is taken from the destination path, not from either `FileInfo`.
  `DownloadOperation` and `UploadOperation` hand their source and target to
  `targetFileExists` in opposite order, and a `FileInfo` carries whatever name
  it was built with.
- Patterns reload when a download starts, so an edit made in the configuration
  dialog reaches the transfer already on screen.
- The patterns live in their own registry key rather than in `ViewerConfig`. A
  list needs numbered values and `SettingsManager` offers no way to store one.
  A delimited single string was rejected because every plausible delimiter,
  semicolons included, is legal in a Windows filename.
- The list holds at most 64 patterns. Adding a 65th is refused in the editor
  rather than dropped on save, so a pattern that will not be kept is never
  shown as if it had been.
- Both settings are global, not per host, matching how places are stored.
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
- Place buttons are a fixed width. Equal widths keep the two panes in step.
- A name too long for its button is cut back to a leading portion and an
  ellipsis. A push button centres its caption and clips both ends, so the
  untouched name came out as its middle, which reads as nothing. The start of
  a name is the part that identifies it.
- The name is measured against the button rather than cut at a character
  count. The dialog font is proportional, so a character count would be wrong
  in both directions.
- Measuring selects the button's own font into the DC first. The dialog font
  is not the system default, so measuring without it would answer for the
  wrong typeface.
- A shortened button carries the full name on a tooltip. A name the button
  shows in full carries none, because repeating it would be noise.
- Every place button joins the tooltip as a tool at startup and is given its
  text later. A tool holding an empty string shows nothing, so an unused slot
  and an untruncated name need no adding or removing.
- The tooltip is built before the buttons are first labelled, since labelling
  a button also decides whether that button needs a tip.
- Tooltip text is held in a member per button. The tooltip keeps the pointer
  it is given rather than copying the string, so the text has to outlive the
  call, and anything that rewrites it must hand the new pointer over straight
  afterwards.
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

- The file transfer settings live in a dialog of their own, opened from the
  file transfer window. The configuration dialog is reached from the tray icon,
  which is nowhere near a transfer in progress.
- They live there only. Keeping them in both places would mean two dialogs
  editing the same registry values, each needing to load and save correctly.
- The upload setting covers the question asked before the transfer starts, and
  nothing else. A remote file that would be replaced still opens the conflict
  dialog, and the overwrite patterns stay downloads only. The file at risk
  belongs to the other machine either way.
- The gear button carries an icon and no caption, so a tooltip names it. It
  joins the tooltip window the places arrows already use.
- The icon is a real `.ico` at four sizes rather than a text glyph. `BS_ICON`
  with `BM_SETIMAGE` renders the same everywhere, while a gear character
  depends on the dialog font carrying it.
- `LoadImage` at 16 by 16, not `LoadIcon`. `LoadIcon` answers with the large
  icon and leaves the button to scale it down.
- The pattern editor moved across unchanged. It was already self-contained, so
  copying it whole kept one behaviour rather than growing a second.
- `ConfigurationDialog` returns to exactly its pre-phase-3 state. Every change
  ever made to it belonged to the group that moved out.
- The options dialog groups by direction, not by kind of setting. Upload and
  Download each hold everything that governs them, so the asymmetry between
  the two sides is visible in the shape of the dialog. Grouping the two
  checkboxes together under Confirmation hid it.
- The options dialog saves settings itself rather than leaving it to the
  viewer, because it is opened from a transfer and the next download reads the
  setting straight back.

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

### Phase 3

Implementation complete, not yet compiled. The Windows build and the manual
checks are still outstanding.

The glob matcher is the one piece that could be proved here. It was extracted
from `FtAutoOverwrite.cpp` and run against 31 cases on macOS, covering case
folding, the `*.*` rule, and the backtracking a pattern such as `*a*b*c*`
needs. All pass. The harness pulls the function straight out of the source, so
it cannot drift from what ships.

Three things surfaced while reading the code.

`onFtTargetFileExists` serves both directions and `FileTransferCore` keeps its
running state protected, so the dialog records the direction in the two button
handlers instead. Reaching into the core would have been the larger change.

`DownloadOperation` and `UploadOperation` pass their source and target to
`targetFileExists` in opposite order. The destination path is the same thing in
both directions, so the file name comes from there, through the existing
`File::getName`.

Skip All pressed partway through a batch would otherwise have been ignored for
a matching file, since the pattern check runs before `FileExistDialog` gets to
short-circuit. `isSkipAll` closes that.

Enter was fixed in both dialogs rather than left as a wart. Each has a text box
beside a list under a default OK button, so Enter closed the dialog and threw
away what had been typed. All three boxes now act on the selection instead.

One case remains. Typing into a box and then clicking OK with the mouse still
discards the text, because the click moves focus to the button before the
command arrives. Committing pending text on OK is a different design, and it
would surprise anyone who left a box half edited. The Add button sits beside
the box, so the model stays visible.

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

Narrower buttons made the clipping visible. A push button centres its caption
and clips both ends, so a long name came out as its middle, which identifies
nothing. Names are now measured against the button and cut back to a leading
portion and an ellipsis.

`BS_LEFT` was the one-line alternative. It left-aligns the caption, so clipping
takes from the right and the start survives. It was rejected for saying
nothing about the cut, and for pushing every short name against the left edge
to fix a case that only some names hit.

Shortening a name hides the rest of it, so a shortened button now carries the
whole name on the tooltip already serving the two arrows. Buttons whose names
fit carry none. Every button joins as a tool at startup and is given its text
afterwards, because a tool holding an empty string shows nothing, which saves
adding and removing tools as places come and go.

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

[x] Think about moving the overwrite options into the transfer window
[x] Upload confirmation should also be gated on the overwrite setting

### Phase 5

Built and tested on Windows. All the phase 5 checks pass.

The pattern editor moved across unchanged. It was already self-contained, so
`ConfigurationDialog` gave up every line it had gained and returns to exactly
its pre-phase-3 state, byte for byte. That is worth saying plainly: every
change ever made to that file belonged to the group that moved out.

The upload setting covers the confirmation and nothing else. Uploads still ask
before replacing a remote file, and the overwrite patterns are still downloads
only, so the rule about the file at risk belonging to the other machine
survives intact. The Upload group holds one checkbox and nothing else, which
says the same thing by its shape.

The gear is a real icon rather than a text glyph. The places arrow got away
with U+25BC because the dialog font carries it, but U+2699 is far less likely
to be present, and a missing glyph draws a box. `res/gear.ico` holds 16, 24, 32
and 48 pixel versions, and the button takes the 16 through `BM_SETIMAGE`.
`LoadImage` is asked for that size directly, since `LoadIcon` answers with the
large icon and would leave the button scaling it down.

The gear was drawn on macOS and had never been rendered by Windows until the
build. It reads correctly on a real button face at the default scaling.
