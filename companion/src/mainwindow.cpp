/*
 * Copyright (C) OpenTX
 *
 * Based on code named
 *   th9x - http://code.google.com/p/th9x
 *   er9x - http://code.google.com/p/er9x
 *   gruvin9x - http://code.google.com/p/gruvin9x
 *
 * License GPLv2: http://www.gnu.org/licenses/gpl-2.0.html
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "mainwindow.h"
#include "mdichild.h"
#include "burnconfigdialog.h"
#include "comparedialog.h"
#include "logsdialog.h"
#include "apppreferencesdialog.h"
#include "fwpreferencesdialog.h"
#include "firmwareinterface.h"
#include "fusesdialog.h"
#include "downloaddialog.h"
#include "printdialog.h"
#include "version.h"
#include "contributorsdialog.h"
#include "releasenotesdialog.h"
#include "releasenotesfirmwaredialog.h"
#include "customizesplashdialog.h"
#include "flasheepromdialog.h"
#include "flashfirmwaredialog.h"
#include "hexinterface.h"
#include "warnings.h"
#include "helpers.h"
#include "appdata.h"
#include "radionotfound.h"
#include "process_sync.h"
#include "radiointerface.h"
#include "progressdialog.h"
#include "storage.h"
#include "translations.h"

#include <QtGui>
#include <QNetworkProxyFactory>
#include <QFileInfo>
#include <QDesktopServices>

#define OPENTX_COMPANION_DOWNLOADS        "http://downloads-22.open-tx.org/companion"
#define DONATE_STR                        "https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=QUZ48K4SEXDP2"

#ifdef __APPLE__
  #define COMPANION_STAMP                 "companion-macosx.stamp"
  #define COMPANION_INSTALLER             "companion-macosx-%1.dmg"
#else
  #define COMPANION_STAMP                 "companion-windows.stamp"
  #define COMPANION_INSTALLER             "companion-windows-%1.exe"
#endif

#ifdef WIN32
  #define OPENTX_NIGHT_COMPANION_DOWNLOADS  "http://downloads-22.open-tx.org/nightly/companion/windows"
#else
  #define OPENTX_NIGHT_COMPANION_DOWNLOADS  "http://downloads-22.open-tx.org/nightly/companion/linux"
#endif

MainWindow::MainWindow():
  downloadDialog_forWait(NULL),
  checkForUpdatesState(0),
  windowsListActions(new QActionGroup(this))
{
  // setUnifiedTitleAndToolBarOnMac(true);
  this->setWindowIcon(QIcon(":/icon.png"));
  QNetworkProxyFactory::setUseSystemConfiguration(true);
  setAcceptDrops(true);

  mdiArea = new QMdiArea(this);
  mdiArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  mdiArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  mdiArea->setTabsClosable(true);
  mdiArea->setTabsMovable(true);
  mdiArea->setDocumentMode(true);
  connect(mdiArea, &QMdiArea::subWindowActivated, this, &MainWindow::updateMenus);

  setCentralWidget(mdiArea);

  createActions();
  createMenus();
  createToolBars();
  retranslateUi();
  updateMenus();

  setIconThemeSize(g.iconSize());
  restoreGeometry(g.mainWinGeo());
  restoreState(g.mainWinState());
  setTabbedWindows(g.tabbedMdi());

  connect(windowsListActions, &QActionGroup::triggered, this, &MainWindow::onChangeWindowAction);

  // give time to the splash to disappear and main window to open before starting updates
  int updateDelay = 1000;
  bool showSplash = g.showSplash();
  if (showSplash) {
    updateDelay += (SPLASH_TIME*1000);
  }
  QTimer::singleShot(updateDelay, this, SLOT(doAutoUpdates()));
  QTimer::singleShot(updateDelay, this, SLOT(displayWarnings()));

  QStringList strl = QApplication::arguments();
  QString str;
  QString printfilename;
  int printing = strl.contains("--print");
  int model = -1;
  int count = 0;
  foreach(QString arg, strl) {
    count++;
    if (arg.contains("--model")) {
      model = strl[count].toInt() - 1;
    }
    if (arg.contains("--filename")) {
      printfilename = strl[count];
    }
  }
  if (strl.count() > 1)
    str = strl[1];
  if (!str.isEmpty()) {
    int fileType = getStorageType(str);

    if (fileType==STORAGE_TYPE_EEPE || fileType==STORAGE_TYPE_EEPM || fileType==STORAGE_TYPE_BIN || fileType==STORAGE_TYPE_OTX) {
      MdiChild * child = createMdiChild();
      if (child->loadFile(str)) {
        if (!(printing && model >= 0 && (getCurrentFirmware()->getCapability(Models) == 0 || model<getCurrentFirmware()->getCapability(Models)) && !printfilename.isEmpty())) {
          statusBar()->showMessage(tr("File loaded"), 2000);
          child->show();
        }
        else {
          child->show();
          child->print(model, printfilename);
          child->close();
        }
      }
    }
  }
  if (printing) {
    QTimer::singleShot(0, this, SLOT(autoClose()));
  }
}

MainWindow::~MainWindow()
{
  if (windowsListActions) {
    delete windowsListActions;
    windowsListActions = NULL;
  }
}

void MainWindow::displayWarnings()
{
  int warnId = g.warningId();
  if (warnId<WARNING_ID && warnId!=0) {
    int res=0;
    if (WARNING_LEVEL>0)
      QMessageBox::warning(this, "Companion", WARNING);
    else
      QMessageBox::about(this, "Companion", WARNING);
    res = QMessageBox::question(this, "Companion", tr("Display previous warning again at startup ?"), QMessageBox::Yes | QMessageBox::No);
    if (res == QMessageBox::No) {
      g.warningId(WARNING_ID);
    }
  }
  else if (warnId==0) {
    g.warningId(WARNING_ID);
  }
}

void MainWindow::doAutoUpdates()
{
  if (g.autoCheckApp())
    checkForUpdatesState |= CHECK_COMPANION;
  if (g.autoCheckFw())
    checkForUpdatesState |= CHECK_FIRMWARE;
  checkForUpdates();
}

void MainWindow::doUpdates()
{
  checkForUpdatesState = CHECK_COMPANION | CHECK_FIRMWARE | SHOW_DIALOG_WAIT;
  checkForUpdates();
}

void MainWindow::checkForFirmwareUpdate()
{
  checkForUpdatesState = CHECK_FIRMWARE | SHOW_DIALOG_WAIT;
  checkForUpdates();
}

void MainWindow::dowloadLastFirmwareUpdate()
{
  checkForUpdatesState = CHECK_FIRMWARE | AUTOMATIC_DOWNLOAD | SHOW_DIALOG_WAIT;
  checkForUpdates();
}

QString MainWindow::getCompanionUpdateBaseUrl()
{
#if defined(ALLOW_NIGHTLY_BUILDS)
  return g.useCompanionNightlyBuilds() ? OPENTX_NIGHT_COMPANION_DOWNLOADS : OPENTX_COMPANION_DOWNLOADS;
#else
  return OPENTX_COMPANION_DOWNLOADS;
#endif
}

void MainWindow::checkForUpdates()
{
  if (checkForUpdatesState & SHOW_DIALOG_WAIT) {
    checkForUpdatesState -= SHOW_DIALOG_WAIT;
    downloadDialog_forWait = new downloadDialog(NULL, tr("Checking for updates"));
    downloadDialog_forWait->show();
  }

  if (checkForUpdatesState & CHECK_COMPANION) {
    checkForUpdatesState -= CHECK_COMPANION;
    // TODO why create each time a network manager?
    networkManager = new QNetworkAccessManager(this);
    connect(networkManager, SIGNAL(finished(QNetworkReply*)),this, SLOT(checkForCompanionUpdateFinished(QNetworkReply*)));
    QUrl url(QString("%1/%2").arg(getCompanionUpdateBaseUrl()).arg(COMPANION_STAMP));
    qDebug() << "Checking for Companion update " << url.url();
    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);
    networkManager->get(request);
  }
  else if (checkForUpdatesState & CHECK_FIRMWARE) {
    checkForUpdatesState -= CHECK_FIRMWARE;
    QString stamp = getCurrentFirmware()->getStampUrl();
    if (!stamp.isEmpty()) {
      networkManager = new QNetworkAccessManager(this);
      connect(networkManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(checkForFirmwareUpdateFinished(QNetworkReply*)));
      QUrl url(stamp);
      qDebug() << "Checking for firmware update " << url.url();
      QNetworkRequest request(url);
      request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);
      networkManager->get(request);
    }
  }
  else if (checkForUpdatesState==0) {
    closeUpdatesWaitDialog();
  }
}

void MainWindow::onUpdatesError()
{
  checkForUpdatesState = 0;
  closeUpdatesWaitDialog();
  QMessageBox::warning(this, "Companion", tr("Unable to check for updates."));
}

void MainWindow::closeUpdatesWaitDialog()
{
  if (downloadDialog_forWait) {
    downloadDialog_forWait->close();
    delete downloadDialog_forWait;
    downloadDialog_forWait = NULL;
  }
}

QString MainWindow::seekCodeString(const QByteArray & qba, const QString & label)
{
  int posLabel = qba.indexOf(label);
  if (posLabel < 0)
    return QString::null;
  int start = qba.indexOf("\"", posLabel + label.length());
  if (start < 0)
    return QString::null;
  int end = qba.indexOf("\"", start + 1);
  if (end < 0)
    return QString::null;
  return qba.mid(start + 1, end - start - 1);
}

void MainWindow::checkForCompanionUpdateFinished(QNetworkReply * reply)
{
  QByteArray qba = reply->readAll();
  QString version = seekCodeString(qba, "VERSION");
  if (version.isNull())
    return onUpdatesError();

  int vnum = version2index(version);

  QString c9xversion = QString(VERSION);
  int c9xver = version2index(c9xversion);

  if (c9xver < vnum) {
#if defined WIN32 || !defined __GNUC__ // || defined __APPLE__  // OSX should only notify of updates since no update packages are available.
    int ret = QMessageBox::question(this, "Companion", tr("A new version of Companion is available (version %1)<br>"
                                                        "Would you like to download it?").arg(version) ,
                                    QMessageBox::Yes | QMessageBox::No);

    if (ret == QMessageBox::Yes) {
      QDir dir(g.updatesDir());
      QString fileName = QFileDialog::getSaveFileName(this, tr("Save As"), dir.absoluteFilePath(QString(COMPANION_INSTALLER).arg(version)), tr("Executable (*.exe)"));
      if (!fileName.isEmpty()) {
        g.updatesDir(QFileInfo(fileName).dir().absolutePath());
        downloadDialog * dd = new downloadDialog(this, QString("%1/%2").arg(getCompanionUpdateBaseUrl()).arg(QString(COMPANION_INSTALLER).arg(version)), fileName);
        installer_fileName = fileName;
        connect(dd, SIGNAL(accepted()), this, SLOT(updateDownloaded()));
        dd->exec();
      }
    }
#else
    QMessageBox::warning(this, tr("New release available"), tr("A new release of Companion is available, please check the OpenTX website!"));
#endif
  }
  else {
    if (downloadDialog_forWait && checkForUpdatesState==0) {
      QMessageBox::information(this, "Companion", tr("No updates available at this time."));
    }
  }

  checkForUpdates();
}

void MainWindow::updateDownloaded()
{
  int ret = QMessageBox::question(this, "Companion", tr("Would you like to launch the installer?") ,
                                   QMessageBox::Yes | QMessageBox::No);
  if (ret == QMessageBox::Yes) {
    if (QDesktopServices::openUrl(QUrl::fromLocalFile(installer_fileName)))
      QApplication::exit();
  }
}

void MainWindow::firmwareDownloadAccepted()
{
  QString errormsg;
  QFile file(g.profile[g.id()].fwName());
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {  //reading HEX TEXT file
    QMessageBox::critical(this, tr("Error"),
        tr("Error opening file %1:\n%2.")
        .arg(g.profile[g.id()].fwName())
        .arg(file.errorString()));
    return;
  }
  file.reset();
  QTextStream inputStream(&file);
  QString hline = inputStream.readLine();
  if (hline.startsWith("ERROR:")) {
    int errnum = hline.mid(6).toInt();
    switch(errnum) {
      case 1:
        errormsg = tr("Not enough flash available on this board for all the selected options");
        break;
      case 2:
        errormsg = tr("Compilation server temporary failure, try later");
        break;
      case 3:
        errormsg = tr("Compilation server too busy, try later");
        break;
      case 4:
        errormsg = tr("Compilation error");
        break;
      case 5:
        errormsg = tr("Invalid firmware");
        break;
      case 6:
        errormsg = tr("Invalid board");
        break;
      case 7:
        errormsg = tr("Invalid language");
        break;
      default:
        errormsg = tr("Unknown server failure, try later");
        break;
    }
    file.close();
    file.remove();
    QMessageBox::critical(this, tr("Error"), errormsg);
    return;
  }
  file.close();
  g.fwRev.set(Firmware::getCurrentVariant()->getId(), version2index(firmwareVersionString));
  if (g.profile[g.id()].burnFirmware()) {
    int ret = QMessageBox::question(this, "Companion", tr("Do you want to write the firmware to the radio now ?"), QMessageBox::Yes | QMessageBox::No);
    if (ret == QMessageBox::Yes) {
      writeFlash(g.profile[g.id()].fwName());
    }
  }
}

void MainWindow::checkForFirmwareUpdateFinished(QNetworkReply * reply)
{
  bool download = false;
  bool ignore = false;

  QByteArray qba = reply->readAll();
  QString versionString = seekCodeString(qba, "VERSION");
  QString dateString = seekCodeString(qba, "DATE");
  if (versionString.isNull() || dateString.isNull())
    return onUpdatesError();

  long version = version2index(versionString);
  if (version <= 0)
    return onUpdatesError();

  QString fullVersionString = QString("%1 (%2)").arg(versionString).arg(dateString);

  if (checkForUpdatesState & AUTOMATIC_DOWNLOAD) {
    checkForUpdatesState -= AUTOMATIC_DOWNLOAD;
    download = true;
  }
  else {
    int currentVersion = g.fwRev.get(Firmware::getCurrentVariant()->getId());
    QString currentVersionString = index2version(currentVersion);

    QMessageBox msgBox;
    msgBox.setWindowTitle("Companion");
    QSpacerItem * horizontalSpacer = new QSpacerItem(500, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);
    QGridLayout * layout = (QGridLayout*)msgBox.layout();
    layout->addItem(horizontalSpacer, layout->rowCount(), 0, 1, layout->columnCount());

    if (currentVersion == 0) {
      QString rn = getCurrentFirmware()->getReleaseNotesUrl();
      QAbstractButton *rnButton = NULL;
      msgBox.setText(tr("Firmware %1 does not seem to have ever been downloaded.\nRelease %2 is available.\nDo you want to download it now?\n\nWe recommend you view the release notes using the button below to learn about any changes that may be important to you.")
                     .arg(Firmware::getCurrentVariant()->getId()).arg(fullVersionString));
      QAbstractButton *YesButton = msgBox.addButton(trUtf8("Yes"), QMessageBox::YesRole);
      msgBox.addButton(trUtf8("No"), QMessageBox::NoRole);
      if (!rn.isEmpty()) {
        rnButton = msgBox.addButton(trUtf8("Release Notes"), QMessageBox::ActionRole);
      }
      msgBox.setIcon(QMessageBox::Question);
      msgBox.resize(0, 0);
      msgBox.exec();
      if (msgBox.clickedButton() == rnButton) {
        ReleaseNotesFirmwareDialog * dialog = new ReleaseNotesFirmwareDialog(this, rn);
        dialog->exec();
        int ret2 = QMessageBox::question(this, "Companion", tr("Do you want to download release %1 now ?").arg(fullVersionString), QMessageBox::Yes | QMessageBox::No);
        if (ret2 == QMessageBox::Yes)
          download = true;
        else
          ignore = true;
      }
      else if (msgBox.clickedButton() == YesButton ) {
        download = true;
      }
      else {
        ignore = true;
      }
    }
    else if (version > currentVersion) {
      QString rn = getCurrentFirmware()->getReleaseNotesUrl();
      QAbstractButton *rnButton = NULL;
      msgBox.setText(tr("A new version of %1 firmware is available:\n  - current is %2\n  - newer is %3\n\nDo you want to download it now?\n\nWe recommend you view the release notes using the button below to learn about any changes that may be important to you.")
                     .arg(Firmware::getCurrentVariant()->getId()).arg(currentVersionString).arg(fullVersionString));
      QAbstractButton *YesButton = msgBox.addButton(trUtf8("Yes"), QMessageBox::YesRole);
      msgBox.addButton(trUtf8("No"), QMessageBox::NoRole);
      if (!rn.isEmpty()) {
        rnButton = msgBox.addButton(trUtf8("Release Notes"), QMessageBox::ActionRole);
      }
      msgBox.setIcon(QMessageBox::Question);
      msgBox.resize(0,0);
      msgBox.exec();
      if( msgBox.clickedButton() == rnButton ) {
        ReleaseNotesFirmwareDialog * dialog = new ReleaseNotesFirmwareDialog(this, rn);
        dialog->exec();
        int ret2 = QMessageBox::question(this, "Companion", tr("Do you want to download release %1 now ?").arg(fullVersionString),
              QMessageBox::Yes | QMessageBox::No);
        if (ret2 == QMessageBox::Yes) {
          download = true;
        }
        else {
          ignore = true;
        }
      }
      else if (msgBox.clickedButton() == YesButton ) {
        download = true;
      }
      else {
        ignore = true;
      }
    }
    else {
      if (downloadDialog_forWait && checkForUpdatesState==0) {
        QMessageBox::information(this, "Companion", tr("No updates available at this time."));
      }
    }
  }

  if (ignore) {
    int res = QMessageBox::question(this, "Companion", tr("Ignore this release %1?").arg(fullVersionString), QMessageBox::Yes | QMessageBox::No);
    if (res==QMessageBox::Yes)   {
      g.fwRev.set(Firmware::getCurrentVariant()->getId(), version);
    }
  }
  else if (download == true) {
    firmwareVersionString = versionString;
    startFirmwareDownload();
  }

  checkForUpdates();
}

void MainWindow::startFirmwareDownload()
{
  QString url = Firmware::getCurrentVariant()->getFirmwareUrl();
  qDebug() << "Downloading firmware" << url;
  QString ext = url.mid(url.lastIndexOf("."));
  QString defaultFilename = g.flashDir() + "/" + Firmware::getCurrentVariant()->getId();
  if (g.profile[g.id()].renameFwFiles()) {
    defaultFilename += "-" + firmwareVersionString;
  }
  defaultFilename += ext;

  QString filename = QFileDialog::getSaveFileName(this, tr("Save As"), defaultFilename);
  if (!filename.isEmpty()) {
    g.profile[g.id()].fwName(filename);
    g.flashDir(QFileInfo(filename).dir().absolutePath());
    downloadDialog * dd = new downloadDialog(this, url, filename);
    connect(dd, SIGNAL(accepted()), this, SLOT(firmwareDownloadAccepted()));
    dd->exec();
  }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
  g.mainWinGeo(saveGeometry());
  g.mainWinState(saveState());
  g.tabbedMdi(actTabbedWindows->isChecked());
  mdiArea->closeAllSubWindows();
  if (mdiArea->currentSubWindow()) {
    event->ignore();
  }
  else {
    event->accept();
  }
}

void MainWindow::changeEvent(QEvent * e)
{
  QMainWindow::changeEvent(e);
  switch (e->type()) {
    case QEvent::LanguageChange:
      retranslateUi(true);
      break;
    default:
      break;
  }
}

void MainWindow::setLanguage(const QString & langString)
{
  g.locale(langString);
  Translations::installTranslators();
}

void MainWindow::onLanguageChanged(QAction * act)
{
  QString lang = act->property("locale").toString();
  if (!lang.isNull())
    setLanguage(lang);
}

void  MainWindow::setTheme(int index)
{
  g.theme(index);
  QMessageBox::information(this, tr("Companion"), tr("The new theme will be loaded the next time you start Companion."));
}

void MainWindow::onThemeChanged(QAction * act)
{
  bool ok;
  int id = act->property("themeId").toInt(&ok);
  if (ok && id >= 0 && id < 5)
    setTheme(id);
}

void  MainWindow::setIconThemeSize(int index)
{
  if (index != g.iconSize())
    g.iconSize(index);

  QSize size;
  switch(g.iconSize()) {
    case 0:
      size=QSize(16,16);
      break;
    case 2:
      size=QSize(32,32);
      break;
    case 3:
      size=QSize(48,48);
      break;
    case 1:
    default:
      size=QSize(24,24);
      break;
  }
  this->setIconSize(size);
}

void MainWindow::onIconSizeChanged(QAction * act)
{
  bool ok;
  int id = act->property("sizeId").toInt(&ok);
  if (ok && id >= 0 && id < 4)
    setIconThemeSize(id);
}

void MainWindow::setTabbedWindows(bool on)
{
  mdiArea->setViewMode(on ? QMdiArea::TabbedView : QMdiArea::SubWindowView);
  if (actTileWindows)
    actTileWindows->setDisabled(on);
  if (actCascadeWindows)
    actCascadeWindows->setDisabled(on);

  if (actTabbedWindows->isChecked() != on)
    actTabbedWindows->setChecked(on);
}

void MainWindow::newFile()
{
  MdiChild * child = createMdiChild();
  child->newFile();
  child->show();
}

void MainWindow::openDocURL()
{
  QString link = "http://www.open-tx.org/documents.html";
  QDesktopServices::openUrl(QUrl(link));
}

void MainWindow::openFile(const QString & fileName, bool updateLastUsedDir)
{
  if (!fileName.isEmpty()) {
    if (updateLastUsedDir) {
      g.eepromDir(QFileInfo(fileName).dir().absolutePath());
    }

    QMdiSubWindow *existing = findMdiChild(fileName);
    if (existing) {
      mdiArea->setActiveSubWindow(existing);
      return;
    }

    MdiChild *child = createMdiChild();
    if (child->loadFile(fileName)) {
      statusBar()->showMessage(tr("File loaded"), 2000);
      child->show();
    }
  }
}

void MainWindow::openFile()
{
  QString fileFilter;
  fileFilter = tr(EEPROM_FILES_FILTER);
  QString fileName = QFileDialog::getOpenFileName(this, tr("Open Models and Settings file"), g.eepromDir(), fileFilter);
  openFile(fileName);
}

void MainWindow::save()
{
  if (activeMdiChild() && activeMdiChild()->save()) {
    statusBar()->showMessage(tr("File saved"), 2000);
  }
}

void MainWindow::saveAs()
{
  if (activeMdiChild() && activeMdiChild()->saveAs()) {
    statusBar()->showMessage(tr("File saved"), 2000);
  }
}

void MainWindow::closeFile()
{
  if (mdiArea->activeSubWindow())
    mdiArea->activeSubWindow()->close();
}

void MainWindow::openRecentFile()
{
  QAction *action = qobject_cast<QAction *>(sender());
  if (action) {
    QString fileName = action->data().toString();
    openFile(fileName, false);
  }
}

void MainWindow::loadProfileId(const unsigned pid)  // TODO Load all variables - Also HW!
{
  if (pid >= MAX_PROFILES)
    return;

  // Set the new profile number
  g.id(pid);
  Firmware::setCurrentVariant(Firmware::getFirmwareForId(g.profile[pid].fwType()));
  emit firmwareChanged();
  updateMenus();
}

void MainWindow::loadProfile()
{
  QAction * action = qobject_cast<QAction *>(sender());
  if (action) {
    bool ok = false;
    unsigned profnum = action->data().toUInt(&ok);
    if (ok)
      loadProfileId(profnum);
  }
}

void MainWindow::appPrefs()
{
  AppPreferencesDialog * dialog = new AppPreferencesDialog(this);
  connect(dialog, &AppPreferencesDialog::firmwareProfileChanged, this, &MainWindow::loadProfileId);
  dialog->exec();
  dialog->deleteLater();
}

void MainWindow::fwPrefs()
{
  FirmwarePreferencesDialog * dialog = new FirmwarePreferencesDialog(this);
  dialog->exec();
  dialog->deleteLater();
}

void MainWindow::contributors()
{
  ContributorsDialog * dialog = new ContributorsDialog(this);
  dialog->exec();
  dialog->deleteLater();
}

void MainWindow::sdsync()
{
  QString sdPath = g.profile[g.id()].sdPath();
  if (sdPath.isEmpty()) {
    QMessageBox::warning(this, QObject::tr("Synchronization error"), QObject::tr("No SD directory configured!"));
    return;
  }
  QString massstoragePath = findMassstoragePath("SOUNDS");
  if (massstoragePath.isEmpty()) {
    QMessageBox::warning(this, QObject::tr("Synchronization error"), QObject::tr("No Radio connected!"));
    return;
  }
  massstoragePath = massstoragePath.left(massstoragePath.length() - 7);
  ProgressDialog progressDialog(this, tr("Synchronize SD"), CompanionIcon("sdsync.png"));
  SyncProcess syncProcess(massstoragePath, g.profile[g.id()].sdPath(), progressDialog.progress());
  if (!syncProcess.run()) {
    progressDialog.exec();
  }
}

void MainWindow::changelog()
{
  ReleaseNotesDialog * dialog = new ReleaseNotesDialog(this);
  dialog->exec();
  dialog->deleteLater();
}

void MainWindow::fwchangelog()
{
  Firmware * firmware = getCurrentFirmware();
  QString url = firmware->getReleaseNotesUrl();
  if (url.isEmpty()) {
    QMessageBox::information(this, tr("Firmware updates"), tr("Current firmware does not provide release notes informations."));
  }
  else {
    ReleaseNotesFirmwareDialog * dialog = new ReleaseNotesFirmwareDialog(this, url);
    dialog->exec();
    dialog->deleteLater();
  }
}

void MainWindow::customizeSplash()
{
  customizeSplashDialog * dialog = new customizeSplashDialog(this);
  dialog->exec();
  dialog->deleteLater();
}

void MainWindow::writeEeprom()
{
  if (activeMdiChild()) activeMdiChild()->writeEeprom();
}

void MainWindow::readEeprom()
{
  Board::Type board = getCurrentBoard();
  QString tempFile;
  if (IS_HORUS(board))
    tempFile = generateProcessUniqueTempFileName("temp.otx");
  else if (IS_ARM(board))
    tempFile = generateProcessUniqueTempFileName("temp.bin");
  else
    tempFile = generateProcessUniqueTempFileName("temp.hex");

  qDebug() << "MainWindow::readEeprom(): using temp file: " << tempFile;

  if (readEepromFromRadio(tempFile)) {
    MdiChild * child = createMdiChild();
    child->newFile();
    child->loadFile(tempFile, false);
    child->show();
    qunlink(tempFile);
  }
}

bool MainWindow::readFirmwareFromRadio(const QString & filename)
{
  ProgressDialog progressDialog(this, tr("Read Firmware from Radio"), CompanionIcon("read_flash.png"));
  bool result = readFirmware(filename, progressDialog.progress());
  if (!result && !progressDialog.isEmpty()) {
    progressDialog.exec();
  }
  return result;
}

bool MainWindow::readEepromFromRadio(const QString & filename)
{
  ProgressDialog progressDialog(this, tr("Read Models and Settings from Radio"), CompanionIcon("read_eeprom.png"));
  bool result = ::readEeprom(filename, progressDialog.progress());
  if (!result) {
    if (!progressDialog.isEmpty()) {
      progressDialog.exec();
    }
  }
  else {
    statusBar()->showMessage(tr("Models and Settings read"), 2000);
  }
  return result;
}

void MainWindow::writeBackup()
{
  if (IS_HORUS(getCurrentBoard())) {
    QMessageBox::information(this, "Companion", tr("This function is not yet implemented"));
    return;
    // TODO implementation
  }
  FlashEEpromDialog *cd = new FlashEEpromDialog(this);
  cd->exec();
}

void MainWindow::writeFlash(QString fileToFlash)
{
  FlashFirmwareDialog * cd = new FlashFirmwareDialog(this);
  cd->exec();
}

void MainWindow::readBackup()
{
  if (IS_HORUS(getCurrentBoard())) {
    QMessageBox::information(this, "Companion", tr("This function is not yet implemented"));
    return;
    // TODO implementation
  }
  QString fileName = QFileDialog::getSaveFileName(this, tr("Save Radio Backup to File"), g.eepromDir(), tr(EXTERNAL_EEPROM_FILES_FILTER));
  if (!fileName.isEmpty()) {
    if (!readEepromFromRadio(fileName))
      return;
  }
}

void MainWindow::readFlash()
{
  QString fileName = QFileDialog::getSaveFileName(this,tr("Read Radio Firmware to File"), g.flashDir(),tr(FLASH_FILES_FILTER));
  if (!fileName.isEmpty()) {
    readFirmwareFromRadio(fileName);
  }
}

void MainWindow::burnConfig()
{
  burnConfigDialog *bcd = new burnConfigDialog(this);
  bcd->exec();
  delete bcd;
}

void MainWindow::burnList()
{
  burnConfigDialog bcd(this);
  bcd.listAvrdudeProgrammers();
}

void MainWindow::burnFuses()
{
  FusesDialog *fd = new FusesDialog(this);
  fd->exec();
  delete fd;
}

void MainWindow::compare()
{
  CompareDialog *fd = new CompareDialog(this,getCurrentFirmware());
  fd->setAttribute(Qt::WA_DeleteOnClose, true);
  fd->show();
}

void MainWindow::logFile()
{
  LogsDialog *fd = new LogsDialog(this);
  fd->setWindowFlags(Qt::Window);   //to show minimize an maximize buttons
  fd->setAttribute(Qt::WA_DeleteOnClose, true);
  fd->show();
}

void MainWindow::about()
{
  QString aboutStr = "<center><img src=\":/images/companion-title.png\"></center><br/>";
  aboutStr.append(tr("OpenTX Home Page: <a href='%1'>%1</a>").arg("http://www.open-tx.org"));
  aboutStr.append("<br/><br/>");
  aboutStr.append(tr("The OpenTX Companion project was originally forked from <a href='%1'>eePe</a>").arg("http://code.google.com/p/eepe"));
  aboutStr.append("<br/><br/>");
  aboutStr.append(tr("If you've found this program useful, please support by <a href='%1'>donating</a>").arg(DONATE_STR));
  aboutStr.append("<br/><br/>");
  aboutStr.append(QString("Version %1, %2").arg(VERSION).arg(__DATE__));
  aboutStr.append("<br/><br/>");
  aboutStr.append(tr("Copyright OpenTX Team") + "<br/>&copy; 2011-2017<br/>");
  QMessageBox msgBox(this);
  msgBox.setWindowIcon(CompanionIcon("information.png"));
  msgBox.setWindowTitle(tr("About Companion"));
  msgBox.setText(aboutStr);
  msgBox.exec();
}

void MainWindow::updateMenus()
{
  QMdiSubWindow * activeChild = mdiArea->activeSubWindow();

  newAct->setEnabled(true);
  openAct->setEnabled(true);
  saveAct->setEnabled(activeChild);
  saveAsAct->setEnabled(activeChild);
  closeAct->setEnabled(activeChild);
  compareAct->setEnabled(activeChild);
  writeEepromAct->setEnabled(activeChild);
  readEepromAct->setEnabled(true);
  writeBUToRadioAct->setEnabled(true);
  readBUToFileAct->setEnabled(true);

  foreach (QAction * act, fileWindowActions) {
    if (!act)
      continue;
    if (fileMenu && fileMenu->actions().contains(act))
      fileMenu->removeAction(act);
    if (fileToolBar && fileToolBar->actions().contains(act)) {
      fileToolBar->removeAction(act);
    }
    if (act->isSeparator() && act->parent() == this)
      delete act;
  }
  fileWindowActions.clear();
  if (activeChild) {
    editMenu->clear();
    editMenu->addActions(activeMdiChild()->getEditActions());
    editMenu->addSeparator();
    editMenu->addActions(activeMdiChild()->getModelActions());  // maybe separate menu/toolbar?
    editMenu->setEnabled(true);

    editToolBar->clear();
    editToolBar->addActions(activeMdiChild()->getEditActions());
    editToolBar->setEnabled(true);
    if (activeMdiChild()->getAction(MdiChild::ACT_MDL_MOV)) {
      // workaround for default split button appearance of action with menu  :-/
      QToolButton * btn;
      if ((btn = qobject_cast<QToolButton *>(editToolBar->widgetForAction(activeMdiChild()->getAction(MdiChild::ACT_MDL_MOV)))))
        btn->setPopupMode(QToolButton::InstantPopup);
    }

    fileWindowActions = activeMdiChild()->getGeneralActions();
    QAction *sep = new QAction(this);
    sep->setSeparator(true);
    fileWindowActions.append(sep);
    fileMenu->insertActions(logsAct, fileWindowActions);
    fileToolBar->insertActions(logsAct, fileWindowActions);
  }
  else {
    editToolBar->setDisabled(true);
    editMenu->setDisabled(true);
  }

  foreach (QAction * act, windowsListActions->actions()) {
    if (act->property("window_ptr").canConvert<QMdiSubWindow *>() &&
        act->property("window_ptr").value<QMdiSubWindow *>() == activeChild) {
      act->setChecked(true);
      break;
    }
  }

  updateRecentFileActions();
  updateProfilesActions();
  setWindowTitle(tr("OpenTX Companion %1 - Radio: %2 - Profile: %3").arg(VERSION).arg(getCurrentFirmware()->getName()).arg(g.profile[g.id()].name()));
}

MdiChild * MainWindow::createMdiChild()
{
  QMdiSubWindow * win = new QMdiSubWindow();
  MdiChild * child = new MdiChild(this, win);
  win->setAttribute(Qt::WA_DeleteOnClose);
  win->setWidget(child);
  mdiArea->addSubWindow(win);
  if (g.mdiWinGeo().size() < 10 && g.mdiWinGeo() == "maximized")
    win->showMaximized();

  connect(this, &MainWindow::firmwareChanged, child, &MdiChild::onFirmwareChanged);
  connect(child, &MdiChild::windowTitleChanged, this, &MainWindow::onSubwindowTitleChanged);
  connect(child, &MdiChild::modified, this, &MainWindow::onSubwindowModified);
  connect(child, &MdiChild::newStatusMessage, statusBar(), &QStatusBar::showMessage);
  connect(win, &QMdiSubWindow::destroyed, this, &MainWindow::updateWindowActions);

  updateWindowActions();
  return child;
}

QAction * MainWindow::addAct(const QString & icon, const char *slot, const QKeySequence & shortcut, QObject *slotObj, const char * signal)
{
  QAction * newAction = new QAction( this );
  newAction->setMenuRole(QAction::NoRole);
  if (!icon.isEmpty())
    newAction->setIcon(CompanionIcon(icon));
  if (!shortcut.isEmpty())
    newAction->setShortcut(shortcut);
  if (slot) {
    if (!slotObj)
      slotObj = this;
    if (!signal)
      connect(newAction, SIGNAL(triggered()), slotObj, slot);
    else
      connect(newAction, signal, slotObj, slot);
  }
  return newAction;
}

QAction * MainWindow::addActToGroup(QActionGroup * aGroup, const QString & sName, const QString & lName, const char * propName, const QVariant & propValue, const QVariant & dfltValue, const QKeySequence & shortcut)
{
  QAction * act = aGroup->addAction(sName);
  act->setMenuRole(QAction::NoRole);
  act->setStatusTip(lName);
  act->setCheckable(true);
  if (!shortcut.isEmpty())
    act->setShortcut(shortcut);
  if (propName) {
    act->setProperty(propName, propValue);
    if (propValue == dfltValue)
      act->setChecked(true);
  }
  return act;
}

void MainWindow::trAct(QAction * act, const QString & text, const QString & descript)
{
  if (!text.isEmpty())
    act->setText(text);
  if (!descript.isEmpty())
    act->setStatusTip(descript);
}

void MainWindow::retranslateUi(bool showMsg)
{
  trAct(newAct,    tr("New"),        tr("Create a new Models and Settings file"));
  trAct(openAct,   tr("Open..."),    tr("Open Models and Settings file"));
  trAct(saveAct,   tr("Save"),       tr("Save Models and Settings file"));
  trAct(saveAsAct, tr("Save As..."), tr("Save Models and Settings file"));
  trAct(closeAct,  tr("Close"),      tr("Close Models and Settings file"));
  trAct(exitAct,   tr("Exit"),       tr("Exit the application"));
  trAct(aboutAct,  tr("About..."),   tr("Show the application's About box"));

  trAct(recentFilesAct,     tr("Recent Files"),               tr("List of recently used files"));
  trAct(profilesMenuAct,    tr("Radio Profiles"),             tr("Create or Select Radio Profiles"));
  trAct(logsAct,            tr("View Log File..."),           tr("Open and view log file"));
  trAct(appPrefsAct,        tr("Settings..."),                tr("Edit Settings"));
  trAct(fwPrefsAct,         tr("Download..."),                tr("Download firmware and voice files"));
  trAct(checkForUpdatesAct, tr("Check for Updates..."),       tr("Check OpenTX and Companion updates"));
  trAct(changelogAct,       tr("Companion Changes..."),       tr("Show Companion change log"));
  trAct(fwchangelogAct,     tr("Firmware Changes..."),        tr("Show firmware change log"));
  trAct(compareAct,         tr("Compare Models..."),          tr("Compare models"));
  trAct(editSplashAct,      tr("Edit Radio Splash Image..."), tr("Edit the splash image of your Radio"));
  trAct(burnListAct,        tr("List programmers..."),        tr("List available programmers"));
  trAct(burnFusesAct,       tr("Fuses..."),                   tr("Show fuses dialog"));
  trAct(readFlashAct,       tr("Read Firmware from Radio"),   tr("Read firmware from Radio"));
  trAct(writeFlashAct,      tr("Write Firmware to Radio"),    tr("Write firmware to Radio"));
  trAct(sdsyncAct,          tr("Synchronize SD"),             tr("SD card synchronization"));

  trAct(openDocURLAct,      tr("Manuals and other Documents"),         tr("Open the OpenTX document page in a web browser"));
  trAct(writeEepromAct,     tr("Write Models and Settings To Radio"),  tr("Write Models and Settings to Radio"));
  trAct(readEepromAct,      tr("Read Models and Settings From Radio"), tr("Read Models and Settings from Radio"));
  trAct(burnConfigAct,      tr("Configure Communications..."),         tr("Configure software for communicating with the Radio"));
  trAct(writeBUToRadioAct,  tr("Write Backup to Radio"),               tr("Write Backup from file to Radio"));
  trAct(readBUToFileAct,    tr("Backup Radio to File"),                tr("Save a complete backup file of all settings and model data in the Radio"));
  trAct(contributorsAct,    tr("Contributors..."),                     tr("A tribute to those who have contributed to OpenTX and Companion"));

  trAct(createProfileAct,   tr("Add Radio Profile"),               tr("Create a new Radio Settings Profile"));
  trAct(copyProfileAct,     tr("Copy Current Radio Profile"),      tr("Duplicate current Radio Settings Profile"));
  trAct(deleteProfileAct,   tr("Delete Current Radio Profile..."), tr("Delete the current Radio Settings Profile"));

  trAct(actTabbedWindows,   tr("Tabbed Windows"),    tr("Use tabs to arrange open windows."));
  trAct(actTileWindows,     tr("Tile Windows"),      tr("Arrange open windows across all the available space."));
  trAct(actCascadeWindows,  tr("Cascade Windows"),   tr("Arrange all open windows in a stack."));
  trAct(actCloseAllWindows, tr("Close All Windows"), tr("Closes all open files (prompts to save if necessary."));

  editMenu->setTitle(tr("Edit"));
  fileMenu->setTitle(tr("File"));
  settingsMenu->setTitle(tr("Settings"));
  themeMenu->setTitle(tr("Set Icon Theme"));
  iconThemeSizeMenu->setTitle(tr("Set Icon Size"));
  burnMenu->setTitle(tr("Read/Write"));
  windowMenu->setTitle(tr("Window"));
  helpMenu->setTitle(tr("Help"));

  fileToolBar->setWindowTitle(tr("File"));
  editToolBar->setWindowTitle(tr("Edit"));
  burnToolBar->setWindowTitle(tr("Write"));
  helpToolBar->setWindowTitle(tr("Help"));

  showReadyStatus();

  if (showMsg)
    QMessageBox::information(this, tr("Companion"), tr("Some text will not be translated until the next time you start Companion. Please note that some translations may not be complete."));
}

void MainWindow::createActions()
{
  newAct =             addAct("new.png",    SLOT(newFile()),                  QKeySequence::New);
  openAct =            addAct("open.png",   SLOT(openFile()),                 QKeySequence::Open);
  saveAct =            addAct("save.png",   SLOT(save()),                     QKeySequence::Save);
  saveAsAct =          addAct("saveas.png", SLOT(saveAs()),                   tr("Ctrl+Shift+S"));       // Windows doesn't have "native" save-as key, Lin/OSX both use this one anyway
  closeAct =           addAct("clear.png",  SLOT(closeFile())             /*, QKeySequence::Close*/);    // setting/showing this shortcut interferes with the system one (Ctrl+W/Ctrl-F4)
  exitAct =            addAct("exit.png",   SLOT(closeAllWindows()),          QKeySequence::Quit, qApp);

  logsAct =            addAct("logs.png",           SLOT(logFile()),          tr("Ctrl+Alt+L"));
  appPrefsAct =        addAct("apppreferences.png", SLOT(appPrefs()),         QKeySequence::Preferences);
  fwPrefsAct =         addAct("fwpreferences.png",  SLOT(fwPrefs()),          tr("Ctrl+Alt+D"));
  compareAct =         addAct("compare.png",        SLOT(compare()),          tr("Ctrl+Alt+R"));
  sdsyncAct =          addAct("sdsync.png",         SLOT(sdsync()));

  editSplashAct =      addAct("paintbrush.png",        SLOT(customizeSplash()));
  burnListAct =        addAct("list.png",              SLOT(burnList()));
  burnFusesAct =       addAct("fuses.png",             SLOT(burnFuses()));
  readFlashAct =       addAct("read_flash.png",        SLOT(readFlash()));
  writeFlashAct =      addAct("write_flash.png",       SLOT(writeFlash()));
  writeEepromAct =     addAct("write_eeprom.png",      SLOT(writeEeprom()));
  readEepromAct =      addAct("read_eeprom.png",       SLOT(readEeprom()));
  burnConfigAct =      addAct("configure.png",         SLOT(burnConfig()));
  writeBUToRadioAct =  addAct("write_eeprom_file.png", SLOT(writeBackup()));
  readBUToFileAct =    addAct("read_eeprom_file.png",  SLOT(readBackup()));

  createProfileAct =   addAct("new.png",   SLOT(createProfile()));
  copyProfileAct   =   addAct("copy.png",  SLOT(copyProfile()));
  deleteProfileAct =   addAct("clear.png", SLOT(deleteCurrentProfile()));

  actTabbedWindows =   addAct("", SLOT(setTabbedWindows(bool)), 0, this, SIGNAL(triggered(bool)));
  actTileWindows =     addAct("", SLOT(tileSubWindows()),       0, mdiArea);
  actCascadeWindows =  addAct("", SLOT(cascadeSubWindows()),    0, mdiArea);
  actCloseAllWindows = addAct("", SLOT(closeAllSubWindows()),   0, mdiArea);

  checkForUpdatesAct = addAct("update.png",         SLOT(doUpdates()));
  aboutAct =           addAct("information.png",    SLOT(about()));
  openDocURLAct =      addAct("changelog.png",      SLOT(openDocURL()));
  changelogAct =       addAct("changelog.png",      SLOT(changelog()));
  fwchangelogAct =     addAct("changelog.png",      SLOT(fwchangelog()));
  contributorsAct =    addAct("contributors.png",   SLOT(contributors()));

  // these two get assigned menus in createMenus()
  recentFilesAct =     addAct("recentdocument.png");
  profilesMenuAct =    addAct("profiles.png");

  exitAct->setMenuRole(QAction::QuitRole);
  aboutAct->setMenuRole(QAction::AboutRole);
  appPrefsAct->setMenuRole(QAction::PreferencesRole);
  contributorsAct->setMenuRole(QAction::ApplicationSpecificRole);
  openDocURLAct->setMenuRole(QAction::ApplicationSpecificRole);
  checkForUpdatesAct->setMenuRole(QAction::ApplicationSpecificRole);
  changelogAct->setMenuRole(QAction::ApplicationSpecificRole);
  fwchangelogAct->setMenuRole(QAction::ApplicationSpecificRole);

  actTabbedWindows->setCheckable(true);
  compareAct->setEnabled(false);
}

void MainWindow::createMenus()
{
  fileMenu = menuBar()->addMenu("");
  fileMenu->addAction(newAct);
  fileMenu->addAction(openAct);
  fileMenu->addAction(saveAct);
  fileMenu->addAction(saveAsAct);
  fileMenu->addAction(closeAct);
  fileMenu->addAction(recentFilesAct);
  fileMenu->addSeparator();
  fileMenu->addAction(logsAct);
  fileMenu->addAction(fwPrefsAct);
  fileMenu->addAction(compareAct);
  fileMenu->addAction(sdsyncAct);
  fileMenu->addSeparator();
  fileMenu->addAction(exitAct);

  editMenu = menuBar()->addMenu("");

  settingsMenu = menuBar()->addMenu("");
  settingsMenu->addMenu(createLanguageMenu(settingsMenu));

  themeMenu = settingsMenu->addMenu("");
  QActionGroup * themeGroup = new QActionGroup(themeMenu);
  addActToGroup(themeGroup, tr("Classical"),  tr("The classic companion9x icon theme"),  "themeId", 0, g.theme());
  addActToGroup(themeGroup, tr("Yerico"),     tr("Yellow round honey sweet icon theme"), "themeId", 1, g.theme());
  addActToGroup(themeGroup, tr("Monochrome"), tr("A monochrome black icon theme"),       "themeId", 3, g.theme());
  addActToGroup(themeGroup, tr("MonoBlue"),   tr("A monochrome blue icon theme"),        "themeId", 4, g.theme());
  addActToGroup(themeGroup, tr("MonoWhite"),  tr("A monochrome white icon theme"),       "themeId", 2, g.theme());
  connect(themeGroup, &QActionGroup::triggered, this, &MainWindow::onThemeChanged);
  themeMenu->addActions(themeGroup->actions());

  iconThemeSizeMenu = settingsMenu->addMenu("");
  QActionGroup * szGroup = new QActionGroup(iconThemeSizeMenu);
  addActToGroup(szGroup, tr("Small"),  tr("Use small toolbar icons"),       "sizeId", 0, g.iconSize());
  addActToGroup(szGroup, tr("Normal"), tr("Use normal size toolbar icons"), "sizeId", 1, g.iconSize());
  addActToGroup(szGroup, tr("Big"),    tr("Use big toolbar icons"),         "sizeId", 2, g.iconSize());
  addActToGroup(szGroup, tr("Huge"),   tr("Use huge toolbar icons"),        "sizeId", 3, g.iconSize());
  connect(szGroup, &QActionGroup::triggered, this, &MainWindow::onIconSizeChanged);
  iconThemeSizeMenu->addActions(szGroup->actions());

  settingsMenu->addSeparator();
  settingsMenu->addAction(appPrefsAct);
  settingsMenu->addAction(profilesMenuAct);
  settingsMenu->addAction(editSplashAct);
  settingsMenu->addAction(burnConfigAct);

  burnMenu = menuBar()->addMenu("");
  burnMenu->addAction(writeEepromAct);
  burnMenu->addAction(readEepromAct);
  burnMenu->addSeparator();
  burnMenu->addAction(writeBUToRadioAct);
  burnMenu->addAction(readBUToFileAct);
  burnMenu->addSeparator();
  burnMenu->addAction(writeFlashAct);
  burnMenu->addAction(readFlashAct);
  burnMenu->addSeparator();
  burnMenu->addSeparator();
  if (!IS_ARM(getCurrentBoard())) {
    burnMenu->addAction(burnFusesAct);
    burnMenu->addAction(burnListAct);
  }

  windowMenu = menuBar()->addMenu("");
  windowMenu->addAction(actTabbedWindows);
  windowMenu->addAction(actTileWindows);
  windowMenu->addAction(actCascadeWindows);
  windowMenu->addAction(actCloseAllWindows);
  windowMenu->addSeparator();

  helpMenu = menuBar()->addMenu("");
  helpMenu->addSeparator();
  helpMenu->addAction(checkForUpdatesAct);
  helpMenu->addSeparator();
  helpMenu->addAction(aboutAct);
  helpMenu->addAction(openDocURLAct);
  helpMenu->addSeparator();
  helpMenu->addAction(changelogAct);
  helpMenu->addAction(fwchangelogAct);
  helpMenu->addSeparator();
  helpMenu->addAction(contributorsAct);

  recentFilesMenu = new QMenu(this);
  for ( int i = 0; i < g.historySize(); ++i) {
    recentFileActs.append(recentFilesMenu->addAction(""));
    recentFileActs[i]->setVisible(false);
    connect(recentFileActs[i], SIGNAL(triggered()), this, SLOT(openRecentFile()));
  }
  recentFilesAct->setMenu(recentFilesMenu);

  profilesMenu = new QMenu(this);
  QActionGroup *profilesGroup = new QActionGroup(this);
  for (int i=0; i < MAX_PROFILES; i++) {
    profileActs.append(profilesMenu->addAction(""));
    profileActs[i]->setVisible(false);
    profileActs[i]->setCheckable(true);
    connect(profileActs[i], SIGNAL(triggered()), this, SLOT(loadProfile()));
    profilesGroup->addAction(profileActs[i]);
  }
  profilesMenu->addSeparator();
  profilesMenu->addAction(createProfileAct);
  profilesMenu->addAction(copyProfileAct);
  profilesMenu->addAction(deleteProfileAct);
  profilesMenuAct->setMenu(profilesMenu);
}

void MainWindow::createToolBars()
{
  fileToolBar = addToolBar("");
  fileToolBar->setObjectName("File");
  fileToolBar->addAction(newAct);
  fileToolBar->addAction(openAct);
  fileToolBar->addAction(recentFilesAct);
  fileToolBar->addAction(saveAct);
  fileToolBar->addAction(closeAct);
  fileToolBar->addSeparator();
  fileToolBar->addAction(logsAct);
  fileToolBar->addAction(fwPrefsAct);
  fileToolBar->addSeparator();
  fileToolBar->addAction(appPrefsAct);
  fileToolBar->addAction(profilesMenuAct);
  fileToolBar->addAction(editSplashAct);
  fileToolBar->addAction(editSplashAct);
  fileToolBar->addSeparator();
  fileToolBar->addAction(compareAct);
  fileToolBar->addAction(sdsyncAct);

  // workaround for default split button appearance of action with menu  :-/
  QToolButton * btn;
  if ((btn = qobject_cast<QToolButton *>(fileToolBar->widgetForAction(recentFilesAct))))
    btn->setPopupMode(QToolButton::InstantPopup);
  if ((btn = qobject_cast<QToolButton *>(fileToolBar->widgetForAction(profilesMenuAct))))
    btn->setPopupMode(QToolButton::InstantPopup);

  // gets populated later
  editToolBar = addToolBar("");
  editToolBar->setObjectName("Edit");

  burnToolBar = new QToolBar(this);
  addToolBar( Qt::LeftToolBarArea, burnToolBar );
  burnToolBar->setObjectName("Write");
  burnToolBar->addAction(writeEepromAct);
  burnToolBar->addAction(readEepromAct);
  burnToolBar->addSeparator();
  burnToolBar->addAction(writeBUToRadioAct);
  burnToolBar->addAction(readBUToFileAct);
  burnToolBar->addSeparator();
  burnToolBar->addAction(writeFlashAct);
  burnToolBar->addAction(readFlashAct);
  burnToolBar->addSeparator();
  burnToolBar->addAction(burnConfigAct);

  helpToolBar = addToolBar("");
  helpToolBar->setObjectName("Help");
  helpToolBar->addAction(checkForUpdatesAct);
  helpToolBar->addAction(aboutAct);
}

QMenu * MainWindow::createLanguageMenu(QWidget * parent)
{
  QMenu * menu = new QMenu(tr("Set Menu Language"), parent);
  QActionGroup * actGroup = new QActionGroup(menu);
  QString lName;

  addActToGroup(actGroup, tr("System language"), tr("Use default system language."), "locale", QString(""), g.locale());
  foreach (const QString & lang, Translations::getAvailableTranslations()) {
    QLocale locale(lang);
    lName = locale.nativeLanguageName();
    addActToGroup(actGroup, lName.left(1).toUpper() % lName.mid(1), tr("Use %1 language (some translations may not be complete).").arg(lName), "locale", lang, g.locale());
  }
  if (!actGroup->checkedAction())
    actGroup->actions().first()->setChecked(true);

  connect(actGroup, &QActionGroup::triggered, this, &MainWindow::onLanguageChanged);
  menu->addActions(actGroup->actions());
  return menu;
}

void MainWindow::showReadyStatus()
{
  statusBar()->showMessage(tr("Ready"));
}

MdiChild *MainWindow::activeMdiChild()
{
  if (QMdiSubWindow *activeSubWindow = mdiArea->activeSubWindow())
    return qobject_cast<MdiChild *>(activeSubWindow->widget());
  return 0;
}

QMdiSubWindow *MainWindow::findMdiChild(const QString &fileName)
{
  QString canonicalFilePath = QFileInfo(fileName).canonicalFilePath();

  foreach (QMdiSubWindow *window, mdiArea->subWindowList()) {
    MdiChild *mdiChild = qobject_cast<MdiChild *>(window->widget());
    if (mdiChild->currentFile() == canonicalFilePath)
      return window;
  }
  return 0;
}

void MainWindow::updateRecentFileActions()
{
  //  Hide all document slots
  for (int i=0 ; i < qMin(recentFileActs.size(), g.historySize()); i++) {
    recentFileActs[i]->setVisible(false);
  }

  // Fill slots with content and unhide them
  QStringList files = g.recentFiles();
  int numRecentFiles = qMin(recentFileActs.size(), qMin(files.size(), g.historySize()));

  for (int i=0; i < numRecentFiles; i++) {
    QString text = strippedName(files[i]);
    if (!text.trimmed().isEmpty()) {
      recentFileActs[i]->setText(text);
      recentFileActs[i]->setData(files[i]);
      recentFileActs[i]->setVisible(true);
    }
  }
}

void MainWindow::updateProfilesActions()
{
  for (int i=0; i < qMin(profileActs.size(), MAX_PROFILES); i++) {
    if (g.profile[i].existsOnDisk()) {
      QString text = tr("%2").arg(g.profile[i].name());
      profileActs[i]->setText(text);
      profileActs[i]->setData(i);
      profileActs[i]->setVisible(true);
      if (i == g.id())
        profileActs[i]->setChecked(true);
    }
    else {
      profileActs[i]->setVisible(false);
    }
  }
}

void MainWindow::updateWindowActions()
{
  if (!windowsListActions)
    return;

  foreach (QAction * act, windowsListActions->actions()) {
    windowsListActions->removeAction(act);
    if (windowMenu->actions().contains(act))
      windowMenu->removeAction(act);
    delete act;
  }
  int count = 0;
  foreach (QMdiSubWindow * win, mdiArea->subWindowList()) {
    QString scut;
    if (++count < 10)
      scut = tr("Alt+%1").arg(count);
    QAction * act = addActToGroup(windowsListActions, "", "", "window_ptr", qVariantFromValue(win), QVariant(), scut);
    act->setChecked(win == mdiArea->activeSubWindow());
    updateWindowActionTitle(win, act);
  }
  windowMenu->addActions(windowsListActions->actions());
}

void MainWindow::updateWindowActionTitle(const QMdiSubWindow * win, QAction * act)
{
  MdiChild * child = qobject_cast<MdiChild *>(win->widget());
  if (!child)
    return;

  if (!act) {
    foreach (QAction * a, windowsListActions->actions()) {
      if (a->property("window_ptr").canConvert<QMdiSubWindow *>() &&
          a->property("window_ptr").value<QMdiSubWindow *>() == win) {
        act = a;
        break;
      }
    }
  }
  if (!act)
    return;

  QString ttl = child->userFriendlyCurrentFile();
  if (child->isWindowModified())
    ttl.prepend("* ");
  act->setText(ttl);
}

void MainWindow::onSubwindowTitleChanged()
{
  QMdiSubWindow * win = NULL;
  if ((win = qobject_cast<QMdiSubWindow *>(sender()->parent())))
    updateWindowActionTitle(win);
}

void MainWindow::onSubwindowModified()
{
  onSubwindowTitleChanged();
}

void MainWindow::onChangeWindowAction(QAction * act)
{
  if (!act->isChecked())
    return;

  QMdiSubWindow * win = NULL;
  if (act->property("window_ptr").canConvert<QMdiSubWindow *>())
    win = act->property("window_ptr").value<QMdiSubWindow *>();
  if (win)
    mdiArea->setActiveSubWindow(win);
}

int MainWindow::newProfile(bool loadProfile)
{
  int i;
  for (i=0; i < MAX_PROFILES && g.profile[i].existsOnDisk(); i++)
    ;
  if (i == MAX_PROFILES)  //Failed to find free slot
    return -1;

  g.profile[i].init(i);
  g.profile[i].name(tr("New Radio"));

  if (loadProfile) {
    g.id(i);
    loadProfileId(i);
    appPrefs();
  }

  return i;
}

void MainWindow::createProfile()
{
  newProfile(true);
}

void MainWindow::copyProfile()
{
  int newId = newProfile(false);

  if (newId > -1) {
    g.profile[newId] = g.profile[g.id()];
    g.id(newId);
    g.profile[newId].name(g.profile[newId].name() + tr(" - Copy"));
    loadProfileId(newId);
    appPrefs();
  }
}

void MainWindow::deleteProfile(const int pid)
{
  if (pid == 0) {
    QMessageBox::information(this, tr("Not possible to remove profile"), tr("The default profile can not be removed."));
    return;
  }
  int ret = QMessageBox::question(this,
                                  tr("Confirm Delete Profile"),
                                  tr("Are you sure you wish to delete the \"%1\" radio profile? There is no way to undo this action!").arg(g.profile[pid].name()));
  if (ret != QMessageBox::Yes)
    return;

  g.profile[pid].remove();
  loadProfileId(0);
}

void MainWindow::deleteCurrentProfile()
{
  deleteProfile(g.id());
}

QString MainWindow::strippedName(const QString &fullFileName)
{
  return QFileInfo(fullFileName).fileName();
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
  if (event->mimeData()->hasFormat("text/uri-list"))
    event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent *event)
{
  QList<QUrl> urls = event->mimeData()->urls();
  if (urls.isEmpty()) return;
  QString fileName = urls.first().toLocalFile();
  openFile(fileName);
}

void MainWindow::autoClose()
{
  this->close();
}
