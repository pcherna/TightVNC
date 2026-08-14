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

#include "FtPlaces.h"

#include "win-system/Registry.h"

FtPlaces::FtPlaces(const TCHAR *registryPath, bool remote)
{
  StringStorage keyName;
  keyName.format(_T("%s\\FtPlaces\\%s"),
                 registryPath,
                 remote ? _T("Remote") : _T("Local"));

  m_key.open(Registry::getCurrentUserKey(), keyName.getString());
}

FtPlaces::~FtPlaces()
{
}

void FtPlaces::load()
{
  m_places.clear();

  size_t count = 0;
  if (!m_key.getSubKeyNames(0, &count) || count == 0) {
    return;
  }

  vector<StringStorage> names(count);
  if (!m_key.getSubKeyNames(&names.front(), 0)) {
    return;
  }

  for (size_t i = 0; i < count; i++) {
    RegistryKey placeKey(&m_key, names[i].getString(), false);
    if (!placeKey.isOpened()) {
      continue;
    }

    FtPlace place;
    place.name.setString(names[i].getString());

    StringStorage valueName;
    StringStorage candidate;

    for (size_t c = 0; c < MAX_CANDIDATES; c++) {
      valueName.format(_T("%u"), (unsigned int)c);
      if (!placeKey.getValueAsString(valueName.getString(), &candidate)) {
        break;
      }
      place.candidates.push_back(candidate);
    }

    if (!place.candidates.empty()) {
      m_places.push_back(place);
    }
  }
}

size_t FtPlaces::getCount() const
{
  return m_places.size();
}

const FtPlace *FtPlaces::getPlace(size_t index) const
{
  return &m_places.at(index);
}
