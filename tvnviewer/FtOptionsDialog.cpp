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

#include "FtOptionsDialog.h"

#include "client-config-lib/ViewerConfig.h"
#include "client-config-lib/ViewerSettingsManager.h"

#include "NamingDefs.h"
#include "resource.h"

//
// Removing an entry rebuilds the vector rather than calling erase.
//
// erase shifts the tail down by assignment, and StringStorage::operator =
// returns void, so it is not assignable in the sense the standard containers
// ask for. Rebuilding touches only copy construction, which is well defined.
//

static void erasePattern(vector<StringStorage> *patterns, size_t index)
{
  vector<StringStorage> kept;

  for (size_t i = 0; i < patterns->size(); i++) {
    if (i != index) {
      kept.push_back(patterns->at(i));
    }
  }

  patterns->clear();
  for (size_t i = 0; i < kept.size(); i++) {
    patterns->push_back(kept.at(i));
  }
}

FtOptionsDialog::FtOptionsDialog(Control *parent)
: m_autoOverwrite(RegistryPaths::VIEWER_PATH)
{
  setResourceId(ftclient_optionsDialog);
  setParent(parent);
}

FtOptionsDialog::~FtOptionsDialog()
{
}

BOOL FtOptionsDialog::onInitDialog()
{
  initControls();

  ViewerConfig *config = ViewerConfig::getInstance();

  m_skipDownloadConfirm.check(config->isDownloadConfirmationSkipped());
  m_skipUploadConfirm.check(config->isUploadConfirmationSkipped());

  m_autoOverwrite.load();
  m_autoOverwrite.copyTo(&m_workingPatterns);

  fillPatternList(-1);

  return TRUE;
}

void FtOptionsDialog::initControls()
{
  HWND hwnd = m_ctrlThis.getWindow();

  m_skipDownloadConfirm.setWindow(GetDlgItem(hwnd, IDC_FTO_SKIP_DOWNLOAD));
  m_skipUploadConfirm.setWindow(GetDlgItem(hwnd, IDC_FTO_SKIP_UPLOAD));

  m_patternList.setWindow(GetDlgItem(hwnd, IDC_FTO_PATTERN_LIST));
  m_patternBox.setWindow(GetDlgItem(hwnd, IDC_FTO_PATTERN_EDIT));

  m_addPattern.setWindow(GetDlgItem(hwnd, IDC_FTO_ADD_PATTERN));
  m_replacePattern.setWindow(GetDlgItem(hwnd, IDC_FTO_REPLACE_PATTERN));
  m_removePattern.setWindow(GetDlgItem(hwnd, IDC_FTO_REMOVE_PATTERN));
}

BOOL FtOptionsDialog::onCommand(UINT controlID, UINT notificationID)
{
  switch (controlID) {
  case IDC_FTO_PATTERN_LIST:
    if (notificationID == LBN_SELCHANGE) {
      onPatternSelectionChanged();
    }
    break;
  case IDC_FTO_PATTERN_EDIT:
    //
    // Adding takes what is typed in the box beside the list, so the buttons
    // have to follow the text as well as the selection.
    //

    if (notificationID == EN_CHANGE) {
      updatePatternButtons();
    }
    break;
  case IDC_FTO_ADD_PATTERN:
    onAddPattern();
    break;
  case IDC_FTO_REPLACE_PATTERN:
    onReplacePattern();
    break;
  case IDC_FTO_REMOVE_PATTERN:
    onRemovePattern();
    break;
  case IDOK:
    if (!onPatternBoxEnter()) {
      onOkPressed();
      kill(IDOK);
    }
    break;
  case IDCANCEL:
    kill(IDCANCEL);
    break;
  }
  return TRUE;
}

void FtOptionsDialog::fillPatternList(int selectIndex)
{
  m_patternList.clear();

  for (size_t i = 0; i < m_workingPatterns.size(); i++) {
    m_patternList.addString(m_workingPatterns.at(i).getString());
  }

  int count = static_cast<int>(m_workingPatterns.size());

  if (selectIndex >= count) {
    selectIndex = count - 1;
  }
  if (selectIndex >= 0) {
    m_patternList.setSelectedIndex(selectIndex);
  }

  onPatternSelectionChanged();
}

void FtOptionsDialog::onPatternSelectionChanged()
{
  int index = m_patternList.getSelectedIndex();

  if (index >= 0 && static_cast<size_t>(index) < m_workingPatterns.size()) {
    m_patternBox.setText(m_workingPatterns.at(index).getString());
  }

  updatePatternButtons();
}

void FtOptionsDialog::updatePatternButtons()
{
  int index = m_patternList.getSelectedIndex();
  bool haveSelection = index >= 0 &&
                       static_cast<size_t>(index) < m_workingPatterns.size();

  StringStorage typed;
  m_patternBox.getText(&typed);
  bool havePattern = !typed.isEmpty();

  m_addPattern.setEnabled(havePattern);
  m_replacePattern.setEnabled(haveSelection && havePattern);
  m_removePattern.setEnabled(haveSelection);
}

bool FtOptionsDialog::patternIsTaken(const TCHAR *pattern, int exceptIndex)
{
  StringStorage wanted(pattern);
  wanted.toLowerCase();

  for (size_t i = 0; i < m_workingPatterns.size(); i++) {
    if (static_cast<int>(i) == exceptIndex) {
      continue;
    }

    StringStorage held(m_workingPatterns.at(i));
    held.toLowerCase();

    if (held.isEqualTo(&wanted)) {
      return true;
    }
  }
  return false;
}

void FtOptionsDialog::onAddPattern()
{
  StringStorage pattern;
  m_patternBox.getText(&pattern);

  //
  // The button is disabled while the box is empty, so this only guards
  // against the two falling out of step.
  //

  if (pattern.isEmpty()) {
    return;
  }
  if (patternIsTaken(pattern.getString(), -1)) {
    MessageBox(m_ctrlThis.getWindow(),
               _T("That pattern is already in the list."),
               _T("Transfer Options"), MB_OK | MB_ICONINFORMATION);
    return;
  }

  //
  // Refused here rather than dropped on save, so that a pattern which will
  // not be kept is never shown as if it had been.
  //

  if (m_workingPatterns.size() >= FtAutoOverwrite::MAX_PATTERNS) {
    StringStorage message;
    message.format(_T("The list holds at most %u patterns."),
                   (unsigned int)FtAutoOverwrite::MAX_PATTERNS);

    MessageBox(m_ctrlThis.getWindow(), message.getString(),
               _T("Transfer Options"), MB_OK | MB_ICONINFORMATION);
    return;
  }

  m_workingPatterns.push_back(pattern);

  fillPatternList(static_cast<int>(m_workingPatterns.size()) - 1);
}

void FtOptionsDialog::onReplacePattern()
{
  int index = m_patternList.getSelectedIndex();

  if (index < 0 || static_cast<size_t>(index) >= m_workingPatterns.size()) {
    return;
  }

  StringStorage pattern;
  m_patternBox.getText(&pattern);

  if (pattern.isEmpty()) {
    return;
  }
  if (patternIsTaken(pattern.getString(), index)) {
    MessageBox(m_ctrlThis.getWindow(),
               _T("That pattern is already in the list."),
               _T("Transfer Options"), MB_OK | MB_ICONINFORMATION);
    return;
  }

  m_workingPatterns.at(index).setString(pattern.getString());

  fillPatternList(index);
}

void FtOptionsDialog::onRemovePattern()
{
  int index = m_patternList.getSelectedIndex();

  if (index < 0 || static_cast<size_t>(index) >= m_workingPatterns.size()) {
    return;
  }

  erasePattern(&m_workingPatterns, static_cast<size_t>(index));

  fillPatternList(index);
}

bool FtOptionsDialog::onPatternBoxEnter()
{
  if (GetFocus() != m_patternBox.getWindow()) {
    return false;
  }

  StringStorage typed;
  m_patternBox.getText(&typed);

  if (typed.isEmpty()) {
    return false;
  }

  int index = m_patternList.getSelectedIndex();

  if (index >= 0 && static_cast<size_t>(index) < m_workingPatterns.size()) {
    onReplacePattern();
  } else {
    onAddPattern();
  }

  //
  // Consumed even when the action refused the pattern, a duplicate for
  // instance. Closing the dialog on the back of a refusal would be the same
  // loss this exists to prevent.
  //

  return true;
}

void FtOptionsDialog::onOkPressed()
{
  ViewerConfig *config = ViewerConfig::getInstance();

  config->skipDownloadConfirmation(m_skipDownloadConfirm.isChecked());
  config->skipUploadConfirmation(m_skipUploadConfirm.isChecked());

  SettingsManager *sm = ViewerSettingsManager::getInstance();
  config->saveToStorage(sm);

  //
  // The patterns live in their own registry key rather than in ViewerConfig,
  // because a list needs numbered values and SettingsManager offers no way to
  // store one.
  //

  m_autoOverwrite.save(&m_workingPatterns);
}
