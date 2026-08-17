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

#ifndef _FT_PLACES_H_
#define _FT_PLACES_H_

#include "win-system/RegistryKey.h"

#include <vector>

using namespace std;

//
// One named location and the candidate paths it can resolve to.
//
// Order matters. The first candidate that exists wins.
//

struct FtPlace
{
  StringStorage name;
  vector<StringStorage> candidates;
};

//
// Named locations for one pane of the file transfer dialog.
//
// A place lets a single name such as "Log Folder" cover machines that keep
// the same thing in different directories. Picking it walks the candidates
// in order and stops at the first that exists.
//
// Definitions are global rather than per host, and live under
//
//   <registryPath>\FtPlaces\<Local|Remote>\<place name>\
//
// with the candidates stored as numbered values, the shape ConnectionHistory
// already uses:
//
//   0     = C:/ProgramData/Acme/logs
//   1     = C:/Acme/logs
//   2     = D:/Acme/logs
//   Order = 0
//
// Reading a place stops at the first gap in the numbering. The candidate
// values are numbers and the order value is a word, so the two cannot
// collide.
//
// Places come back in the order given by their Order value, lowest first.
// Order is what the dialog puts on its buttons: the first three places get a
// button each and the rest go in the menu, so the user has to be able to say
// which three those are. Registry enumeration is alphabetical, which is not
// something anyone chose.
//
// A place with no Order value sorts after every place that has one, keeping
// its alphabetical position among the others. A place added to the registry
// by hand therefore appears at the end rather than displacing a button.
//
// Candidates may be written with either slash. Loading rewrites them to the
// separator the pane actually uses, forward for remote and backward for
// local, so whoever edits the registry does not have to remember which side
// wants which.
//

class FtPlaces
{
public:
  //
  // Parameters:
  // registryPath - viewer registry path, for example
  //                "Software\TightVNC\Viewer".
  // remote - true for the remote pane's places, false for the local pane's.
  //

  FtPlaces(const TCHAR *registryPath, bool remote);
  virtual ~FtPlaces();

  //
  // Rereads every place from the registry, discarding what was held.
  //
  // Places with no candidates are skipped. They could never resolve, so
  // offering them would only produce a menu entry that always fails.
  //
  // The result is sorted by the Order value, so getPlace(0) is the place the
  // user put first.
  //

  void load();

  //
  // Replaces everything stored for this side with the given list, then
  // rereads it.
  //
  // The position of a place in the list is its order, and is written out as
  // the Order value. Reordering the list is therefore all the editor has to
  // do to reorder the buttons.
  //
  // Existing definitions are removed first, so a place dropped from the list
  // disappears from the registry and no stale candidate numbering survives.
  //

  void save(const vector<FtPlace> *places);

  size_t getCount() const;

  //
  // Place at the given index, which must be below getCount().
  //

  const FtPlace *getPlace(size_t index) const;

  //
  // Copies the places out for editing.
  //

  void copyTo(vector<FtPlace> *out) const;

  //
  // Rewrites every path separator to the one the given side uses, forward
  // for remote and backward for local.
  //
  // Exposed so that an editor can normalise what someone types at the point
  // they type it, rather than leaving it to look different after a reload.
  //

  static void normalizePath(const StringStorage *in, bool remote,
                            StringStorage *out);

  //
  // Rewrites a place name into what can be a registry key.
  //
  // A backslash becomes a forward slash. The registry API reads a backslash
  // in a key name as a path separator and has no way to escape it, so a place
  // called "Support\BCF" would silently become a key "Support" holding a key
  // "BCF", and neither would carry any candidates. Forward slash is legal in
  // a key name and reads the same way to a person.
  //
  // Applied where the name is typed rather than only where it is saved, so
  // that one spelling reaches the list, the duplicate check, and the
  // per-host resolved-answer cache, which is keyed by name.
  //
  // in and out must be different objects.
  //

  static void normalizeName(const StringStorage *in, StringStorage *out);

private:
  RegistryKey m_key;
  vector<FtPlace> m_places;
  bool m_remote;

  //
  // Stops a runaway read if the registry holds something unexpected. No real
  // place needs anywhere near this many candidates.
  //

  static const size_t MAX_CANDIDATES = 64;

  //
  // Name of the value holding a place's position in the list.
  //

  static const TCHAR ORDER_VALUE[];
};

#endif
