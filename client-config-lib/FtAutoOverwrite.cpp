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

#include "FtAutoOverwrite.h"

#include "win-system/Registry.h"

//
// Matches a name against a pattern, both already lowered.
//
// Written with two cursors and a remembered star rather than recursively, so
// that a pattern full of stars cannot drive the stack down. On a mismatch the
// last star gives up one more character and matching resumes from there.
//

static bool globMatch(const TCHAR *name, const TCHAR *pattern)
{
  const TCHAR *starPattern = 0;
  const TCHAR *starName = 0;

  while (*name != _T('\0')) {
    if (*pattern == _T('?') || *pattern == *name) {
      name++;
      pattern++;
    } else if (*pattern == _T('*')) {
      starPattern = pattern++;
      starName = name;
    } else if (starPattern != 0) {
      pattern = starPattern + 1;
      name = ++starName;
    } else {
      return false;
    }
  }

  //
  // Trailing stars are free, anything else left in the pattern still wants a
  // character that the name does not have.
  //

  while (*pattern == _T('*')) {
    pattern++;
  }

  return *pattern == _T('\0');
}

bool FtAutoOverwrite::matches(const TCHAR *fileName, const TCHAR *pattern)
{
  //
  // Lowered once here rather than compared character by character, because
  // the comparison happens inside a backtracking loop that can revisit the
  // same character many times.
  //

  StringStorage loweredName(fileName);
  StringStorage loweredPattern(pattern);

  loweredName.toLowerCase();
  loweredPattern.toLowerCase();

  return globMatch(loweredName.getString(), loweredPattern.getString());
}

FtAutoOverwrite::FtAutoOverwrite(const TCHAR *registryPath)
{
  StringStorage keyName;
  keyName.format(_T("%s\\FtAutoOverwrite"), registryPath);

  m_key.open(Registry::getCurrentUserKey(), keyName.getString());
}

FtAutoOverwrite::~FtAutoOverwrite()
{
}

void FtAutoOverwrite::load()
{
  m_patterns.clear();

  StringStorage valueName;
  StringStorage pattern;

  for (size_t i = 0; i < MAX_PATTERNS; i++) {
    valueName.format(_T("%u"), (unsigned int)i);
    if (!m_key.getValueAsString(valueName.getString(), &pattern)) {
      break;
    }
    if (!pattern.isEmpty()) {
      m_patterns.push_back(pattern);
    }
  }
}

void FtAutoOverwrite::save(const vector<StringStorage> *patterns)
{
  StringStorage valueName;
  size_t written = 0;

  for (size_t i = 0; i < patterns->size() && written < MAX_PATTERNS; i++) {
    const StringStorage *pattern = &patterns->at(i);

    if (pattern->isEmpty()) {
      continue;
    }

    valueName.format(_T("%u"), (unsigned int)written);
    m_key.setValueAsString(valueName.getString(), pattern->getString());
    written++;
  }

  //
  // A shorter list than last time would otherwise leave the tail of the old
  // one in place, and load stops at the first gap rather than at the first
  // deleted value, so the leftovers have to go.
  //

  for (size_t i = written; i < MAX_PATTERNS; i++) {
    valueName.format(_T("%u"), (unsigned int)i);
    m_key.deleteValue(valueName.getString());
  }

  load();
}

size_t FtAutoOverwrite::getCount() const
{
  return m_patterns.size();
}

const StringStorage *FtAutoOverwrite::getPattern(size_t index) const
{
  return &m_patterns.at(index);
}

void FtAutoOverwrite::copyTo(vector<StringStorage> *out) const
{
  out->clear();

  for (size_t i = 0; i < m_patterns.size(); i++) {
    out->push_back(m_patterns.at(i));
  }
}

bool FtAutoOverwrite::matchesAny(const TCHAR *fileName) const
{
  for (size_t i = 0; i < m_patterns.size(); i++) {
    if (matches(fileName, m_patterns.at(i).getString())) {
      return true;
    }
  }
  return false;
}
