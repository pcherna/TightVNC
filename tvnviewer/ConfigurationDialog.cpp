// Copyright (C) 2012 GlavSoft LLC.
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

#include "ConfigurationDialog.h"
#include "NamingDefs.h"
#include "TvnViewer.h"
#include "resource.h"

#include "file-lib/File.h"
#include "win-system/Process.h"

//
// Removing an entry rebuilds the vector rather than calling erase.
//
// erase shifts the tail down by assignment, and StringStorage::operator =
// returns void, so it is not assignable in the sense the standard containers
// ask for. Rebuilding touches only copy construction, which is well defined.
//

static void erasePattern(vector<StringStorage> *patterns, size_t index)
{
  vector<StringStorage> kept;

  for (size_t i = 0; i < patterns->size(); i++) {
    if (i != index) {
      kept.push_back(patterns->at(i));
    }
  }

  patterns->clear();
  for (size_t i = 0; i < kept.size(); i++) {
    patterns->push_back(kept.at(i));
  }
}

ConfigurationDialog::ConfigurationDialog()
: BaseDialog(IDD_CONFIGURATION),
  m_application(0),
  m_autoOverwrite(RegistryPaths::VIEWER_PATH)
{
}

void ConfigurationDialog::setListenerOfUpdate(WindowsApplication *application)
{
  m_application = application;
}

BOOL ConfigurationDialog::onCommand(UINT controlID, UINT notificationID)
{
  if (controlID == IDC_EVERBLVL) {
    if (notificationID == EN_CHANGE) {
      onLogLevelChange();
    }
  }
  if (controlID == IDOK) {
    onOkPressed();
    if (m_application != 0) {
      m_application->postMessage(TvnViewer::WM_USER_CONFIGURATION_RELOAD);
    }
    kill(1);
    return TRUE;
  }
  if (controlID == IDCANCEL) {
    kill(0);
    return TRUE;
  }
  if (controlID == IDC_BCLEAR_LIST) {
    ViewerConfig::getInstance()->getConnectionHistory()->clear();
  }
  if (controlID == IDC_OPEN_LOG_FOLDER_BUTTON) {
    onOpenFolderButtonClick();
  }
  if (controlID == IDC_CFT_PATTERN_LIST) {
    if (notificationID == LBN_SELCHANGE) {
      onPatternSelectionChanged();
    }
  }
  if (controlID == IDC_CFT_PATTERN_EDIT) {
    //
    // Add and Replace both take what is typed in the box, so the buttons have
    // to follow the text as well as the selection.
    //

    if (notificationID == EN_CHANGE) {
      updatePatternButtons();
    }
  }
  if (controlID == IDC_CFT_ADD_PATTERN) {
    onAddPattern();
  }
  if (controlID == IDC_CFT_REPLACE_PATTERN) {
    onReplacePattern();
  }
  if (controlID == IDC_CFT_REMOVE_PATTERN) {
    onRemovePattern();
  }
  return FALSE;
}

void ConfigurationDialog::onLogLevelChange()
{
  StringStorage text;
  int logLevel;
  m_verbLvl.getText(&text);
  StringParser::parseInt(text.getString(), &logLevel);
  if (logLevel != 0) {
    m_logging.setEnabled(true);

    // If log-file is exist, then enable button "Locate...", else disable him.
    StringStorage logDir;
    ViewerConfig::getInstance()->getLogDir(&logDir);
    StringStorage logFileName;
    logFileName.format(_T("%s\\%s.log"),
                       logDir.getString(),
                       LogNames::VIEWER_LOG_FILE_STUB_NAME);

    File logFile(logFileName.getString());
    if (logFile.exists()) {
      m_openLogDir.setEnabled(true);
    } else {
      m_openLogDir.setEnabled(false);
    }
  } else {
    m_logging.setEnabled(false);
    m_openLogDir.setEnabled(false);
  }
}
void ConfigurationDialog::onOpenFolderButtonClick()
{
  StringStorage logDir;
  
  ViewerConfig::getInstance()->getLogDir(&logDir);

  StringStorage command;
  command.format(_T("explorer /select,%s\\%s.log"),
                 logDir.getString(),
                 LogNames::VIEWER_LOG_FILE_STUB_NAME);

  Process explorer(command.getString());

  try {
    explorer.start();
  } catch (...) {
    // TODO: Place error notification here.
  }
}

BOOL ConfigurationDialog::onInitDialog()
{
  setControlById(m_showToolBars, IDC_CSHOWTOOLBARS); 
  setControlById(m_warnAtSwitching, IDC_CWARNATSW);
  setControlById(m_numberConn, IDC_ENUMCON);
  setControlById(m_snumConn, IDC_SNUMCON);
  setControlById(m_reverseConn, IDC_EREVCON);
  setControlById(m_sreverseConn, IDC_SREVCON);
  setControlById(m_verbLvl, IDC_EVERBLVL);
  setControlById(m_sverbLvl, IDC_SVERBLVL);
  setControlById(m_logging, IDC_ELOGGING);
  setControlById(m_openLogDir, IDC_OPEN_LOG_FOLDER_BUTTON);
  setControlById(m_skipDownloadConfirm, IDC_CFT_SKIP_CONFIRM);
  setControlById(m_patternList, IDC_CFT_PATTERN_LIST);
  setControlById(m_patternBox, IDC_CFT_PATTERN_EDIT);
  setControlById(m_addPattern, IDC_CFT_ADD_PATTERN);
  setControlById(m_replacePattern, IDC_CFT_REPLACE_PATTERN);
  setControlById(m_removePattern, IDC_CFT_REMOVE_PATTERN);

  m_snumConn.setRange(0, 1024);
  m_snumConn.setBuddy(&m_numberConn);

  m_sreverseConn.setRange32(1, 65535);
  m_sreverseConn.setBuddy(&m_reverseConn);

  m_sverbLvl.setRange(0, 9);
  m_sverbLvl.setBuddy(&m_verbLvl);

  updateControlValues();

  return FALSE;
}

void ConfigurationDialog::updateControlValues()
{
  ViewerConfig *config = ViewerConfig::getInstance();

  StringStorage txt;

  txt.format(_T("%d"), config->getListenPort());
  m_reverseConn.setText(txt.getString());

  txt.format(_T("%d"), config->getLogLevel());
  m_verbLvl.setText(txt.getString());

  txt.format(_T("%d"), config->getHistoryLimit());
  m_numberConn.setText(txt.getString());

  m_showToolBars.check(config->isToolbarShown());
  m_warnAtSwitching.check(config->isPromptOnFullscreenEnabled());
  m_skipDownloadConfirm.check(config->isDownloadConfirmationSkipped());

  StringStorage logFileName;
  logFileName.format(_T("%s\\%s.log"), config->getPathToLogFile(), LogNames::VIEWER_LOG_FILE_STUB_NAME);
  m_logging.setText(logFileName.getString());

  m_autoOverwrite.load();
  m_autoOverwrite.copyTo(&m_workingPatterns);

  fillPatternList(-1);
}

void ConfigurationDialog::fillPatternList(int selectIndex)
{
  m_patternList.clear();

  for (size_t i = 0; i < m_workingPatterns.size(); i++) {
    m_patternList.addString(m_workingPatterns.at(i).getString());
  }

  int count = static_cast<int>(m_workingPatterns.size());

  if (selectIndex >= count) {
    selectIndex = count - 1;
  }
  if (selectIndex >= 0) {
    m_patternList.setSelectedIndex(selectIndex);
  }

  onPatternSelectionChanged();
}

void ConfigurationDialog::onPatternSelectionChanged()
{
  int index = m_patternList.getSelectedIndex();

  if (index >= 0 && static_cast<size_t>(index) < m_workingPatterns.size()) {
    m_patternBox.setText(m_workingPatterns.at(index).getString());
  }

  updatePatternButtons();
}

void ConfigurationDialog::updatePatternButtons()
{
  int index = m_patternList.getSelectedIndex();
  bool haveSelection = index >= 0 &&
                       static_cast<size_t>(index) < m_workingPatterns.size();

  StringStorage typed;
  m_patternBox.getText(&typed);
  bool havePattern = !typed.isEmpty();

  m_addPattern.setEnabled(havePattern);
  m_replacePattern.setEnabled(haveSelection && havePattern);
  m_removePattern.setEnabled(haveSelection);
}

bool ConfigurationDialog::patternIsTaken(const TCHAR *pattern, int exceptIndex)
{
  StringStorage wanted(pattern);
  wanted.toLowerCase();

  for (size_t i = 0; i < m_workingPatterns.size(); i++) {
    if (static_cast<int>(i) == exceptIndex) {
      continue;
    }

    StringStorage held(m_workingPatterns.at(i));
    held.toLowerCase();

    if (held.isEqualTo(&wanted)) {
      return true;
    }
  }
  return false;
}

void ConfigurationDialog::onAddPattern()
{
  StringStorage pattern;
  m_patternBox.getText(&pattern);

  //
  // The button is disabled while the box is empty, so this only guards
  // against the two falling out of step.
  //

  if (pattern.isEmpty()) {
    return ;
  }
  if (patternIsTaken(pattern.getString(), -1)) {
    MessageBox(m_ctrlThis.getWindow(),
               _T("That pattern is already in the list."),
               StringTable::getString(IDS_CONFIGURATION_CAPTION),
               MB_OK | MB_ICONINFORMATION);
    return ;
  }

  //
  // Refused here rather than dropped on save, so that a pattern which will
  // not be kept is never shown as if it had been.
  //

  if (m_workingPatterns.size() >= FtAutoOverwrite::MAX_PATTERNS) {
    StringStorage message;
    message.format(_T("The list holds at most %u patterns."),
                   (unsigned int)FtAutoOverwrite::MAX_PATTERNS);

    MessageBox(m_ctrlThis.getWindow(), message.getString(),
               StringTable::getString(IDS_CONFIGURATION_CAPTION),
               MB_OK | MB_ICONINFORMATION);
    return ;
  }

  m_workingPatterns.push_back(pattern);

  fillPatternList(static_cast<int>(m_workingPatterns.size()) - 1);
}

void ConfigurationDialog::onReplacePattern()
{
  int index = m_patternList.getSelectedIndex();

  if (index < 0 || static_cast<size_t>(index) >= m_workingPatterns.size()) {
    return ;
  }

  StringStorage pattern;
  m_patternBox.getText(&pattern);

  if (pattern.isEmpty()) {
    return ;
  }
  if (patternIsTaken(pattern.getString(), index)) {
    MessageBox(m_ctrlThis.getWindow(),
               _T("That pattern is already in the list."),
               StringTable::getString(IDS_CONFIGURATION_CAPTION),
               MB_OK | MB_ICONINFORMATION);
    return ;
  }

  m_workingPatterns.at(index).setString(pattern.getString());

  fillPatternList(index);
}

void ConfigurationDialog::onRemovePattern()
{
  int index = m_patternList.getSelectedIndex();

  if (index < 0 || static_cast<size_t>(index) >= m_workingPatterns.size()) {
    return ;
  }

  erasePattern(&m_workingPatterns, static_cast<size_t>(index));

  fillPatternList(index);
}

bool ConfigurationDialog::isInputValid()
{
  if (!testNum(&m_reverseConn, StringTable::getString(IDS_CONFIGURATION_LISTEN_PORT))) {
    return false;
  }
  if (!testNum(&m_verbLvl, StringTable::getString(IDS_CONFIGURATION_LOG_LEVEL))) {
    return false;
  }
  if (!testNum(&m_numberConn, StringTable::getString(IDS_CONFIGURATION_HISTORY_LIMIT))) {
    return false;
  }
  return true;
}

bool ConfigurationDialog::testNum(TextBox *tb, const TCHAR *tbName)
{
  StringStorage text;
  tb->getText(&text);

  if (StringParser::tryParseInt(text.getString())) {
    return true;
  }

  StringStorage message;
  message.format(StringTable::getString(IDS_ERROR_VALUE_FIELD_ONLY_NUMERIC), tbName);

  MessageBox(m_ctrlThis.getWindow(), message.getString(),
             StringTable::getString(IDS_CONFIGURATION_CAPTION), MB_OK | MB_ICONWARNING);

  tb->setFocus();

  return false;
}

void ConfigurationDialog::onOkPressed()
{
  if (!isInputValid()) {
    return ;
  }

  ViewerConfig *config = ViewerConfig::getInstance();

  StringStorage text;
  int intVal;

  m_reverseConn.getText(&text);
  StringParser::parseInt(text.getString(), &intVal);
  config->setListenPort(intVal);

  m_verbLvl.getText(&text);
  StringParser::parseInt(text.getString(), &intVal);
  config->setLogLevel(intVal);

  int oldLimit = config->getHistoryLimit();
  m_numberConn.getText(&text);
  StringParser::parseInt(text.getString(), &intVal);
  config->setHistoryLimit(intVal);

  if (config->getHistoryLimit() < oldLimit) {
    config->getConnectionHistory()->truncate();
  }

  config->showToolbar(m_showToolBars.isChecked());
  config->promptOnFullscreen(m_warnAtSwitching.isChecked());
  config->skipDownloadConfirmation(m_skipDownloadConfirm.isChecked());

  SettingsManager *sm = ViewerSettingsManager::getInstance();
  config->saveToStorage(sm);

  //
  // The patterns live in their own registry key rather than in ViewerConfig,
  // because a list needs numbered values and SettingsManager offers no way to
  // store one.
  //

  m_autoOverwrite.save(&m_workingPatterns);
}
