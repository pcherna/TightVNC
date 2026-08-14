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

#ifndef _FT_HOST_STATE_H_
#define _FT_HOST_STATE_H_

#include "win-system/RegistryKey.h"

//
// Per-host state of the file transfer dialog.
//
// Remembers the folder each pane was last showing, so that reconnecting to
// the same server returns the user to where they were.
//
// State is stored beside the connection options that ConnectionConfigSM
// writes, under <registryPath>\History\<host>.
//

class FtHostState
{
public:
  //
  // Opens the per-host registry entry, creating it if it does not exist.
  //
  // Parameters:
  // registryPath - viewer registry path, for example
  //                "Software\TightVNC\Viewer".
  // hostName - server host as the user typed it.
  //

  FtHostState(const TCHAR *registryPath, const TCHAR *hostName);
  virtual ~FtHostState();

  //
  // Folder each pane was last showing.
  //
  // Returns false if nothing has been stored yet, leaving out untouched.
  //
  // Remark: an empty string is a valid stored value. It is the local pane's
  // "My Computer" root, so callers must not treat empty as unset.
  //

  bool getLastLocalFolder(StringStorage *out);
  bool getLastRemoteFolder(StringStorage *out);

  void setLastLocalFolder(const TCHAR *path);
  void setLastRemoteFolder(const TCHAR *path);

  //
  // Where a named place resolved to on this host.
  //
  // Resolving walks candidate paths, and on the remote side each one costs a
  // round trip. The answer is stable once found, so it is kept here and
  // reused. Returns false when this place has not been resolved yet.
  //

  bool getResolvedPlace(const TCHAR *placeName, StringStorage *out);
  void setResolvedPlace(const TCHAR *placeName, const TCHAR *path);

  //
  // Throws away every cached resolution for this host, so the next use of
  // each place hunts again. This is what the Rescan menu item does.
  //

  void clearResolvedPlaces();

private:
  RegistryKey m_key;
};

#endif
