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

#include <algorithm>
#include <utility>

const TCHAR FtPlaces::ORDER_VALUE[] = _T("Order");

void FtPlaces::normalizePath(const StringStorage *in, bool remote,
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

  //
  // A place with no Order value sorts after every place that has one. Nothing
  // real reaches this order, so it stands in for "unordered" without needing
  // a second flag.
  //

  const int UNORDERED = 0x7fffffff;

  vector<FtPlace> loaded;

  //
  // Sorted as (order, position read), so places that share an order, and the
  // unordered ones as a group, keep the alphabetical order the registry
  // enumeration handed back. Sorting these pairs rather than the places
  // themselves also keeps FtPlace out of an algorithm that would assign it,
  // which StringStorage does not support.
  //

  vector< pair<int, size_t> > sortKeys;

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
      normalizePath(&candidate, m_remote, &normalized);
      place.candidates.push_back(normalized);
    }

    if (place.candidates.empty()) {
      continue;
    }

    int stored = 0;
    int order = placeKey.getValueAsInt32(ORDER_VALUE, &stored) ? stored
                                                               : UNORDERED;

    sortKeys.push_back(make_pair(order, loaded.size()));
    loaded.push_back(place);
  }

  std::sort(sortKeys.begin(), sortKeys.end());

  for (size_t i = 0; i < sortKeys.size(); i++) {
    m_places.push_back(loaded.at(sortKeys.at(i).second));
  }
}

void FtPlaces::save(const vector<FtPlace> *places)
{
  //
  // Every definition is removed and rewritten rather than merged. Merging
  // would have to notice candidates deleted from the end of a list, and the
  // whole set is small enough that rewriting costs nothing.
  //

  size_t existingCount = 0;
  if (m_key.getSubKeyNames(0, &existingCount) && existingCount > 0) {
    vector<StringStorage> existing(existingCount);

    if (m_key.getSubKeyNames(&existing.front(), 0)) {
      for (size_t i = 0; i < existingCount; i++) {
        m_key.deleteSubKeyTree(existing[i].getString());
      }
    }
  }

  StringStorage valueName;

  //
  // Counted over the places actually written rather than over the input, so
  // that a place skipped for having no candidates leaves no gap in the
  // numbering.
  //

  int order = 0;

  for (size_t i = 0; i < places->size(); i++) {
    const FtPlace *place = &places->at(i);

    if (place->name.isEmpty() || place->candidates.empty()) {
      continue;
    }

    RegistryKey placeKey(&m_key, place->name.getString(), true);
    if (!placeKey.isOpened()) {
      continue;
    }

    for (size_t c = 0; c < place->candidates.size(); c++) {
      valueName.format(_T("%u"), (unsigned int)c);
      placeKey.setValueAsString(valueName.getString(),
                                place->candidates.at(c).getString());
    }

    placeKey.setValueAsInt32(ORDER_VALUE, order);
    order++;
  }

  load();
}

size_t FtPlaces::getCount() const
{
  return m_places.size();
}

void FtPlaces::copyTo(vector<FtPlace> *out) const
{
  out->clear();

  for (size_t i = 0; i < m_places.size(); i++) {
    out->push_back(m_places.at(i));
  }
}

const FtPlace *FtPlaces::getPlace(size_t index) const
{
  return &m_places.at(index);
}
