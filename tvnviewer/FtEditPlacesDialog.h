// Copyright (C) 2026 Peter Cherna.
// All rights reserved.
//
//-------------------------------------------------------------------------
// This file is part of the TightVNC software.  Please visit our Web site:
//
//                       http://www.tightvnc.com/
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//-------------------------------------------------------------------------
//

#ifndef _FT_EDIT_PLACES_DIALOG_H_
#define _FT_EDIT_PLACES_DIALOG_H_

#include "gui/BaseDialog.h"
#include "gui/Control.h"
#include "gui/ListBox.h"
#include "gui/TextBox.h"

#include "client-config-lib/FtPlaces.h"

#include <vector>

using namespace std;

//
// Editor for one pane's named places.
//
// Edits a working copy and writes nothing until OK. Cancel therefore leaves
// the registry exactly as it was.
//
// The order of the places list is the order they appear in, and the first few
// of them get a button of their own in the file transfer dialog. Up and Down
// are how a place reaches a button.
//
// Saving also drops the cached resolution of every place whose candidates
// changed, on every host, so that an edit reaches the machines already
// resolved rather than only the ones never visited.
//

class FtEditPlacesDialog : public BaseDialog
{
public:
  //
  // Parameters:
  // parent - dialog to sit over.
  // remote - true to edit the remote pane's places, false for the local
  //          pane's. Decides the path separator and whether cached
  //          resolutions need dropping, since only remote answers are cached.
  //

  FtEditPlacesDialog(Control *parent, bool remote);
  virtual ~FtEditPlacesDialog();

protected:

  //
  // Inherited from BaseDialog
  //

  virtual BOOL onInitDialog();
  virtual BOOL onCommand(UINT controlID, UINT notificationID);

private:

  void initControls();

  //
  // Refills a list from the working copy and selects the given row, or the
  // nearest one that exists.
  //

  void fillPlacesList(int selectIndex);
  void fillCandidatesList(int selectIndex);

  //
  // Greys whatever the current selection makes meaningless.
  //

  void updateButtons();

  void onPlaceSelectionChanged();
  void onCandidateSelectionChanged();

  void onAddPlace();
  void onRenamePlace();
  void onRemovePlace();

  //
  // Moves the selected place by one row. The order of this list is the order
  // the places appear in, and the first few of them get a button of their own
  // in the file transfer dialog, so this is how a place is promoted onto the
  // toolbar.
  //

  void onMovePlace(int delta);

  void onAddCandidate();
  void onReplaceCandidate();
  void onRemoveCandidate();

  //
  // Moves the selected candidate by one row. Order decides which path wins,
  // so this is how a preferred layout is promoted.
  //

  void onMoveCandidate(int delta);

  //
  // Handles Enter pressed inside either text box, and returns true when it
  // did.
  //
  // OK is the default button, so Enter would otherwise close the dialog and
  // throw away whatever was typed. Enter acts on the selection instead: it
  // renames or replaces the highlighted row, or adds a row when none is
  // highlighted. Selecting a row copies it into the box, so select, edit,
  // Enter has to mean rename or replace. Adding there would leave the old row
  // behind next to a near-duplicate.
  //
  // An empty box is left to close the dialog, which is what Enter does
  // everywhere else in it.
  //

  bool onTextBoxEnter();

  void onOkButtonClick();

  //
  // Index of the place being edited, or -1 when none is selected.
  //

  int getSelectedPlace();

  //
  // True when a place of this name already exists, ignoring the row at
  // exceptIndex so that renaming a place to its own name is allowed.
  //

  bool nameIsTaken(const TCHAR *name, int exceptIndex);

  //
  // Reads the path box, normalised to the separator this side uses.
  //

  void getCandidateInput(StringStorage *out);

  //
  // Reads the name box, normalised to what can be a registry key. A
  // backslash comes back as a forward slash.
  //

  void getPlaceNameInput(StringStorage *out);

  //
  // Drops the cached resolution of every place whose candidates no longer
  // match what was loaded, and of every place that was removed or renamed.
  //

  void forgetChangedPlaces();

private:
  bool m_remote;

  FtPlaces m_places;
  vector<FtPlace> m_original;
  vector<FtPlace> m_working;

  ListBox m_placesList;
  ListBox m_candidatesList;

  TextBox m_placeNameBox;
  TextBox m_candidatePathBox;

  Control m_addPlaceButton;
  Control m_renamePlaceButton;
  Control m_removePlaceButton;
  Control m_upPlaceButton;
  Control m_downPlaceButton;

  Control m_addCandidateButton;
  Control m_replaceCandidateButton;
  Control m_removeCandidateButton;
  Control m_upCandidateButton;
  Control m_downCandidateButton;
};

#endif
