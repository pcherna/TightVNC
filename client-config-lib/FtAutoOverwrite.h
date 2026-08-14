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

#ifndef _FT_AUTO_OVERWRITE_H_
#define _FT_AUTO_OVERWRITE_H_

#include "win-system/RegistryKey.h"

#include <vector>

using namespace std;

//
// Filename patterns whose downloads overwrite an existing local file without
// asking.
//
// Re-downloading the same log or the same dated statement always ends in
// pressing Overwrite, so the six-button conflict dialog carries no
// information for those files. A pattern here answers for them.
//
// The list is global rather than per host, and lives under
//
//   <registryPath>\FtAutoOverwrite\
//
// with the patterns stored as numbered values, the shape ConnectionHistory
// and FtPlaces already use:
//
//   0 = *.log
//   1 = bill_202*.*
//
// Reading stops at the first gap in the numbering.
//
// Patterns apply to downloads only. An upload that would replace a remote
// file still asks, because the file at risk then belongs to the other
// machine.
//

class FtAutoOverwrite
{
public:
  //
  // Parameters:
  // registryPath - viewer registry path, for example
  //                "Software\TightVNC\Viewer".
  //

  FtAutoOverwrite(const TCHAR *registryPath);
  virtual ~FtAutoOverwrite();

  //
  // Most patterns that will be read or written.
  //
  // Stops a runaway read if the registry holds something unexpected, and
  // bounds the sweep that clears leftovers on save. Public so that an editor
  // can refuse a pattern that saving would drop.
  //

  static const size_t MAX_PATTERNS = 64;

  //
  // Rereads every pattern from the registry, discarding what was held.
  //
  // Empty entries are skipped. An empty pattern matches nothing, so keeping
  // one would only be a row that does nothing.
  //

  void load();

  //
  // Replaces everything stored with the given list, then rereads it.
  //
  // Patterns are renumbered from zero and anything left over from a longer
  // previous list is deleted, so a removed pattern cannot survive as a stale
  // value.
  //

  void save(const vector<StringStorage> *patterns);

  size_t getCount() const;

  //
  // Pattern at the given index, which must be below getCount().
  //

  const StringStorage *getPattern(size_t index) const;

  //
  // Copies the patterns out for editing.
  //

  void copyTo(vector<StringStorage> *out) const;

  //
  // True when the given file name matches any stored pattern.
  //
  // Pass the bare file name, not a path. Patterns describe file names, so a
  // path would only ever match one containing a separator.
  //

  bool matchesAny(const TCHAR *fileName) const;

  //
  // True when the given file name matches one pattern.
  //
  // '*' stands for any run of characters including none, '?' for exactly one,
  // and every other character for itself. Matching ignores case, as the
  // Windows file system does.
  //
  // A dot carries no special meaning, so "*.log" wants a name ending in
  // ".log" and "*.*" wants a name containing a dot. This differs from
  // PathMatchSpec, which inherits the DOS rule that "*.*" also matches a name
  // with no extension at all.
  //
  // Exposed so that an editor can show what a typed pattern would catch.
  //

  static bool matches(const TCHAR *fileName, const TCHAR *pattern);

private:
  RegistryKey m_key;
  vector<StringStorage> m_patterns;
};

#endif
