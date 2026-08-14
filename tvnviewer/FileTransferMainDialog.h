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

#ifndef _FILE_TRANSFER_MAIN_DIALOG_H_
#define _FILE_TRANSFER_MAIN_DIALOG_H_

#include "gui/BaseDialog.h"
#include "gui/Control.h"
#include "gui/TextBox.h"
#include "gui/ComboBox.h"
#include "gui/ImagedButton.h"
#include "gui/ProgressBar.h"

#include "ft-common/FileInfo.h"

#include "client-config-lib/FtHostState.h"

#include "io-lib/IOException.h"

#include "FileInfoListView.h"
#include "FileExistDialog.h"
#include "thread/Thread.h"
#include "ft-client-lib/FileTransferInterface.h"

#include <vector>

using namespace std;

class FileTransferMainDialog : public BaseDialog,
                               public FileTransferInterface
{
public:
  //
  // Parameters:
  // core - file transfer core driving the operations.
  // hostName - server host as the user typed it, used to key the remembered
  //            folders. May be empty, in which case nothing is remembered.
  //

  FileTransferMainDialog(FileTransferCore *core, const TCHAR *hostName);
  virtual ~FileTransferMainDialog();

  //
  // Kill main dialog.
  // Function return false, if closing dialog is canceled, and true else
  //
  bool tryClose();

  //
  // Inherited from FtInterface
  //
  int onFtTargetFileExists(FileInfo *sourceFileInfo,
                           FileInfo *targetFileInfo,
                           const TCHAR *pathToTargetFile);
  void setProgress(double progress);

  void onFtOpError(const TCHAR *message);
  void onFtOpInfo(const TCHAR *message);
  void onFtOpStarted();
  void onFtOpFinished(int state, int result);

  //
  // filetransfer's operation is finished. Need update of control
  //
  void setNothingState();

  //
  // Called if local file list is updated
  //
  void onRefreshLocalFileList();

  // Called if remote file list is updated
  void onRefreshRemoteFileList();

  //
  // Shows error message and throws exception
  //

  void raise(Exception &ex);

protected:

  //
  // Inherited from BaseDialog
  //

  virtual BOOL onInitDialog();
  virtual BOOL onNotify(UINT controlID, LPARAM data) throw(IOException);
  virtual BOOL onCommand(UINT controlID, UINT notificationID) throw(IOException);
  virtual BOOL onDestroy();

  virtual void onMessageReceived(UINT uMsg, WPARAM wParam, LPARAM lParam);

  //
  // Button event handlers
  //

  void onCancelButtonClick();
  void onCancelOperationButtonClick();

  void onRenameRemoteButtonClick() throw(IOException);
  void onMkDirRemoteButtonClick() throw(IOException);
  void onRemoveRemoteButtonClick() throw(IOException);
  void onRefreshRemoteButtonClick() throw(IOException);

  void onRenameLocalButtonClick();
  void onMkDirLocalButtonClick();
  void onRemoveLocalButtonClick();
  void onRefreshLocalButtonClick();

  void onUploadButtonClick();
  void onDownloadButtonClick();

  void moveUpLocalFolder();
  void moveUpRemoteFolder() throw(IOException);

  //
  // List view event handlers
  //

  void onRemoteListViewDoubleClick() throw(IOException);
  void onLocalListViewDoubleClick();

  void onRemoteListViewKeyDown(UINT key);
  void onLocalListViewKeyDown(UINT key);

  //
  // Enables or disables rename and delete buttons
  // depending of file list views selected items count.
  //

  void checkRemoteListViewSelection();
  void checkLocalListViewSelection();

  //
  // Text notification methods
  //

  void insertMessageIntoComboBox(const TCHAR *message);

private:

  //
  // Enables or disables all controls from m_controlsToBlock list
  //

  void enableControls(bool enabled);

  //
  // Links gui control class members with windows in dialog
  //

  void initControls();


  //
  // Refreshes local file list
  //

  void refreshLocalFileList();

  //
  // Refreshes remote file list
  //

  void refreshRemoteFileList() throw(IOException);

  //
  // Displays file list of pathToFile folder of local machine
  // to local file list view.
  //
  // Returns false if the folder could not be listed. The failure is written
  // to the message combo box and the pane is left as it was.
  //

  bool tryListLocalFolder(const TCHAR *pathToFile);

  //
  // Sends file list request to server and shows result
  // in remote file list view
  //

  void tryListRemoteFolder(const TCHAR *pathToFile) throw(IOException);

  //
  // Sends a file list request on the user's behalf.
  //
  // Cancels any running candidate chain first, so that a reply still in
  // flight cannot move the pane after the user has navigated elsewhere.
  //

  void navigateRemoteFolder(const TCHAR *pathToFile) throw(IOException);

  //
  // Returns each pane to the folder it was showing the last time this host
  // was used, falling back to the pane's root.
  //

  void restoreLocalFolder();
  void restoreRemoteFolder() throw(IOException);

  //
  // Remote candidate chain.
  //
  // Remote listing is asynchronous. tryListRemoteFolder sends a request and
  // returns, and the reply arrives later through onFtOpFinished. So trying
  // several paths until one works cannot be a loop. It is a small state
  // machine driven by those replies.
  //
  // startRemoteChain sends the first candidate. Every failed reply advances
  // to the next. The first success ends the chain. Running out of candidates
  // ends it with a message in the combo box and leaves the pane alone.
  //

  void startRemoteChain(const vector<StringStorage> *candidates,
                        const TCHAR *description) throw(IOException);
  void fireRemoteChainCandidate() throw(IOException);
  void onRemoteChainReply(bool listed) throw(IOException);

  //
  // Stops tracking a chain, whether it succeeded, ran out of candidates, or
  // was superseded by the user navigating somewhere else.
  //

  void endRemoteChain();

  //
  // True while a chain is running and the reply arriving now belongs to it.
  //

  bool isChainReplyExpected() const;

  //
  // Filenames helper methods
  //

  //
  // FIXME: Make classes for getPathTo*** methods
  //

  void getPathToCurrentLocalFolder(StringStorage *out);
  void getPathToParentLocalFolder(StringStorage *out);
  void getPathToSelectedLocalFile(StringStorage *out);

  void getPathToCurrentRemoteFolder(StringStorage *out);
  void getPathToParentRemoteFolder(StringStorage *out);
  void getPathToSelectedRemoteFile(StringStorage *out);

protected:
  //
  // True if window state is closing.
  //

  bool m_isClosing;



  StringStorage m_lastSentFileListPath;
  StringStorage m_lastReceivedFileListPath;

  //
  // Buttons
  //

  Control m_renameRemoteButton;
  Control m_mkDirRemoteButton;
  Control m_removeRemoteButton;
  Control m_refreshRemoteButton;

  Control m_renameLocalButton;
  Control m_mkDirLocalButton;
  Control m_removeLocalButton;
  Control m_refreshLocalButton;

  Control m_uploadButton;
  Control m_downloadButton;

  Control m_cancelButton;

  //
  // Progress bar
  //

  ProgressBar m_copyProgressBar;

  //
  // Combo box
  //

  ComboBox m_logComboBox;

  //
  // Text boxes
  //

  TextBox m_localCurFolderTextBox;
  TextBox m_remoteCurFolderTextBox;

  //
  // Tables
  //

  FileInfoListView m_localFileListView;
  FileInfoListView m_remoteFileListView;

  //
  // Helper modal dialogs
  //

  FileExistDialog m_fileExistDialog;

  //
  // File info of ".." fake folder
  //

  FileInfo *m_fakeMoveUpFolder;

  //
  // Remembered folders for the connected host.
  //
  // Null when the host name is not known, in which case the dialog opens at
  // the roots and remembers nothing.
  //

  FtHostState *m_hostState;

  //
  // Remote candidate chain state. See startRemoteChain.
  //

  vector<StringStorage> m_chainCandidates;
  size_t m_chainIndex;
  bool m_chainActive;
  StringStorage m_chainDescription;

  //
  // Guards a reply from an abandoned chain against advancing a live one.
  //
  // m_chainGeneration is bumped whenever a chain starts or is cancelled, and
  // captured into m_chainFiredGeneration each time a candidate goes out. A
  // reply counts only while the two still agree.
  //

  UINT m_chainGeneration;
  UINT m_chainFiredGeneration;

private:

  static const UINT WM_OPERATION_FINISHED = WM_USER + 2;
};

#endif
