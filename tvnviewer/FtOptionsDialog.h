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

#ifndef _FT_OPTIONS_DIALOG_H_
#define _FT_OPTIONS_DIALOG_H_

#include "client-config-lib/FtAutoOverwrite.h"

#include "gui/BaseDialog.h"
#include "gui/CheckBox.h"
#include "gui/Control.h"
#include "gui/ListBox.h"
#include "gui/TextBox.h"

#include <vector>

using namespace std;

//
// The file transfer settings, on a dialog of their own.
//
// These used to sit in a group at the bottom of the viewer configuration
// dialog, which is reached from the tray icon and is nowhere near a transfer
// in progress. The gear button in the file transfer window opens this
// instead, so the settings are one click from the work they govern.
//
// Everything is edited on a working copy and written only when OK is pressed,
// so Cancel leaves both the registry and the pattern list untouched.
//

class FtOptionsDialog : public BaseDialog
{
public:
  FtOptionsDialog(Control *parent);
  virtual ~FtOptionsDialog();

protected:

  //
  // Inherited from BaseDialog
  //

  virtual BOOL onInitDialog();
  virtual BOOL onCommand(UINT controlID, UINT notificationID);

private:

  void initControls();

  void fillPatternList(int selectIndex);

  //
  // Greys whatever the current selection and the pattern box make
  // meaningless.
  //

  void updatePatternButtons();

  void onPatternSelectionChanged();
  void onAddPattern();
  void onReplacePattern();
  void onRemovePattern();

  //
  // Handles Enter pressed inside the pattern box, and returns true when it
  // did.
  //
  // OK is the default button, so Enter would otherwise close the dialog and
  // throw away whatever was typed. Enter acts on the selection instead: it
  // replaces the highlighted row, or adds a row when none is highlighted.
  // Selecting a row copies it into the box, so select, edit, Enter has to
  // mean replace. Adding there would leave the old row behind next to a
  // near-duplicate.
  //
  // An empty box is left to close the dialog, which is what Enter does
  // everywhere else in it.
  //

  bool onPatternBoxEnter();

  //
  // True when the list already holds this pattern, ignoring the row at
  // exceptIndex so that replacing a row with itself is allowed.
  //
  // Compared without case, because matching ignores case too, so two rows
  // differing only in case would behave identically.
  //

  bool patternIsTaken(const TCHAR *pattern, int exceptIndex);

  void onOkPressed();

  CheckBox m_skipDownloadConfirm;
  CheckBox m_skipUploadConfirm;

  ListBox m_patternList;
  TextBox m_patternBox;

  Control m_addPattern;
  Control m_replacePattern;
  Control m_removePattern;

  FtAutoOverwrite m_autoOverwrite;
  vector<StringStorage> m_workingPatterns;
};

#endif
