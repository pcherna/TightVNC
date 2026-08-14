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
//   0 = C:/ProgramData/Acme/logs
//   1 = C:/Acme/logs
//   2 = D:/Acme/logs
//
// Reading a place stops at the first gap in the numbering. Places come back
// in registry enumeration order, which is alphabetical by name.
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

  void load();

  size_t getCount() const;

  //
  // Place at the given index, which must be below getCount().
  //

  const FtPlace *getPlace(size_t index) const;

private:
  RegistryKey m_key;
  vector<FtPlace> m_places;

  //
  // Stops a runaway read if the registry holds something unexpected. No real
  // place needs anywhere near this many candidates.
  //

  static const size_t MAX_CANDIDATES = 64;
};

#endif
