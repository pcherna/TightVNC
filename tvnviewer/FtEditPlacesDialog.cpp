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

#include "FtEditPlacesDialog.h"

#include "client-config-lib/FtHostState.h"

#include "NamingDefs.h"
#include "resource.h"

//
// Removing an entry rebuilds the vector rather than calling erase.
//
// erase shifts the tail down by assignment, and StringStorage::operator =
// returns void, so it is not assignable in the sense the standard containers
// ask for. Rebuilding touches only copy construction, which is well defined.
//

static void eraseCandidate(vector<StringStorage> *candidates, size_t index)
{
  vector<StringStorage> kept;

  for (size_t i = 0; i < candidates->size(); i++) {
    if (i != index) {
      kept.push_back(candidates->at(i));
    }
  }

  candidates->clear();
  for (size_t i = 0; i < kept.size(); i++) {
    candidates->push_back(kept.at(i));
  }
}

static void erasePlace(vector<FtPlace> *places, size_t index)
{
  vector<FtPlace> kept;

  for (size_t i = 0; i < places->size(); i++) {
    if (i != index) {
      kept.push_back(places->at(i));
    }
  }

  places->clear();
  for (size_t i = 0; i < kept.size(); i++) {
    places->push_back(kept.at(i));
  }
}

//
// Exchanges two places, rebuilt through copy construction for the same reason
// erasePlace is. StringStorage assignment returns void, so an FtPlace cannot
// be shifted about by the standard algorithms.
//

static void swapPlaces(vector<FtPlace> *places, size_t a, size_t b)
{
  vector<FtPlace> rebuilt;

  for (size_t i = 0; i < places->size(); i++) {
    size_t take = i;

    if (i == a) {
      take = b;
    } else if (i == b) {
      take = a;
    }
    rebuilt.push_back(places->at(take));
  }

  places->clear();
  for (size_t i = 0; i < rebuilt.size(); i++) {
    places->push_back(rebuilt.at(i));
  }
}

//
// True when two places list exactly the same candidates in the same order.
// Order matters, because it decides which path wins.
//

static bool sameCandidates(const FtPlace *a, const FtPlace *b)
{
  if (a->candidates.size() != b->candidates.size()) {
    return false;
  }
  for (size_t i = 0; i < a->candidates.size(); i++) {
    if (!a->candidates.at(i).isEqualTo(&b->candidates.at(i))) {
      return false;
    }
  }
  return true;
}

FtEditPlacesDialog::FtEditPlacesDialog(Control *parent, bool remote)
: m_remote(remote),
  m_places(RegistryPaths::VIEWER_PATH, remote)
{
  setResourceId(ftclient_editPlacesDialog);
  setParent(parent);
}

FtEditPlacesDialog::~FtEditPlacesDialog()
{
}

BOOL FtEditPlacesDialog::onInitDialog()
{
  initControls();

  m_places.load();
  m_places.copyTo(&m_original);
  m_places.copyTo(&m_working);

  fillPlacesList(m_working.empty() ? -1 : 0);

  return TRUE;
}

void FtEditPlacesDialog::initControls()
{
  HWND hwnd = m_ctrlThis.getWindow();

  m_placesList.setWindow(GetDlgItem(hwnd, IDC_FTEP_PLACES_LIST));
  m_candidatesList.setWindow(GetDlgItem(hwnd, IDC_FTEP_CANDS_LIST));

  m_placeNameBox.setWindow(GetDlgItem(hwnd, IDC_FTEP_PLACE_NAME));
  m_candidatePathBox.setWindow(GetDlgItem(hwnd, IDC_FTEP_CAND_PATH));

  m_addPlaceButton.setWindow(GetDlgItem(hwnd, IDC_FTEP_ADD_PLACE));
  m_renamePlaceButton.setWindow(GetDlgItem(hwnd, IDC_FTEP_RENAME_PLACE));
  m_removePlaceButton.setWindow(GetDlgItem(hwnd, IDC_FTEP_REMOVE_PLACE));
  m_upPlaceButton.setWindow(GetDlgItem(hwnd, IDC_FTEP_UP_PLACE));
  m_downPlaceButton.setWindow(GetDlgItem(hwnd, IDC_FTEP_DOWN_PLACE));

  m_addCandidateButton.setWindow(GetDlgItem(hwnd, IDC_FTEP_ADD_CAND));
  m_replaceCandidateButton.setWindow(GetDlgItem(hwnd, IDC_FTEP_REPLACE_CAND));
  m_removeCandidateButton.setWindow(GetDlgItem(hwnd, IDC_FTEP_REMOVE_CAND));
  m_upCandidateButton.setWindow(GetDlgItem(hwnd, IDC_FTEP_UP_CAND));
  m_downCandidateButton.setWindow(GetDlgItem(hwnd, IDC_FTEP_DOWN_CAND));

  StringStorage caption;
  caption.format(_T("Edit %s Places"),
                 m_remote ? _T("Remote") : _T("Local"));
  m_ctrlThis.setText(caption.getString());
}

BOOL FtEditPlacesDialog::onCommand(UINT controlID, UINT notificationID)
{
  switch (controlID) {
  case IDC_FTEP_PLACES_LIST:
    if (notificationID == LBN_SELCHANGE) {
      onPlaceSelectionChanged();
    }
    break;
  case IDC_FTEP_CANDS_LIST:
    if (notificationID == LBN_SELCHANGE) {
      onCandidateSelectionChanged();
    }
    break;
  case IDC_FTEP_PLACE_NAME:
  case IDC_FTEP_CAND_PATH:
    //
    // Adding takes what is typed in the box beside the list, so the buttons
    // have to follow the text as well as the selection.
    //

    if (notificationID == EN_CHANGE) {
      updateButtons();
    }
    break;
  case IDC_FTEP_ADD_PLACE:
    onAddPlace();
    break;
  case IDC_FTEP_RENAME_PLACE:
    onRenamePlace();
    break;
  case IDC_FTEP_REMOVE_PLACE:
    onRemovePlace();
    break;
  case IDC_FTEP_UP_PLACE:
    onMovePlace(-1);
    break;
  case IDC_FTEP_DOWN_PLACE:
    onMovePlace(1);
    break;
  case IDC_FTEP_ADD_CAND:
    onAddCandidate();
    break;
  case IDC_FTEP_REPLACE_CAND:
    onReplaceCandidate();
    break;
  case IDC_FTEP_REMOVE_CAND:
    onRemoveCandidate();
    break;
  case IDC_FTEP_UP_CAND:
    onMoveCandidate(-1);
    break;
  case IDC_FTEP_DOWN_CAND:
    onMoveCandidate(1);
    break;
  case IDOK:
    if (!onTextBoxEnter()) {
      onOkButtonClick();
    }
    break;
  case IDCANCEL:
    kill(IDCANCEL);
    break;
  }
  return TRUE;
}

int FtEditPlacesDialog::getSelectedPlace()
{
  int index = m_placesList.getSelectedIndex();

  if (index < 0 || static_cast<size_t>(index) >= m_working.size()) {
    return -1;
  }
  return index;
}

void FtEditPlacesDialog::fillPlacesList(int selectIndex)
{
  m_placesList.clear();

  for (size_t i = 0; i < m_working.size(); i++) {
    m_placesList.addString(m_working.at(i).name.getString());
  }

  int count = static_cast<int>(m_working.size());

  if (selectIndex >= count) {
    selectIndex = count - 1;
  }
  if (selectIndex >= 0) {
    m_placesList.setSelectedIndex(selectIndex);
  }

  onPlaceSelectionChanged();
}

void FtEditPlacesDialog::fillCandidatesList(int selectIndex)
{
  m_candidatesList.clear();

  int place = getSelectedPlace();

  if (place >= 0) {
    const vector<StringStorage> *candidates = &m_working.at(place).candidates;

    for (size_t i = 0; i < candidates->size(); i++) {
      m_candidatesList.addString(candidates->at(i).getString());
    }

    int count = static_cast<int>(candidates->size());
    if (selectIndex >= count) {
      selectIndex = count - 1;
    }
    if (selectIndex >= 0) {
      m_candidatesList.setSelectedIndex(selectIndex);
    }
  }

  onCandidateSelectionChanged();
}

void FtEditPlacesDialog::onPlaceSelectionChanged()
{
  int place = getSelectedPlace();

  if (place >= 0) {
    m_placeNameBox.setText(m_working.at(place).name.getString());
  }

  fillCandidatesList(-1);
}

void FtEditPlacesDialog::onCandidateSelectionChanged()
{
  int place = getSelectedPlace();
  int candidate = m_candidatesList.getSelectedIndex();

  if (place >= 0 && candidate >= 0 &&
      static_cast<size_t>(candidate) < m_working.at(place).candidates.size()) {
    m_candidatePathBox.setText(
      m_working.at(place).candidates.at(candidate).getString());
  }

  updateButtons();
}

void FtEditPlacesDialog::updateButtons()
{
  int place = getSelectedPlace();
  bool havePlace = place >= 0;

  int candidate = m_candidatesList.getSelectedIndex();
  bool haveCandidate = havePlace && candidate >= 0;

  int candidateCount = 0;
  if (havePlace) {
    candidateCount = static_cast<int>(m_working.at(place).candidates.size());
  }

  //
  // Add, Rename and Replace all take their input from the box beside the
  // list, so none of them offers itself while that box is empty.
  //

  StringStorage typedName;
  m_placeNameBox.getText(&typedName);

  StringStorage typedPath;
  m_candidatePathBox.getText(&typedPath);

  bool haveName = !typedName.isEmpty();
  bool havePath = !typedPath.isEmpty();

  int placeCount = static_cast<int>(m_working.size());

  m_addPlaceButton.setEnabled(haveName);
  m_renamePlaceButton.setEnabled(havePlace && haveName);
  m_removePlaceButton.setEnabled(havePlace);
  m_upPlaceButton.setEnabled(havePlace && place > 0);
  m_downPlaceButton.setEnabled(havePlace && place < placeCount - 1);

  m_addCandidateButton.setEnabled(havePlace && havePath);
  m_replaceCandidateButton.setEnabled(haveCandidate && havePath);
  m_removeCandidateButton.setEnabled(haveCandidate);
  m_upCandidateButton.setEnabled(haveCandidate && candidate > 0);
  m_downCandidateButton.setEnabled(haveCandidate &&
                                   candidate < candidateCount - 1);
}

bool FtEditPlacesDialog::nameIsTaken(const TCHAR *name, int exceptIndex)
{
  for (size_t i = 0; i < m_working.size(); i++) {
    if (static_cast<int>(i) == exceptIndex) {
      continue;
    }
    if (m_working.at(i).name.isEqualTo(name)) {
      return true;
    }
  }
  return false;
}

void FtEditPlacesDialog::getCandidateInput(StringStorage *out)
{
  StringStorage typed;
  m_candidatePathBox.getText(&typed);

  FtPlaces::normalizePath(&typed, m_remote, out);
}

void FtEditPlacesDialog::getPlaceNameInput(StringStorage *out)
{
  StringStorage typed;
  m_placeNameBox.getText(&typed);

  FtPlaces::normalizeName(&typed, out);
}

void FtEditPlacesDialog::onAddPlace()
{
  StringStorage name;
  getPlaceNameInput(&name);

  //
  // The button is disabled while the box is empty, so this only guards
  // against the two falling out of step.
  //

  if (name.isEmpty()) {
    return;
  }
  if (nameIsTaken(name.getString(), -1)) {
    MessageBox(m_ctrlThis.getWindow(),
               _T("A place of that name already exists."),
               _T("Edit Places"), MB_OK | MB_ICONINFORMATION);
    return;
  }

  FtPlace place;
  place.name.setString(name.getString());
  m_working.push_back(place);

  fillPlacesList(static_cast<int>(m_working.size()) - 1);
}

void FtEditPlacesDialog::onRenamePlace()
{
  int place = getSelectedPlace();
  if (place < 0) {
    return;
  }

  StringStorage name;
  getPlaceNameInput(&name);

  //
  // The button is disabled while the box is empty, so this only guards
  // against the two falling out of step.
  //

  if (name.isEmpty()) {
    return;
  }
  if (nameIsTaken(name.getString(), place)) {
    MessageBox(m_ctrlThis.getWindow(),
               _T("A place of that name already exists."),
               _T("Edit Places"), MB_OK | MB_ICONINFORMATION);
    return;
  }

  m_working.at(place).name.setString(name.getString());

  fillPlacesList(place);
}

void FtEditPlacesDialog::onRemovePlace()
{
  int place = getSelectedPlace();
  if (place < 0) {
    return;
  }

  erasePlace(&m_working, static_cast<size_t>(place));

  fillPlacesList(place);
}

void FtEditPlacesDialog::onMovePlace(int delta)
{
  int place = getSelectedPlace();
  if (place < 0) {
    return;
  }

  int target = place + delta;

  if (target < 0 || target >= static_cast<int>(m_working.size())) {
    return;
  }

  swapPlaces(&m_working, static_cast<size_t>(place),
             static_cast<size_t>(target));

  fillPlacesList(target);
}

void FtEditPlacesDialog::onAddCandidate()
{
  int place = getSelectedPlace();
  if (place < 0) {
    return;
  }

  StringStorage path;
  getCandidateInput(&path);

  if (path.isEmpty()) {
    return;
  }

  m_working.at(place).candidates.push_back(path);

  fillCandidatesList(
    static_cast<int>(m_working.at(place).candidates.size()) - 1);
}

void FtEditPlacesDialog::onReplaceCandidate()
{
  int place = getSelectedPlace();
  int candidate = m_candidatesList.getSelectedIndex();

  if (place < 0 || candidate < 0) {
    return;
  }

  StringStorage path;
  getCandidateInput(&path);

  //
  // The button is disabled while the box is empty, so this only guards
  // against the two falling out of step.
  //

  if (path.isEmpty()) {
    return;
  }

  m_working.at(place).candidates.at(candidate).setString(path.getString());

  fillCandidatesList(candidate);
}

void FtEditPlacesDialog::onRemoveCandidate()
{
  int place = getSelectedPlace();
  int candidate = m_candidatesList.getSelectedIndex();

  if (place < 0 || candidate < 0) {
    return;
  }

  eraseCandidate(&m_working.at(place).candidates,
                 static_cast<size_t>(candidate));

  fillCandidatesList(candidate);
}

void FtEditPlacesDialog::onMoveCandidate(int delta)
{
  int place = getSelectedPlace();
  int candidate = m_candidatesList.getSelectedIndex();

  if (place < 0 || candidate < 0) {
    return;
  }

  vector<StringStorage> *candidates = &m_working.at(place).candidates;
  int target = candidate + delta;

  if (target < 0 || target >= static_cast<int>(candidates->size())) {
    return;
  }

  //
  // Swapped through a copy rather than std::swap, because StringStorage
  // assignment returns void and so is not swappable in the way the standard
  // algorithms expect.
  //

  StringStorage moved(candidates->at(candidate));
  candidates->at(candidate).setString(candidates->at(target).getString());
  candidates->at(target).setString(moved.getString());

  fillCandidatesList(target);
}

void FtEditPlacesDialog::forgetChangedPlaces()
{
  //
  // Only the remote side caches anything, so the local editor has nothing to
  // invalidate.
  //

  if (!m_remote) {
    return;
  }

  for (size_t i = 0; i < m_original.size(); i++) {
    const FtPlace *before = &m_original.at(i);
    bool stillThere = false;

    for (size_t j = 0; j < m_working.size(); j++) {
      const FtPlace *after = &m_working.at(j);

      if (before->name.isEqualTo(&after->name)) {
        stillThere = sameCandidates(before, after);
        break;
      }
    }

    //
    // A place that was removed or renamed leaves its old answer behind under
    // the old name, so drop that too.
    //

    if (!stillThere) {
      FtHostState::forgetPlaceEverywhere(RegistryPaths::VIEWER_PATH,
                                         before->name.getString());
    }
  }
}

bool FtEditPlacesDialog::onTextBoxEnter()
{
  HWND focus = GetFocus();

  if (focus == m_placeNameBox.getWindow()) {
    StringStorage typed;
    m_placeNameBox.getText(&typed);

    if (typed.isEmpty()) {
      return false;
    }

    if (getSelectedPlace() >= 0) {
      onRenamePlace();
    } else {
      onAddPlace();
    }
    return true;
  }

  if (focus == m_candidatePathBox.getWindow()) {
    StringStorage typed;
    m_candidatePathBox.getText(&typed);

    if (typed.isEmpty()) {
      return false;
    }

    //
    // A candidate belongs to a place, so with none selected there is nothing
    // Enter could do. The Add button is disabled in that state too, and
    // swallowing the key keeps the typed path rather than closing on it.
    //

    if (getSelectedPlace() < 0) {
      return true;
    }

    if (m_candidatesList.getSelectedIndex() >= 0) {
      onReplaceCandidate();
    } else {
      onAddCandidate();
    }
    return true;
  }

  return false;
}

bool FtEditPlacesDialog::confirmEmptyPlaces()
{
  int firstEmpty = -1;
  int count = 0;

  StringStorage names;

  for (size_t i = 0; i < m_working.size(); i++) {
    if (!m_working.at(i).candidates.empty()) {
      continue;
    }

    if (firstEmpty < 0) {
      firstEmpty = static_cast<int>(i);
    }
    count++;

    names.appendString(_T("\r\n    "));
    names.appendString(m_working.at(i).name.getString());
  }

  if (count == 0) {
    return true;
  }

  StringStorage message;

  if (count == 1) {
    message.format(_T("This place has no candidate paths, so it will not be ")
                   _T("saved:\r\n%s\r\n\r\nPress Cancel to go back and add a ")
                   _T("path, or OK to save without it."),
                   names.getString());
  } else {
    message.format(_T("These places have no candidate paths, so they will not ")
                   _T("be saved:\r\n%s\r\n\r\nPress Cancel to go back and add ")
                   _T("paths, or OK to save without them."),
                   names.getString());
  }

  //
  // Cancel is the default. A stray Enter on this box would otherwise throw
  // away the very names it is warning about.
  //

  int answer = MessageBox(m_ctrlThis.getWindow(), message.getString(),
                          _T("Edit Places"),
                          MB_OKCANCEL | MB_ICONWARNING | MB_DEFBUTTON2);

  if (answer == IDOK) {
    return true;
  }

  fillPlacesList(firstEmpty);
  m_candidatePathBox.setFocus();

  return false;
}

void FtEditPlacesDialog::onOkButtonClick()
{
  if (!confirmEmptyPlaces()) {
    return;
  }

  forgetChangedPlaces();

  m_places.save(&m_working);

  kill(IDOK);
}
