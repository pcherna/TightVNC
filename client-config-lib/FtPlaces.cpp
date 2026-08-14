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

//
// Rewrites every path separator to the one the pane uses, so that a
// hand-edited registry entry works whichever slash was typed.
//

static void normalizeSeparators(const StringStorage *in, bool remote,
                                StringStorage *out)
{
  const TCHAR wanted = remote ? _T('/') : _T('\\');
  const TCHAR *chars = in->getString();
  size_t length = in->getLength();

  out->setString(_T(""));

  for (size_t i = 0; i < length; i++) {
    TCHAR c = chars[i];

    if (c == _T('/') || c == _T('\\')) {
      c = wanted;
    }
    out->appendChar(c);
  }
}

FtPlaces::FtPlaces(const TCHAR *registryPath, bool remote)
: m_remote(remote)
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
    StringStorage normalized;

    for (size_t c = 0; c < MAX_CANDIDATES; c++) {
      valueName.format(_T("%u"), (unsigned int)c);
      if (!placeKey.getValueAsString(valueName.getString(), &candidate)) {
        break;
      }
      normalizeSeparators(&candidate, m_remote, &normalized);
      place.candidates.push_back(normalized);
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
