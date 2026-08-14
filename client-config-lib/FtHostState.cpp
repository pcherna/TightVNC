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

#include "FtHostState.h"

#include "win-system/Registry.h"

#include <vector>

static const TCHAR LAST_LOCAL_FOLDER_VALUE[]  = _T("FtLastLocalFolder");
static const TCHAR LAST_REMOTE_FOLDER_VALUE[] = _T("FtLastRemoteFolder");

//
// Resolved places go in their own subkey, one value per place, so that
// clearing them all is a single subtree delete and cannot disturb the
// connection options stored alongside them.
//

static const TCHAR RESOLVED_PLACES_SUBKEY[]   = _T("FtResolved");

FtHostState::FtHostState(const TCHAR *registryPath, const TCHAR *hostName)
{
  StringStorage keyName;
  keyName.format(_T("%s\\History\\%s"), registryPath, hostName);

  m_key.open(Registry::getCurrentUserKey(), keyName.getString());
}

FtHostState::~FtHostState()
{
}

bool FtHostState::getLastLocalFolder(StringStorage *out)
{
  return m_key.getValueAsString(LAST_LOCAL_FOLDER_VALUE, out);
}

bool FtHostState::getLastRemoteFolder(StringStorage *out)
{
  return m_key.getValueAsString(LAST_REMOTE_FOLDER_VALUE, out);
}

void FtHostState::setLastLocalFolder(const TCHAR *path)
{
  m_key.setValueAsString(LAST_LOCAL_FOLDER_VALUE, path);
}

void FtHostState::setLastRemoteFolder(const TCHAR *path)
{
  m_key.setValueAsString(LAST_REMOTE_FOLDER_VALUE, path);
}

bool FtHostState::getResolvedPlace(const TCHAR *placeName, StringStorage *out)
{
  RegistryKey resolved(&m_key, RESOLVED_PLACES_SUBKEY, false);

  if (!resolved.isOpened()) {
    return false;
  }
  return resolved.getValueAsString(placeName, out);
}

void FtHostState::setResolvedPlace(const TCHAR *placeName, const TCHAR *path)
{
  RegistryKey resolved(&m_key, RESOLVED_PLACES_SUBKEY, true);

  resolved.setValueAsString(placeName, path);
}

void FtHostState::clearResolvedPlaces()
{
  m_key.deleteSubKeyTree(RESOLVED_PLACES_SUBKEY);
}

void FtHostState::forgetPlaceEverywhere(const TCHAR *registryPath,
                                        const TCHAR *placeName)
{
  StringStorage historyPath;
  historyPath.format(_T("%s\\History"), registryPath);

  RegistryKey history(Registry::getCurrentUserKey(),
                      historyPath.getString(), false);
  if (!history.isOpened()) {
    return;
  }

  size_t hostCount = 0;
  if (!history.getSubKeyNames(0, &hostCount) || hostCount == 0) {
    return;
  }

  std::vector<StringStorage> hosts(hostCount);
  if (!history.getSubKeyNames(&hosts.front(), 0)) {
    return;
  }

  for (size_t i = 0; i < hostCount; i++) {
    RegistryKey hostKey(&history, hosts[i].getString(), false);
    if (!hostKey.isOpened()) {
      continue;
    }

    RegistryKey resolved(&hostKey, RESOLVED_PLACES_SUBKEY, false);
    if (!resolved.isOpened()) {
      continue;
    }
    resolved.deleteValue(placeName);
  }
}
