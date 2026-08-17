// Copyright (C) 2009,2010,2011,2012 GlavSoft LLC.
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

#include "FileTransferMainDialog.h"

#include "util/CommonHeader.h"
#include "util/winhdr.h"
#include "NewFolderDialog.h"
#include "FileRenameDialog.h"
#include "FtEditPlacesDialog.h"
#include "FtOptionsDialog.h"

#include "client-config-lib/ViewerConfig.h"

#include "file-lib/File.h"

#include "gui/Menu.h"

#include "NamingDefs.h"
#include "resource.h"
#include <stdio.h>

FileTransferMainDialog::FileTransferMainDialog(FileTransferCore *core,
                                               const TCHAR *hostName)
: FileTransferInterface(core),
  m_hostState(0),
  m_chainIndex(0),
  m_chainActive(false),
  m_chainGeneration(0),
  m_chainFiredGeneration(0),
  m_localPlaces(RegistryPaths::VIEWER_PATH, false),
  m_remotePlaces(RegistryPaths::VIEWER_PATH, true),
  m_autoOverwrite(RegistryPaths::VIEWER_PATH),
  m_downloadInProgress(false),
  m_placesTooltip(0),
  m_gearIcon(0)
{
  setResourceId(ftclient_mainDialog);

  m_lastSentFileListPath.setString(_T(""));
  m_lastReceivedFileListPath.setString(_T(""));

  if (hostName != 0 && hostName[0] != _T('\0')) {
    m_hostState = new FtHostState(RegistryPaths::VIEWER_PATH, hostName);
  }

  m_fakeMoveUpFolder = new FileInfo(0, 0, FileInfo::DIRECTORY, _T(".."));
}

FileTransferMainDialog::~FileTransferMainDialog()
{
  if (m_gearIcon != 0) {
    DestroyIcon(m_gearIcon);
  }

  delete m_fakeMoveUpFolder;
  delete m_hostState;
}

void FileTransferMainDialog::setProgress(double progress)
{
  WORD pt = 1000;
  WORD pc = static_cast<WORD>(progress * pt);

  m_copyProgressBar.setPos(pc);
}

int FileTransferMainDialog::onFtTargetFileExists(FileInfo *sourceFileInfo,
                                                 FileInfo *targetFileInfo,
                                                 const TCHAR *pathToTargetFile)
{
  //
  // Skip All is checked first because it says something about the whole
  // batch, and the user said it out loud. Overwrite All needs no check, since
  // it already reaches the same answer.
  //

  if (m_downloadInProgress && !m_fileExistDialog.isSkipAll()) {
    //
    // Taken from the path rather than from either FileInfo. The two
    // operations hand their source and target to this method in opposite
    // order, and a FileInfo carries whatever name it was built with, while
    // the destination path is the same thing in both directions. Downloads
    // build it with backslashes, which is the separator File splits on.
    //

    StringStorage fileName;
    File targetFile(pathToTargetFile);
    targetFile.getName(&fileName);

    if (m_autoOverwrite.matchesAny(fileName.getString())) {
      return CopyFileEventListener::TFE_OVERWRITE;
    }
  }

  m_fileExistDialog.setFilesInfo(targetFileInfo,
                                 sourceFileInfo,
                                 pathToTargetFile);

  int reasonOfDialog = m_fileExistDialog.showModal();
  switch (reasonOfDialog) {
  case FileExistDialog::SKIP_RESULT:
    return CopyFileEventListener::TFE_SKIP;
  case FileExistDialog::APPEND_RESULT:
    return CopyFileEventListener::TFE_APPEND;
  case FileExistDialog::CANCEL_RESULT:
    onCancelOperationButtonClick();
    return CopyFileEventListener::TFE_CANCEL;
  } // switch

  return CopyFileEventListener::TFE_OVERWRITE;
}

BOOL FileTransferMainDialog::onInitDialog()
{
  m_isClosing = false;

  initControls();

  restoreRemoteFolder();
  restoreLocalFolder();

  //
  // The places row sits at the top of the tab order now, so the dialog would
  // otherwise open with a place button focused. Returning FALSE says the
  // focus has already been placed.
  //

  m_localFileListView.setFocus();

  return FALSE;
}

BOOL FileTransferMainDialog::onNotify(UINT controlID, LPARAM data)
{
  LPNMHDR nmhdr = (LPNMHDR)data;
  switch (controlID) {
  case IDC_REMOTE_FILE_LIST:
    switch (nmhdr->code) {
    case NM_DBLCLK:
      onRemoteListViewDoubleClick();
      break;
    case LVN_KEYDOWN:
      {
        LPNMLVKEYDOWN nmlvkd = (LPNMLVKEYDOWN)data;
        onRemoteListViewKeyDown(nmlvkd->wVKey);
      }
      break;
    case LVN_COLUMNCLICK:
      {
        NMLISTVIEW *lpdi = reinterpret_cast<NMLISTVIEW *>(data);
        m_remoteFileListView.sort(lpdi->iSubItem);
      }
      break;
    } // switch notification code

    //
    // FIXME: Not better way to call this method at every notification
    // for list view control, but windows have no notification for list view
    // selection changed event. So for now, i didn't found better solution.
    //

    checkRemoteListViewSelection();
    break;
  case IDC_LOCAL_FILE_LIST:
    switch (nmhdr->code) {
    case NM_DBLCLK:
      onLocalListViewDoubleClick();
      break;
    case LVN_KEYDOWN:
      {
        LPNMLVKEYDOWN nmlvkd = (LPNMLVKEYDOWN)data;
        onLocalListViewKeyDown(nmlvkd->wVKey);
      }
      break;
    case LVN_COLUMNCLICK:
      {
        NMLISTVIEW *lpdi = reinterpret_cast<NMLISTVIEW *>(data);
        m_localFileListView.sort(lpdi->iSubItem);
      }
      break;
    } // switch notification code

    //
    // FIXME: Not better way to call this method at every notification
    // for list view control, but windows have no notification for list view
    // selection changed event. So for now, i didn't found better solution.
    //

    checkLocalListViewSelection();
    break;
  } // switch controls
  return TRUE;
}

BOOL FileTransferMainDialog::onCommand(UINT controlID, UINT notificationID)
{
  switch (controlID) {
  case IDCANCEL:
    onCancelButtonClick();
    break;
  case IDC_CANCEL_BUTTON:
    onCancelOperationButtonClick();
    break;
  case IDC_RENAME_REMOTE_BUTTON:
    onRenameRemoteButtonClick();
    break;
  case IDC_MKDIR_REMOTE_BUTTON:
    onMkDirRemoteButtonClick();
    break;
  case IDC_REMOVE_REMOTE_BUTTON:
    onRemoveRemoteButtonClick();
    break;
  case IDC_REFRESH_REMOTE_BUTTON:
    onRefreshRemoteButtonClick();
    break;
  case IDC_RENAME_LOCAL_BUTTON:
    onRenameLocalButtonClick();
    break;
  case IDC_MKDIR_LOCAL_BUTTON:
    onMkDirLocalButtonClick();
    break;
  case IDC_REMOVE_LOCAL_BUTTON:
    onRemoveLocalButtonClick();
    break;
  case IDC_REFRESH_LOCAL_BUTTON:
    onRefreshLocalButtonClick();
    break;
  case IDC_UPLOAD_BUTTON:
    onUploadButtonClick();
    break;
  case IDC_DOWNLOAD_BUTTON:
    onDownloadButtonClick();
    break;
  case IDC_LOCAL_PLACE1_BUTTON:
    onPlaceButtonClick(false, 0);
    break;
  case IDC_LOCAL_PLACE2_BUTTON:
    onPlaceButtonClick(false, 1);
    break;
  case IDC_LOCAL_PLACE3_BUTTON:
    onPlaceButtonClick(false, 2);
    break;
  case IDC_LOCAL_PLACE4_BUTTON:
    onPlaceButtonClick(false, 3);
    break;
  case IDC_REMOTE_PLACE1_BUTTON:
    onPlaceButtonClick(true, 0);
    break;
  case IDC_REMOTE_PLACE2_BUTTON:
    onPlaceButtonClick(true, 1);
    break;
  case IDC_REMOTE_PLACE3_BUTTON:
    onPlaceButtonClick(true, 2);
    break;
  case IDC_REMOTE_PLACE4_BUTTON:
    onPlaceButtonClick(true, 3);
    break;
  case IDC_FT_OPTIONS_BUTTON:
    onOptionsButtonClick();
    break;
  case IDC_LOCAL_PLACES_MORE_BUTTON:
    onPlacesMoreButtonClick(false);
    break;
  case IDC_REMOTE_PLACES_MORE_BUTTON:
    onPlacesMoreButtonClick(true);
    break;
  }
  return TRUE;
}

BOOL FileTransferMainDialog::onDestroy()
{
  return TRUE;
}

void FileTransferMainDialog::onMessageReceived(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uMsg) {
  case WM_OPERATION_FINISHED:
    m_ftCore->onOperationFinished();

    if (!m_isClosing) {
      int state = static_cast<int>(wParam);
      int result = static_cast<int>(lParam);
      m_ftCore->onUpdateState(state, result);

      //
      // A candidate chain spans one operation per candidate, so the controls
      // must stay disabled until the whole chain settles rather than being
      // re-enabled between candidates.
      //

      if (state == FileTransferCore::FILE_LIST_STATE && isChainReplyExpected()) {
        onRemoteChainReply(result != 0);
      }

      setProgress(0.0);
      enableControls(!m_chainActive);
      break;
    } else { // If window is closing we can it only if operation finished
      kill(0);
      return;
    } 
  } // switch
} // void

bool FileTransferMainDialog::tryClose()
{
  if (m_ftCore->isNothingState()) {
    // No operation is executing - close dialog
    kill(IDCANCEL);
    return true;
  }
  if (MessageBox(m_ctrlThis.getWindow(),
                 _T("Do you want to close file transfers and terminate current operation?"),
                 _T("TightVNC File Transfers"),
                 MB_YESNO | MB_ICONQUESTION) == IDYES) {
    // Set flag
    m_isClosing = true;
    // Terminate current operation
    m_ftCore->terminateCurrentOperation();
    return true;
  } // if result is not "yes"
  return false;
}

void FileTransferMainDialog::onCancelButtonClick()
{
  tryClose();
}

void FileTransferMainDialog::onCancelOperationButtonClick()
{
  if (!m_ftCore->isNothingState()) {
  // Logging
    StringStorage message(_T("Operation have been canceled by user"));
    insertMessageIntoComboBox(message.getString());

    // Terminate current operation
    m_ftCore->terminateCurrentOperation();
    // Disable "Cancel" button while waiting for a moment
    // when operation will finishe execution.
    m_cancelButton.setEnabled(false);
  }
}

void FileTransferMainDialog::onRenameRemoteButtonClick()
{
  FileInfo *fileInfo = m_remoteFileListView.getSelectedFileInfo();

  if (fileInfo == NULL) {
    MessageBox(m_ctrlThis.getWindow(),
               _T("No file selected."),
               _T("Rename File"), MB_OK | MB_ICONWARNING);
    return ;
  }

  FileRenameDialog renameDialog(&m_ctrlThis);
  renameDialog.setFileName(fileInfo->getFileName());

  if (renameDialog.showModal() == IDOK) {
    StringStorage remoteFolder;
    getPathToCurrentRemoteFolder(&remoteFolder);

    StringStorage oldName(fileInfo->getFileName());

    StringStorage newName;
    renameDialog.getFileName(&newName);

    m_ftCore->remoteFileRenameOperation(FileInfo(0, 0, FileInfo::DIRECTORY, oldName.getString()),
                                        FileInfo(0, 0, FileInfo::DIRECTORY, newName.getString()),
                                        remoteFolder.getString());
  }
}

void FileTransferMainDialog::onMkDirRemoteButtonClick()
{
  NewFolderDialog folderDialog(&m_ctrlThis);
  if (folderDialog.showModal() == IDOK) {
    StringStorage remoteFolder;
    m_remoteCurFolderTextBox.getText(&remoteFolder);

    StringStorage fileName;
    folderDialog.getFileName(&fileName);


    m_ftCore->remoteFolderCreateOperation(FileInfo(0, 0,
                                                   FileInfo::DIRECTORY,
                                                   fileName.getString()),
                                          remoteFolder.getString());
  }
}

void FileTransferMainDialog::onRemoveRemoteButtonClick()
{
  unsigned int siCount = m_remoteFileListView.getSelectedItemsCount();

  if (siCount == 0) {
    MessageBox(m_ctrlThis.getWindow(),
               _T("No files selected."),
               _T("Delete Files"), MB_OK | MB_ICONWARNING);
    return ;
  }

  int *indexes = new int[siCount];
  FileInfo *filesInfo = new FileInfo[siCount];

  m_remoteFileListView.getSelectedItemsIndexes(indexes);
  for (unsigned int i = 0; i < siCount; i++) {
    FileInfo *fileInfo = reinterpret_cast<FileInfo *>(m_remoteFileListView.getItemData(indexes[i]));
    filesInfo[i] = *fileInfo;
  }

  if (MessageBox(m_ctrlThis.getWindow(),
                 _T("Do you wish to delete the selected files?"),
                 _T("Delete Files"),
                 MB_YESNO | MB_ICONQUESTION) != IDYES) {
    delete[] indexes;
    delete[] filesInfo;
    return ;
  }

  StringStorage remoteFolder;
  m_remoteCurFolderTextBox.getText(&remoteFolder);

  m_ftCore->remoteFilesDeleteOperation(filesInfo, siCount,
                                       remoteFolder.getString());
  delete[] indexes;
  delete[] filesInfo;
}

void FileTransferMainDialog::onRefreshRemoteButtonClick()
{
  refreshRemoteFileList();
}

void FileTransferMainDialog::onRenameLocalButtonClick()
{
  FileInfo *fileInfo = m_localFileListView.getSelectedFileInfo();

  if (fileInfo == NULL) {
    MessageBox(m_ctrlThis.getWindow(),
               _T("No file selected."),
               _T("Rename File"), MB_OK | MB_ICONWARNING);
    return ;
  }

  FileRenameDialog renameDialog(&m_ctrlThis);
  renameDialog.setFileName(fileInfo->getFileName());

  if (renameDialog.showModal() == IDOK) {
    StringStorage localFolder;
    getPathToCurrentLocalFolder(&localFolder);

    StringStorage oldName;
    StringStorage newName;

    oldName.setString(fileInfo->getFileName());
    renameDialog.getFileName(&newName);

    StringStorage pathToOldFile(localFolder.getString());
    StringStorage pathToNewFile(localFolder.getString());

    if (!localFolder.endsWith('\\')) {
      pathToOldFile.appendString(_T("\\"));
      pathToNewFile.appendString(_T("\\"));
    }

    pathToOldFile.appendString(oldName.getString());
    pathToNewFile.appendString(newName.getString());

    //
    // Logging
    //

    StringStorage message;

    message.format(_T("Renaming local file '%s' to '%s'"),
                   pathToOldFile.getString(), pathToNewFile.getString());

    insertMessageIntoComboBox(message.getString());

     // Rename local file
    File oldFile(pathToOldFile.getString());

    if (!oldFile.renameTo(pathToNewFile.getString())) {
      message.format(_T("Error: failed to rename local '%s' file"),
                     pathToOldFile.getString());

      insertMessageIntoComboBox(message.getString());
    }

    refreshLocalFileList();
  } // if dialog result is ok
} // void

void FileTransferMainDialog::onMkDirLocalButtonClick()
{
  StringStorage pathToFile;

  getPathToCurrentLocalFolder(&pathToFile);

  // Not allow user to create folders in our "fake" root folder
  if (pathToFile.isEmpty()) {
    MessageBox(m_ctrlThis.getWindow(),
               _T("It's not allowed to create new folder here."),
               _T("New Folder"), MB_OK | MB_ICONWARNING);
  }

  NewFolderDialog folderDialog(&m_ctrlThis);

  if (folderDialog.showModal() == IDOK) {
    StringStorage fileName;
    folderDialog.getFileName(&fileName);

    if (!pathToFile.endsWith(_T('\\'))) {
      pathToFile.appendString(_T("\\"));
    }
    pathToFile.appendString(fileName.getString());

    // Logging
    StringStorage message;

    message.format(_T("Creating local folder '%s'"), pathToFile.getString());

    insertMessageIntoComboBox(message.getString());

    // File system object
    File file(pathToFile.getString());

    // Failed to create local folder
    if (pathToFile.isEmpty() || !file.mkdir()) {
      message.format(_T("Error: failed to create local folder '%s'"),
                     pathToFile.getString());

      insertMessageIntoComboBox(message.getString());
    }

    refreshLocalFileList();
  } // if dialog result is ok
} // void

void FileTransferMainDialog::onRemoveLocalButtonClick()
{
  unsigned int siCount = m_localFileListView.getSelectedItemsCount();

  if (siCount == 0) {
    MessageBox(m_ctrlThis.getWindow(),
               _T("No files selected."),
               _T("Delete Files"), MB_OK | MB_ICONWARNING);
    return ;
  }

  int *indexes = new int[siCount];
  FileInfo *filesInfo = new FileInfo[siCount];

  m_localFileListView.getSelectedItemsIndexes(indexes);
  for (unsigned int i = 0; i < siCount; i++) {
    FileInfo *fileInfo = reinterpret_cast<FileInfo *>(m_localFileListView.getItemData(indexes[i]));
    filesInfo[i] = *fileInfo;
  }

  if (MessageBox(m_ctrlThis.getWindow(),
                 _T("Do you wish to delete the selected files?"),
                 _T("Delete Files"),
                 MB_YESNO | MB_ICONQUESTION) != IDYES) {
    delete[] indexes;
    delete[] filesInfo;
    return ;
  }

  StringStorage localFolder;
  getPathToCurrentLocalFolder(&localFolder);

  m_ftCore->localFilesDeleteOperation(filesInfo, siCount,
                                      localFolder.getString());

  delete[] indexes;
  delete[] filesInfo;
}

void FileTransferMainDialog::onRefreshLocalButtonClick()
{
  refreshLocalFileList();
}

void FileTransferMainDialog::onUploadButtonClick()
{
  unsigned int siCount = m_localFileListView.getSelectedItemsCount();

  if (siCount == 0) {
    MessageBox(m_ctrlThis.getWindow(),
               _T("No files selected."),
               _T("Upload Files"), MB_OK | MB_ICONWARNING);
    return ;
  }

  int *indexes = new int[siCount];
  FileInfo *filesInfo = new FileInfo[siCount];

  m_localFileListView.getSelectedItemsIndexes(indexes);
  for (unsigned int i = 0; i < siCount; i++) {
    FileInfo *fileInfo = reinterpret_cast<FileInfo *>(m_localFileListView.getItemData(indexes[i]));
    filesInfo[i] = *fileInfo;
  }

  if (!ViewerConfig::getInstance()->isUploadConfirmationSkipped()) {
    if (MessageBox(m_ctrlThis.getWindow(),
                   _T("Do you wish to upload the selected files?"),
                   _T("Upload Files"),
                   MB_YESNO | MB_ICONQUESTION) != IDYES) {
      delete[] indexes;
      delete[] filesInfo;
      return ;
    }
  }

  StringStorage localFolder;
  getPathToCurrentLocalFolder(&localFolder);

  StringStorage remoteFolder;
  getPathToCurrentRemoteFolder(&remoteFolder);


  m_fileExistDialog.resetDialogResultValue();

  m_downloadInProgress = false;

  m_ftCore->uploadOperation(filesInfo, siCount,
                            localFolder.getString(),
                            remoteFolder.getString());
  delete[] indexes;
  delete[] filesInfo;
}

void FileTransferMainDialog::onDownloadButtonClick()
{
  unsigned int siCount = m_remoteFileListView.getSelectedItemsCount();

  if (siCount == 0) {
    MessageBox(m_ctrlThis.getWindow(),
               _T("No files selected."),
               _T("Download Files"), MB_OK | MB_ICONWARNING);
    return ;
  }

  int *indexes = new int[siCount];
  FileInfo *filesInfo = new FileInfo[siCount];

  m_remoteFileListView.getSelectedItemsIndexes(indexes);
  for (unsigned int i = 0; i < siCount; i++) {
    FileInfo *fileInfo = reinterpret_cast<FileInfo *>(m_remoteFileListView.getItemData(indexes[i]));
    filesInfo[i] = *fileInfo;
  }

  if (!ViewerConfig::getInstance()->isDownloadConfirmationSkipped()) {
    if (MessageBox(m_ctrlThis.getWindow(),
                   _T("Do you wish to download the selected files?"),
                   _T("Download Files"),
                   MB_YESNO | MB_ICONQUESTION) != IDYES) {
      delete[] indexes;
      delete[] filesInfo;
      return ;
    }
  }

  StringStorage remoteFolder;
  getPathToCurrentRemoteFolder(&remoteFolder);

  StringStorage localFolder;
  getPathToCurrentLocalFolder(&localFolder);

  m_fileExistDialog.resetDialogResultValue();

  //
  // Reread rather than trust what was loaded when the dialog opened, so that
  // a pattern added in the transfer options applies to this download.
  //

  m_downloadInProgress = true;
  m_autoOverwrite.load();

  m_ftCore->downloadOperation(filesInfo, siCount,
                              localFolder.getString(),
                              remoteFolder.getString());

  delete[] indexes;
  delete[] filesInfo;
}

void FileTransferMainDialog::moveUpLocalFolder()
{
  StringStorage pathToFile;
  getPathToParentLocalFolder(&pathToFile);
  tryListLocalFolder(pathToFile.getString());
}

void FileTransferMainDialog::moveUpRemoteFolder()
{
  StringStorage parent;
  getPathToParentRemoteFolder(&parent);
  navigateRemoteFolder(parent.getString());
}

void FileTransferMainDialog::onRemoteListViewDoubleClick()
{
  FileInfo *selFileInfo = m_remoteFileListView.getSelectedFileInfo();
  if (selFileInfo == 0)
    return;

  if (!selFileInfo->isDirectory())
    return;

  int si = m_remoteFileListView.getSelectedIndex();

  // Fake ".." folder - move one folder up
  if ((si == 0) && (selFileInfo != 0) && (_tcscmp(selFileInfo->getFileName(), _T(".."))) == 0) {
    moveUpRemoteFolder();
    return ;
  }
  if (si == -1) {
    return ;
  }
  StringStorage pathToFile;
  getPathToSelectedRemoteFile(&pathToFile);
  navigateRemoteFolder(pathToFile.getString());
}

void FileTransferMainDialog::onLocalListViewDoubleClick()
{
  // FIXME: removed duplicate code (see onRemoteListViewDoubleClick)
  FileInfo *selFileInfo = m_localFileListView.getSelectedFileInfo();

  if (selFileInfo == 0)
    return;

  if (!selFileInfo->isDirectory())
    return;

  int si = m_localFileListView.getSelectedIndex();

  // Fake ".." folder - move one folder up
  if ((si == 0) && (selFileInfo != 0) && (_tcscmp(selFileInfo->getFileName(), _T(".."))) == 0) {
    moveUpLocalFolder();
    return ;
  }
  if (si == -1) {
    return ;
  }

  StringStorage pathToFile;
  getPathToSelectedLocalFile(&pathToFile);
  tryListLocalFolder(pathToFile.getString());
}

void FileTransferMainDialog::onRemoteListViewKeyDown(UINT key)
{
  switch (key) {
  case VK_RETURN:
    onRemoteListViewDoubleClick();
    break;
  case VK_DELETE:
    onRemoveRemoteButtonClick();
    break;
  case VK_BACK:
    moveUpRemoteFolder();
    break;
  }
}

void FileTransferMainDialog::onLocalListViewKeyDown(UINT key)
{
  // FIXME: removed duplicate code (see onRemoteListViewKeyDown)
  switch (key) {
  case VK_RETURN:
    onLocalListViewDoubleClick();
    break;
  case VK_DELETE:
    onRemoveLocalButtonClick();
    break;
  case VK_BACK:
    moveUpLocalFolder();
    break;
  }
}

void FileTransferMainDialog::checkRemoteListViewSelection()
{
  if (m_ftCore->isNothingState()) {

    bool enabled = m_remoteFileListView.getSelectedItemsCount() > 0;

    m_renameRemoteButton.setEnabled(enabled && m_ftCore->getSupportedOps().isRenameSupported());
    m_removeRemoteButton.setEnabled(enabled && m_ftCore->getSupportedOps().isRemoveSupported());
  }
}

void FileTransferMainDialog::checkLocalListViewSelection()
{
  if (m_ftCore->isNothingState()) {
    bool enabled = m_localFileListView.getSelectedItemsCount() > 0;

    m_renameLocalButton.setEnabled(enabled);
    m_removeLocalButton.setEnabled(enabled);
  }
}

void FileTransferMainDialog::insertMessageIntoComboBox(const TCHAR *message)
{
  m_logComboBox.insertItem(0, message);
  m_logComboBox.setSelectedItem(0);
}

void FileTransferMainDialog::enableControls(bool enabled)
{
  m_mkDirRemoteButton.setEnabled(enabled && m_ftCore->getSupportedOps().isMkDirSupported());

  if (m_remoteFileListView.getSelectedItemsCount() > 0 && enabled) {
    m_renameRemoteButton.setEnabled(true && m_ftCore->getSupportedOps().isRenameSupported());
    m_removeRemoteButton.setEnabled(true && m_ftCore->getSupportedOps().isRemoveSupported());
  } else {
    m_renameRemoteButton.setEnabled(enabled && m_ftCore->getSupportedOps().isRenameSupported());
    m_removeRemoteButton.setEnabled(enabled && m_ftCore->getSupportedOps().isRemoveSupported());
  }

  m_refreshRemoteButton.setEnabled(enabled);

  if (enabled) {
    StringStorage curLocalPath;

    getPathToCurrentLocalFolder(&curLocalPath);

    if (!curLocalPath.isEmpty()) {
      m_mkDirLocalButton.setEnabled(true);
    }
  } else {
    m_mkDirLocalButton.setEnabled(enabled);
  }

  if (m_localFileListView.getSelectedItemsCount() > 0 && enabled) {
    m_renameLocalButton.setEnabled(true);
    m_removeLocalButton.setEnabled(true);
  } else {
    m_renameLocalButton.setEnabled(enabled);
    m_removeLocalButton.setEnabled(enabled);
  }

  m_refreshLocalButton.setEnabled(enabled);

  m_uploadButton.setEnabled(enabled && m_ftCore->getSupportedOps().isUploadSupported());
  m_downloadButton.setEnabled(enabled && m_ftCore->getSupportedOps().isDownloadSupported());

  for (size_t i = 0; i < PLACE_BUTTON_COUNT; i++) {
    m_localPlaceButtons[i].setEnabled(enabled);
    m_remotePlaceButtons[i].setEnabled(enabled);
  }

  m_localPlacesMoreButton.setEnabled(enabled);
  m_remotePlacesMoreButton.setEnabled(enabled);

  m_optionsButton.setEnabled(enabled);

  m_localFileListView.setEnabled(enabled);
  m_remoteFileListView.setEnabled(enabled);

  m_cancelButton.setEnabled(!enabled);
}

void FileTransferMainDialog::initControls()
{
  HWND hwnd = m_ctrlThis.getWindow();

  m_renameRemoteButton.setWindow(GetDlgItem(hwnd, IDC_RENAME_REMOTE_BUTTON));
  m_mkDirRemoteButton.setWindow(GetDlgItem(hwnd, IDC_MKDIR_REMOTE_BUTTON));
  m_removeRemoteButton.setWindow(GetDlgItem(hwnd, IDC_REMOVE_REMOTE_BUTTON));
  m_refreshRemoteButton.setWindow(GetDlgItem(hwnd, IDC_REFRESH_REMOTE_BUTTON));

  m_renameLocalButton.setWindow(GetDlgItem(hwnd, IDC_RENAME_LOCAL_BUTTON));
  m_mkDirLocalButton.setWindow(GetDlgItem(hwnd, IDC_MKDIR_LOCAL_BUTTON));
  m_removeLocalButton.setWindow(GetDlgItem(hwnd, IDC_REMOVE_LOCAL_BUTTON));
  m_refreshLocalButton.setWindow(GetDlgItem(hwnd, IDC_REFRESH_LOCAL_BUTTON));

  m_uploadButton.setWindow(GetDlgItem(hwnd, IDC_UPLOAD_BUTTON));
  m_downloadButton.setWindow(GetDlgItem(hwnd, IDC_DOWNLOAD_BUTTON));

  static const UINT localPlaceIds[PLACE_BUTTON_COUNT] = {
    IDC_LOCAL_PLACE1_BUTTON,
    IDC_LOCAL_PLACE2_BUTTON,
    IDC_LOCAL_PLACE3_BUTTON,
    IDC_LOCAL_PLACE4_BUTTON
  };
  static const UINT remotePlaceIds[PLACE_BUTTON_COUNT] = {
    IDC_REMOTE_PLACE1_BUTTON,
    IDC_REMOTE_PLACE2_BUTTON,
    IDC_REMOTE_PLACE3_BUTTON,
    IDC_REMOTE_PLACE4_BUTTON
  };

  for (size_t i = 0; i < PLACE_BUTTON_COUNT; i++) {
    m_localPlaceButtons[i].setWindow(GetDlgItem(hwnd, localPlaceIds[i]));
    m_remotePlaceButtons[i].setWindow(GetDlgItem(hwnd, remotePlaceIds[i]));
  }

  m_localPlacesMoreButton.setWindow(GetDlgItem(hwnd, IDC_LOCAL_PLACES_MORE_BUTTON));
  m_remotePlacesMoreButton.setWindow(GetDlgItem(hwnd, IDC_REMOTE_PLACES_MORE_BUTTON));

  m_optionsButton.setWindow(GetDlgItem(hwnd, IDC_FT_OPTIONS_BUTTON));

  //
  // LoadImage rather than LoadIcon, because LoadIcon answers with the large
  // icon and the button would then scale it down itself.
  //

  m_gearIcon = (HICON)LoadImage(GetModuleHandle(0),
                                MAKEINTRESOURCE(IDI_GEAR), IMAGE_ICON,
                                16, 16, LR_DEFAULTCOLOR);

  if (m_gearIcon != 0) {
    SendMessage(m_optionsButton.getWindow(), BM_SETIMAGE, IMAGE_ICON,
                reinterpret_cast<LPARAM>(m_gearIcon));
  }

  m_cancelButton.setWindow(GetDlgItem(hwnd, IDC_CANCEL_BUTTON));

  m_copyProgressBar.setWindow(GetDlgItem(hwnd, IDC_TOTAL_PROGRESS));
  m_copyProgressBar.setRange(0, 1000);

  m_logComboBox.setWindow(GetDlgItem(hwnd, IDC_LOG_COMBO));

  m_localCurFolderTextBox.setWindow(GetDlgItem(hwnd, IDC_LOCAL_CURRENT_FOLDER_EDIT));
  m_remoteCurFolderTextBox.setWindow(GetDlgItem(hwnd, IDC_REMOTE_CURRENT_FOLDER_EDIT));

  m_localFileListView.setWindow(GetDlgItem(hwnd, IDC_LOCAL_FILE_LIST));
  m_remoteFileListView.setWindow(GetDlgItem(hwnd, IDC_REMOTE_FILE_LIST));

  m_fileExistDialog.setParent(&m_ctrlThis);

  //
  // The tooltip goes up first, because labelling a button also decides
  // whether that button needs one.
  //

  initPlacesTooltip();

  //
  // Read once here so the place buttons can be labelled. Every later read
  // happens when the menu is opened, which is also where the labels are
  // brought up to date.
  //

  m_localPlaces.load();
  m_remotePlaces.load();

  updatePlaceButtons(false);
  updatePlaceButtons(true);
}

void FileTransferMainDialog::raise(Exception &ex)
{
  MessageBox(m_ctrlThis.getWindow(), ex.getMessage(),
             _T("Exception"), MB_OK | MB_ICONERROR);
  throw ex;
}

void FileTransferMainDialog::refreshLocalFileList()
{
  StringStorage pathToFile;
  getPathToCurrentLocalFolder(&pathToFile);
  tryListLocalFolder(pathToFile.getString());
}

bool FileTransferMainDialog::tryListLocalFolder(const TCHAR *pathToFile,
                                                bool reportFailure)
{
  try {
    vector <FileInfo> *localFileList = m_ftCore->getListLocalFolder(pathToFile);


    // Add to list view
    m_localFileListView.clear();
    if (!localFileList->empty()) {
      FileInfo *fileInfo = &localFileList->front();
      m_localFileListView.addRange(&fileInfo,
                                   localFileList->size());
    }

    bool isRoot = (_tcscmp(pathToFile, _T("")) == 0);

    // Add ".." folder and if not root
    if (!isRoot) {
      m_localFileListView.addItem(0, m_fakeMoveUpFolder);
    }
    // Set label text
    m_localCurFolderTextBox.setText(pathToFile);
    // Enable or disable mkdir button depending on isRoot flag
    m_mkDirLocalButton.setEnabled(!isRoot);

    if (m_hostState != 0) {
      m_hostState->setLastLocalFolder(pathToFile);
    }

  } catch (...) {
    if (reportFailure) {
      StringStorage message;

      message.format(_T("Error: failed to get file list in local folder '%s'"),
                     pathToFile);

      insertMessageIntoComboBox(message.getString());
    }
    return false;
  }
  return true;
}

void FileTransferMainDialog::refreshRemoteFileList()
{
  StringStorage currentFolder;
  m_remoteCurFolderTextBox.getText(&currentFolder);
  navigateRemoteFolder(currentFolder.getString());
}

void FileTransferMainDialog::tryListRemoteFolder(const TCHAR *pathToFile)
{
  m_lastSentFileListPath.setString(pathToFile);
  m_ftCore->remoteFileListOperation(pathToFile);
}

void FileTransferMainDialog::navigateRemoteFolder(const TCHAR *pathToFile)
{
  endRemoteChain();
  tryListRemoteFolder(pathToFile);
}

void FileTransferMainDialog::restoreLocalFolder()
{
  StringStorage saved;

  if (m_hostState != 0 && m_hostState->getLastLocalFolder(&saved)) {
    if (tryListLocalFolder(saved.getString(), false)) {
      return;
    }

    StringStorage warning;
    warning.format(_T("Warning: %s not found on this computer"),
                   saved.getString());
    insertMessageIntoComboBox(warning.getString());
  }
  tryListLocalFolder(_T(""));
}

void FileTransferMainDialog::restoreRemoteFolder()
{
  vector<StringStorage> candidates;
  StringStorage saved;

  //
  // The remembered folder can be gone, so the server root is the fallback.
  // Skip the duplicate when the remembered folder is the root already.
  //

  if (m_hostState != 0 &&
      m_hostState->getLastRemoteFolder(&saved) &&
      !saved.isEqualTo(_T("/"))) {
    candidates.push_back(saved);
  }
  candidates.push_back(StringStorage(_T("/")));

  startRemoteChain(&candidates, _T("the remembered folder"));
}

void FileTransferMainDialog::startRemoteChain(const vector<StringStorage> *candidates,
                                              const TCHAR *description,
                                              const TCHAR *placeName)
{
  //
  // Copied element by element rather than by vector assignment, because
  // StringStorage::operator = returns void and so is not assignable in the
  // sense the standard containers ask for.
  //

  m_chainCandidates.clear();
  for (size_t i = 0; i < candidates->size(); i++) {
    m_chainCandidates.push_back(candidates->at(i));
  }

  m_chainIndex = 0;
  m_chainActive = false;
  m_chainDescription.setString(description);
  m_chainPlaceName.setString(placeName != 0 ? placeName : _T(""));
  m_chainGeneration++;

  if (m_chainCandidates.empty()) {
    return;
  }

  m_chainActive = true;
  fireRemoteChainCandidate();
}

void FileTransferMainDialog::fireRemoteChainCandidate()
{
  m_chainFiredGeneration = m_chainGeneration;
  tryListRemoteFolder(m_chainCandidates.at(m_chainIndex).getString());
}

void FileTransferMainDialog::endRemoteChain()
{
  m_chainActive = false;
  m_chainCandidates.clear();
  m_chainPlaceName.setString(_T(""));
  m_chainGeneration++;
}

bool FileTransferMainDialog::isChainReplyExpected() const
{
  return m_chainActive && (m_chainFiredGeneration == m_chainGeneration);
}

void FileTransferMainDialog::onRemoteChainReply(bool listed)
{
  if (listed) {
    //
    // Remember which candidate won, so the next visit to this place costs a
    // single request instead of hunting again.
    //

    if (!m_chainPlaceName.isEmpty() && m_hostState != 0) {
      m_hostState->setResolvedPlace(m_chainPlaceName.getString(),
                                    m_chainCandidates.at(m_chainIndex).getString());
    }
    endRemoteChain();
    return;
  }

  StringStorage warning;
  warning.format(_T("Warning: %s not found on the server"),
                 m_chainCandidates.at(m_chainIndex).getString());
  insertMessageIntoComboBox(warning.getString());

  m_chainIndex++;
  if (m_chainIndex < m_chainCandidates.size()) {
    fireRemoteChainCandidate();
    return;
  }

  StringStorage message;
  message.format(_T("Error: no folder found on the server for %s"),
                 m_chainDescription.getString());
  insertMessageIntoComboBox(message.getString());

  endRemoteChain();
}

void FileTransferMainDialog::initPlacesTooltip()
{
  //
  // Held in a static because the tooltip keeps the pointer it is given rather
  // than copying the string.
  //

  static TCHAR placesText[] = _T("Places");
  static TCHAR optionsText[] = _T("Transfer Options");

  //
  // Tooltips come from the bar classes, which nothing in this dialog would
  // otherwise pull in. The call costs nothing when they are already up.
  //

  INITCOMMONCONTROLSEX icc;
  icc.dwSize = sizeof(icc);
  icc.dwICC = ICC_BAR_CLASSES;
  InitCommonControlsEx(&icc);

  HWND hwnd = m_ctrlThis.getWindow();

  m_placesTooltip = CreateWindowEx(0, TOOLTIPS_CLASS, 0,
                                   WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,
                                   CW_USEDEFAULT, CW_USEDEFAULT,
                                   CW_USEDEFAULT, CW_USEDEFAULT,
                                   hwnd, 0, GetModuleHandle(0), 0);

  if (m_placesTooltip == 0) {
    return;
  }

  TOOLINFO info;
  memset(&info, 0, sizeof(info));

  info.cbSize = sizeof(info);

  //
  // TTF_SUBCLASS lets the tooltip pick the mouse messages up itself. Without
  // it the dialog would have to relay every one, and a dialog has no message
  // loop of its own to do that in.
  //

  info.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
  info.hwnd = hwnd;
  info.lpszText = placesText;

  info.uId = reinterpret_cast<UINT_PTR>(m_localPlacesMoreButton.getWindow());
  SendMessage(m_placesTooltip, TTM_ADDTOOL, 0,
              reinterpret_cast<LPARAM>(&info));

  info.uId = reinterpret_cast<UINT_PTR>(m_remotePlacesMoreButton.getWindow());
  SendMessage(m_placesTooltip, TTM_ADDTOOL, 0,
              reinterpret_cast<LPARAM>(&info));

  info.lpszText = optionsText;
  info.uId = reinterpret_cast<UINT_PTR>(m_optionsButton.getWindow());
  SendMessage(m_placesTooltip, TTM_ADDTOOL, 0,
              reinterpret_cast<LPARAM>(&info));

  //
  // The place buttons join as tools now and get their text later, once the
  // places are read. A tool carrying an empty string shows nothing, which is
  // what an untruncated name and an unused slot both want.
  //

  info.lpszText = _T("");

  for (size_t i = 0; i < PLACE_BUTTON_COUNT; i++) {
    info.uId = reinterpret_cast<UINT_PTR>(m_localPlaceButtons[i].getWindow());
    SendMessage(m_placesTooltip, TTM_ADDTOOL, 0,
                reinterpret_cast<LPARAM>(&info));

    info.uId = reinterpret_cast<UINT_PTR>(m_remotePlaceButtons[i].getWindow());
    SendMessage(m_placesTooltip, TTM_ADDTOOL, 0,
                reinterpret_cast<LPARAM>(&info));
  }
}

void FileTransferMainDialog::setPlaceButtonTip(bool remote, size_t slot,
                                               const TCHAR *text)
{
  if (m_placesTooltip == 0) {
    return;
  }

  //
  // The tooltip keeps the pointer it is given rather than copying the string,
  // so the text is stored first and the pointer handed over second. Anything
  // that rewrites one of these must send this message again.
  //

  StringStorage *held = remote ? &m_remotePlaceTips[slot]
                               : &m_localPlaceTips[slot];
  held->setString(text);

  Control *button = remote ? &m_remotePlaceButtons[slot]
                           : &m_localPlaceButtons[slot];

  TOOLINFO info;
  memset(&info, 0, sizeof(info));

  info.cbSize = sizeof(info);
  info.uFlags = TTF_IDISHWND;
  info.hwnd = m_ctrlThis.getWindow();
  info.uId = reinterpret_cast<UINT_PTR>(button->getWindow());
  info.lpszText = const_cast<TCHAR *>(held->getString());

  SendMessage(m_placesTooltip, TTM_UPDATETIPTEXT, 0,
              reinterpret_cast<LPARAM>(&info));
}

bool FileTransferMainDialog::setPlaceButtonText(Control *button,
                                                const TCHAR *name)
{
  static const TCHAR ELLIPSIS[] = _T("...");

  HWND hwnd = button->getWindow();

  RECT rect;
  GetClientRect(hwnd, &rect);

  int room = (rect.right - rect.left) - PLACE_BUTTON_TEXT_MARGIN;

  HDC dc = GetDC(hwnd);

  if (dc == 0 || room <= 0) {
    if (dc != 0) {
      ReleaseDC(hwnd, dc);
    }
    button->setText(name);
    return false;
  }

  //
  // Measured in the button's own font. The dialog font is not the system
  // default, so measuring without it would answer for the wrong typeface.
  //

  HFONT font = reinterpret_cast<HFONT>(SendMessage(hwnd, WM_GETFONT, 0, 0));
  HGDIOBJ previous = 0;

  if (font != 0) {
    previous = SelectObject(dc, font);
  }

  StringStorage text(name);
  size_t full = text.getLength();

  bool truncated = false;

  SIZE size;

  if (GetTextExtentPoint32(dc, name, static_cast<int>(full), &size) != 0 &&
      size.cx > room) {
    truncated = true;

    //
    // Shortened one character at a time rather than by estimating from an
    // average width, because a proportional font makes that estimate wrong
    // in both directions.
    //

    text.setString(ELLIPSIS);

    for (size_t keep = full; keep > 0; keep--) {
      StringStorage candidate(name);
      candidate.truncate(full - keep);
      candidate.appendString(ELLIPSIS);

      if (GetTextExtentPoint32(dc, candidate.getString(),
                               static_cast<int>(candidate.getLength()),
                               &size) != 0 && size.cx <= room) {
        text.setString(candidate.getString());
        break;
      }
    }
  }

  if (previous != 0) {
    SelectObject(dc, previous);
  }
  ReleaseDC(hwnd, dc);

  button->setText(text.getString());

  return truncated;
}

void FileTransferMainDialog::onOptionsButtonClick()
{
  FtOptionsDialog options(&m_ctrlThis);

  if (options.showModal() != IDOK) {
    return;
  }

  //
  // A download already under way reads the patterns when it starts, so it
  // keeps the list it began with. This picks up the edit for the next one.
  //

  m_autoOverwrite.load();
}

void FileTransferMainDialog::updatePlaceButtons(bool remote)
{
  const FtPlaces *places = remote ? &m_remotePlaces : &m_localPlaces;
  Control *buttons = remote ? m_remotePlaceButtons : m_localPlaceButtons;

  size_t count = places->getCount();

  for (size_t i = 0; i < PLACE_BUTTON_COUNT; i++) {
    if (i < count) {
      const TCHAR *name = places->getPlace(i)->name.getString();

      //
      // The tooltip carries the name only when the button could not show all
      // of it. Repeating a name the user can already read would be noise.
      //

      bool truncated = setPlaceButtonText(&buttons[i], name);

      setPlaceButtonTip(remote, i, truncated ? name : _T(""));

      buttons[i].setVisible(true);
    } else {
      //
      // Cleared as well as hidden, so a name cannot flash back if the button
      // is shown again before it has been relabelled.
      //

      buttons[i].setText(_T(""));
      setPlaceButtonTip(remote, i, _T(""));
      buttons[i].setVisible(false);
    }
  }
}

void FileTransferMainDialog::onPlaceButtonClick(bool remote, size_t slot)
{
  const FtPlaces *places = remote ? &m_remotePlaces : &m_localPlaces;

  //
  // A hidden button cannot be clicked, so this only guards against the
  // labelling and the places falling out of step.
  //

  if (slot >= places->getCount()) {
    return;
  }

  if (remote) {
    goToRemotePlace(places->getPlace(slot));
  } else {
    goToLocalPlace(places->getPlace(slot));
  }
}

void FileTransferMainDialog::onPlacesMoreButtonClick(bool remote)
{
  //
  // Reread every time, so places edited in the registry show up without
  // restarting the viewer. The buttons follow, since this is the only place
  // that rereads.
  //

  FtPlaces *places = remote ? &m_remotePlaces : &m_localPlaces;
  places->load();
  updatePlaceButtons(remote);

  Menu menu;
  menu.createPopupMenu();

  size_t count = places->getCount();

  if (count == 0) {
    //
    // Command id zero is what a dismissed menu returns, so this entry is
    // inert without needing to be greyed. It is worth saying, because with no
    // places there are no buttons either and the row looks broken otherwise.
    //

    StringStorage empty(_T("(no places defined)"));
    menu.appendMenu(empty, PLACES_MENU_NONE);
    menu.appendSeparator();
  } else if (count > PLACE_BUTTON_COUNT) {
    //
    // Only what did not fit on the buttons. Repeating the first few here
    // would make the row look like it had failed to take them.
    //
    // Every place on a button leaves this menu with nothing of its own, and
    // so with nothing for a separator to separate.
    //

    for (size_t i = PLACE_BUTTON_COUNT; i < count; i++) {
      menu.appendMenu(places->getPlace(i)->name,
                      static_cast<UINT>(PLACES_MENU_FIRST_PLACE + i));
    }
    menu.appendSeparator();
  }

  //
  // Only remote resolutions are cached, so only that pane needs a rescan.
  // Local hunting is a few file system calls and always runs fresh.
  //

  if (remote) {
    StringStorage rescan(_T("Rescan"));
    menu.appendMenu(rescan, PLACES_MENU_RESCAN);
  }

  StringStorage edit(_T("Edit Places..."));
  menu.appendMenu(edit, PLACES_MENU_EDIT);

  Control *button = remote ? &m_remotePlacesMoreButton
                           : &m_localPlacesMoreButton;
  RECT buttonRect;
  GetWindowRect(button->getWindow(), &buttonRect);

  //
  // Right aligned, because the button sits at the right edge of its pane and
  // a left aligned menu would hang off it.
  //

  int action = TrackPopupMenu(menu.getMenu(),
                              TPM_NONOTIFY | TPM_RETURNCMD | TPM_RIGHTALIGN,
                              buttonRect.right, buttonRect.bottom,
                              0, m_ctrlThis.getWindow(), NULL);

  if (action == PLACES_MENU_NONE) {
    return;
  }
  if (action == PLACES_MENU_RESCAN) {
    rescanPlaces();
    return;
  }
  if (action == PLACES_MENU_EDIT) {
    FtEditPlacesDialog editor(&m_ctrlThis, remote);

    if (editor.showModal() == IDOK) {
      places->load();
      updatePlaceButtons(remote);
    }
    return;
  }

  size_t index = static_cast<size_t>(action - PLACES_MENU_FIRST_PLACE);
  if (index >= places->getCount()) {
    return;
  }

  if (remote) {
    goToRemotePlace(places->getPlace(index));
  } else {
    goToLocalPlace(places->getPlace(index));
  }
}

void FileTransferMainDialog::goToLocalPlace(const FtPlace *place)
{
  //
  // Local resolution runs fresh every time. Checking a path costs a file
  // system call, so caching would buy nothing and could only go stale.
  //

  for (size_t i = 0; i < place->candidates.size(); i++) {
    const TCHAR *path = place->candidates.at(i).getString();

    File candidate(path);
    if (candidate.exists() && candidate.isDirectory()) {
      if (tryListLocalFolder(path, false)) {
        return;
      }
    }

    //
    // A candidate that is missing, or that exists but will not list, is only
    // a warning while others remain to be tried.
    //

    StringStorage warning;
    warning.format(_T("Warning: %s not found on this computer"), path);
    insertMessageIntoComboBox(warning.getString());
  }

  StringStorage message;
  message.format(_T("Error: no folder found on this computer for %s"),
                 place->name.getString());
  insertMessageIntoComboBox(message.getString());
}

void FileTransferMainDialog::goToRemotePlace(const FtPlace *place)
{
  vector<StringStorage> candidates;
  StringStorage cached;

  //
  // A cached answer goes first rather than replacing the hunt. If the folder
  // has since gone, the chain simply carries on into the real candidates and
  // the stale entry is overwritten by whatever wins.
  //

  bool haveCached = m_hostState != 0 &&
                    m_hostState->getResolvedPlace(place->name.getString(), &cached);

  if (haveCached) {
    candidates.push_back(cached);
  }

  for (size_t i = 0; i < place->candidates.size(); i++) {
    if (haveCached && cached.isEqualTo(&place->candidates.at(i))) {
      continue;
    }
    candidates.push_back(place->candidates.at(i));
  }

  startRemoteChain(&candidates,
                   place->name.getString(),
                   place->name.getString());
}

void FileTransferMainDialog::rescanPlaces()
{
  if (m_hostState == 0) {
    insertMessageIntoComboBox(_T("No server name is known, so nothing is cached"));
    return;
  }

  //
  // Counted from the places defined now. RegistryKey cannot enumerate values,
  // and an entry left over from a deleted place is not worth reporting even
  // though clearing removes it too.
  //

  size_t cachedCount = 0;
  StringStorage resolved;

  for (size_t i = 0; i < m_remotePlaces.getCount(); i++) {
    if (m_hostState->getResolvedPlace(m_remotePlaces.getPlace(i)->name.getString(),
                                      &resolved)) {
      cachedCount++;
    }
  }

  m_hostState->clearResolvedPlaces();

  StringStorage message;
  message.format(_T("Cleared %u cached location(s) for this server"),
                 static_cast<unsigned int>(cachedCount));
  insertMessageIntoComboBox(message.getString());
}

void FileTransferMainDialog::getPathToCurrentLocalFolder(StringStorage *out)
{
  m_localCurFolderTextBox.getText(out);
}

void FileTransferMainDialog::getPathToParentLocalFolder(StringStorage *out)
{
  getPathToCurrentLocalFolder(out);
  size_t ld = out->findLast(_T('\\'));
  if (ld != (size_t)-1) {
    out->getSubstring(out, 0, ld);  
  } else {
    out->setString(_T(""));
    return;
  }
  if (out->endsWith('\\') && (out->getLength() > 2)) {
    out->getSubstring(out, 0, out->getLength() - 2);
  }
}

void FileTransferMainDialog::getPathToSelectedLocalFile(StringStorage *out)
{
  StringStorage *pathToFile = out;
  getPathToCurrentLocalFolder(pathToFile);

  if (!pathToFile->isEmpty() && !pathToFile->endsWith(_T('\\'))) {
    pathToFile->appendString(_T("\\"));
  }

  const TCHAR *filename = m_localFileListView.getSelectedFileInfo()->getFileName();
  pathToFile->appendString(filename);
}

void FileTransferMainDialog::getPathToCurrentRemoteFolder(StringStorage *out)
{
  m_remoteCurFolderTextBox.getText(out);
}

void FileTransferMainDialog::getPathToParentRemoteFolder(StringStorage *out)
{
  getPathToCurrentRemoteFolder(out);
  size_t ld = out->findLast(_T('/'));
  if (ld != (size_t)-1) {
    out->getSubstring(out, 0, ld);  
  } else {
    out->setString(_T("/"));
    return ;
  }
  if (out->endsWith('/') && (out->getLength() > 2)) {
    out->getSubstring(out, 0, out->getLength() - 2);
  }
}

void FileTransferMainDialog::getPathToSelectedRemoteFile(StringStorage *out)
{
  StringStorage *pathToFile = out;
  getPathToCurrentRemoteFolder(pathToFile);

  if (!pathToFile->endsWith(_T('/'))) {
    pathToFile->appendString(_T("/"));
  }

  const TCHAR *filename = m_remoteFileListView.getSelectedFileInfo()->getFileName();
  pathToFile->appendString(filename);
}

void FileTransferMainDialog::setNothingState()
{
  m_lastReceivedFileListPath = m_lastSentFileListPath;
  m_remoteCurFolderTextBox.setText(m_lastReceivedFileListPath.getString());

  //
  // Only successful listings reach here. FileTransferCore::onUpdateState
  // skips this call when the file list request failed.
  //

  if (m_hostState != 0) {
    m_hostState->setLastRemoteFolder(m_lastReceivedFileListPath.getString());
  }

  m_remoteFileListView.clear();
  vector<FileInfo> *fileRemoteList = m_ftCore->getListRemoteFolder();
  if (!fileRemoteList->empty()) {
    FileInfo *filesInfo = &fileRemoteList->front();
    m_remoteFileListView.addRange(&filesInfo, fileRemoteList->size());
  }

  bool isRoot = m_lastSentFileListPath.isEqualTo(_T("/"));

  // Add fake ".." folder if not root
  if (!isRoot) {
    m_remoteFileListView.addItem(0, m_fakeMoveUpFolder);
  }
}

void FileTransferMainDialog::onFtOpError(const TCHAR *message)
{
  //
  // A candidate failing part way through a chain is not an error, because a
  // later candidate may still work. The chain reports each one as a warning
  // itself, and raises a single error only once every candidate has failed.
  //

  if (isChainReplyExpected()) {
    return;
  }
  insertMessageIntoComboBox(message);
}

void FileTransferMainDialog::onFtOpInfo(const TCHAR *message)
{
  insertMessageIntoComboBox(message);
}

void FileTransferMainDialog::onFtOpStarted()
{
  enableControls(false);
}

void FileTransferMainDialog::onFtOpFinished(int state, int result)
{
  PostMessage(m_ctrlThis.getWindow(), WM_OPERATION_FINISHED, state, result);
}

void FileTransferMainDialog::onRefreshLocalFileList()
{
  refreshLocalFileList();
}
void FileTransferMainDialog::onRefreshRemoteFileList()
{
  refreshRemoteFileList();
}
