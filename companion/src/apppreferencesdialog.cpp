#include "apppreferencesdialog.h"
#include "ui_apppreferencesdialog.h"
#include "mainwindow.h"
#include "appdata.h"
#include "helpers.h"
#include "flashinterface.h"
#ifdef JOYSTICKS
#include "joystick.h"
#include "joystickdialog.h"
#endif
#include <QDesktopServices>
#include <QtGui>

appPreferencesDialog::appPreferencesDialog(QWidget *parent) :
  QDialog(parent),
  ui(new Ui::appPreferencesDialog)
{
  ui->setupUi(this);
  setWindowIcon(CompanionIcon("apppreferences.png"));
  initSettings();
  connect(this, SIGNAL(accepted()), this, SLOT(writeValues()));
#ifndef JOYSTICKS
  ui->joystickCB->hide();
  ui->joystickCB->setDisabled(true);
  ui->joystickcalButton->hide();
  ui->joystickChkB->hide();
  ui->label_11->hide();
#endif
  resize(0,0);
}

appPreferencesDialog::~appPreferencesDialog()
{
  delete ui;
}

void appPreferencesDialog::writeValues()
{
  g.autoCheckApp(ui->startupCheck_companion9x->isChecked());
  g.autoCheckFw(ui->startupCheck_fw->isChecked());
  g.enableWizard(ui->wizardEnable_ChkB->isChecked());
  g.showSplash(ui->showSplash->isChecked());
  g.simuSW(ui->simuSW->isChecked());
  g.historySize(ui->historySize->value());
  g.backLight(ui->backLightColor->currentIndex());
  g.libDir(ui->libraryPath->text());
  g.gePath(ui->ge_lineedit->text());
  g.embedSplashes(ui->splashincludeCB->currentIndex());
  g.enableBackup(ui->backupEnable->isChecked());

  if (ui->joystickChkB ->isChecked() && ui->joystickCB->isEnabled()) {
    g.jsSupport(ui->joystickChkB ->isChecked());  
    g.jsCtrl(ui->joystickCB ->currentIndex());
  }
  else {
    g.jsSupport(false);
    g.jsCtrl(0);
  }

  g.id(ui->profileIndexLE->text().toInt());

  g.profile[g.id()].channelOrder(ui->channelorderCB->currentIndex());
  g.profile[g.id()].defaultMode(ui->stickmodeCB->currentIndex());
  g.profile[g.id()].renameFwFiles(ui->renameFirmware->isChecked());
  g.profile[g.id()].burnFirmware(ui->burnFirmware->isChecked());
  g.profile[g.id()].name(ui->profileNameLE->text());
  g.profile[g.id()].sdPath(ui->sdPath->text());
  g.profile[g.id()].splashFile(ui->SplashFileName->text());
  g.profile[g.id()].firmware(ui->firmwareLE->text());
  
  saveProfile();
}

void appPreferencesDialog::on_snapshotPathButton_clicked()
{
  QString fileName = QFileDialog::getExistingDirectory(this,tr("Select your snapshot folder"), g.snapshotDir());
  if (!fileName.isEmpty()) {
    g.snapshotDir(fileName);
    g.snapToClpbrd(false);
    ui->snapshotPath->setText(fileName);
  }
}

void appPreferencesDialog::initSettings()
{
  ui->snapshotClipboardCKB->setChecked(g.snapToClpbrd());
  ui->burnFirmware->setChecked(g.profile[g.id()].burnFirmware());
  ui->snapshotPath->setText(g.snapshotDir());
  ui->snapshotPath->setReadOnly(true);
  if (ui->snapshotClipboardCKB->isChecked())
  {
    ui->snapshotPath->setDisabled(true);
    ui->snapshotPathButton->setDisabled(true);
  }
  ui->startupCheck_companion9x->setChecked(g.autoCheckApp());
  ui->startupCheck_fw->setChecked(g.autoCheckFw());
  ui->wizardEnable_ChkB->setChecked(g.enableWizard());
  ui->showSplash->setChecked(g.showSplash());
  ui->historySize->setValue(g.historySize());
  ui->backLightColor->setCurrentIndex(g.backLight());
  ui->simuSW->setChecked(g.simuSW());
  ui->libraryPath->setText(g.libDir());
  ui->ge_lineedit->setText(g.gePath());

  if (!g.backupDir().isEmpty()) {
    if (QDir(g.backupDir()).exists()) {
      ui->backupPath->setText(g.backupDir());
      ui->backupEnable->setEnabled(true);
      ui->backupEnable->setChecked(g.enableBackup());
    } else {
      ui->backupEnable->setDisabled(true);
    }
  } else {
      ui->backupEnable->setDisabled(true);
  }
  ui->splashincludeCB->setCurrentIndex(g.embedSplashes());

#ifdef JOYSTICKS
  ui->joystickChkB->setChecked(g.jsSupport());
  if (ui->joystickChkB->isChecked()) {
    QStringList joystickNames;
    joystickNames << tr("No joysticks found");
    joystick = new Joystick(0,false,0,0);
    ui->joystickcalButton->setDisabled(true);
    ui->joystickCB->setDisabled(true);

    if ( joystick ) {
      if ( joystick->joystickNames.count() > 0 ) {
        joystickNames = joystick->joystickNames;
        ui->joystickCB->setEnabled(true);
        ui->joystickcalButton->setEnabled(true);
      }
      joystick->close();
    }
    ui->joystickCB->clear();
    ui->joystickCB->insertItems(0, joystickNames);
    ui->joystickCB->setCurrentIndex(g.jsCtrl());
  }
  else {
    ui->joystickCB->clear();
    ui->joystickCB->setDisabled(true);
    ui->joystickcalButton->setDisabled(true);
  }
#endif  
//  Profile Tab Inits  
  ui->channelorderCB->setCurrentIndex(g.profile[g.id()].channelOrder());
  ui->stickmodeCB->setCurrentIndex(g.profile[g.id()].defaultMode());
  ui->renameFirmware->setChecked(g.profile[g.id()].renameFwFiles());
  ui->sdPath->setText(g.profile[g.id()].sdPath());
  ui->profileIndexLE->setText(QString("%1").arg(g.id()));
  ui->profileNameLE->setText(g.profile[g.id()].name());
  ui->firmwareLE->setText(g.profile[g.id()].firmware());
  ui->SplashFileName->setText(g.profile[g.id()].splashFile());

  displayImage( g.profile[g.id()].splashFile() );
}

void appPreferencesDialog::on_libraryPathButton_clicked()
{
  QString fileName = QFileDialog::getExistingDirectory(this,tr("Select your library folder"), g.libDir());
  if (!fileName.isEmpty()) {
    g.libDir(fileName);
    ui->libraryPath->setText(fileName);
  }
}

void appPreferencesDialog::on_snapshotClipboardCKB_clicked()
{
  if (ui->snapshotClipboardCKB->isChecked()) {
    ui->snapshotPath->setDisabled(true);
    ui->snapshotPathButton->setDisabled(true);
    g.snapToClpbrd(true);
  } else {
    ui->snapshotPath->setEnabled(true);
    ui->snapshotPath->setReadOnly(true);
    ui->snapshotPathButton->setEnabled(true);
    g.snapToClpbrd(false);
  }
}

void appPreferencesDialog::on_backupPathButton_clicked()
{
  QString fileName = QFileDialog::getExistingDirectory(this,tr("Select your Models and Settings backup folder"), g.backupDir());
  if (!fileName.isEmpty()) {
    g.backupDir(fileName);
    ui->backupPath->setText(fileName);
  }
  ui->backupEnable->setEnabled(true);
}

void appPreferencesDialog::on_ge_pathButton_clicked()
{
  QString fileName = QFileDialog::getOpenFileName(this, tr("Select Google Earth executable"),ui->ge_lineedit->text());
  if (!fileName.isEmpty()) {
    ui->ge_lineedit->setText(fileName);
  }
}
 
#ifdef JOYSTICKS
void appPreferencesDialog::on_joystickChkB_clicked() {
  if (ui->joystickChkB->isChecked()) {
    QStringList joystickNames;
    joystickNames << tr("No joysticks found");
    joystick = new Joystick(0,false,0,0);
    ui->joystickcalButton->setDisabled(true);
    ui->joystickCB->setDisabled(true);

    if ( joystick ) {
      if ( joystick->joystickNames.count() > 0 ) {
        joystickNames = joystick->joystickNames;
        ui->joystickCB->setEnabled(true);
        ui->joystickcalButton->setEnabled(true);
      }
      joystick->close();
    }
    ui->joystickCB->clear();
    ui->joystickCB->insertItems(0, joystickNames);
  }
  else {
    ui->joystickCB->clear();
    ui->joystickCB->setDisabled(true);
    ui->joystickcalButton->setDisabled(true);
  }
}

void appPreferencesDialog::on_joystickcalButton_clicked() {
   joystickDialog * jd=new joystickDialog(this, ui->joystickCB->currentIndex());
   jd->exec();
}
#endif

// ******** Profile tab functions

void appPreferencesDialog::on_sdPathButton_clicked()
{
  QString fileName = QFileDialog::getExistingDirectory(this,tr("Select the folder replicating your SD structure"), g.profile[g.id()].sdPath());
  if (!fileName.isEmpty()) {
    ui->sdPath->setText(fileName);
  }
}

void appPreferencesDialog::saveProfile()
{
  // The profile name may NEVER be empty
  if (ui->profileNameLE->text().isEmpty())
    ui->profileNameLE->setText("----");

  g.profile[g.id()].name( ui->profileNameLE->text() );
  g.profile[g.id()].channelOrder( ui->channelorderCB->currentIndex());
  g.profile[g.id()].defaultMode( ui->stickmodeCB->currentIndex());
  g.profile[g.id()].burnFirmware( ui->burnFirmware->isChecked());
  g.profile[g.id()].renameFwFiles( ui->renameFirmware->isChecked());
  g.profile[g.id()].sdPath( ui->sdPath->text());
  g.profile[g.id()].splashFile( ui->SplashFileName->text());
  g.profile[g.id()].firmware( ui->firmwareLE->text());
}

void appPreferencesDialog::on_removeProfileButton_clicked()
{
  if ( g.id() == 0 )
     QMessageBox::information(this, tr("Not possible to remove profile"), tr("The default profile can not be removed."));
  else
  {
    g.profile[g.id()].remove();
    g.id( 0 );
    initSettings();
  }
}

bool appPreferencesDialog::displayImage( QString fileName )
{
  // Start by clearing the pixmap
  ui->imageLabel->setPixmap(QPixmap());

  QImage image(fileName);
  if (image.isNull()) 
    return false;

  // Use the firmware name to determine splash width
  int width = SPLASH_WIDTH;
  if (g.profile[g.id()].firmware().contains("taranis"))
    width = SPLASHX9D_WIDTH;

  ui->imageLabel->setPixmap(QPixmap::fromImage(image.scaled(width, SPLASH_HEIGHT)));
  if (width==SPLASHX9D_WIDTH) {
    image=image.convertToFormat(QImage::Format_RGB32);
    QRgb col;
    int gray, height = image.height();
    for (int i = 0; i < width; ++i) {
      for (int j = 0; j < height; ++j) {
        col = image.pixel(i, j);
        gray = qGray(col);
        image.setPixel(i, j, qRgb(gray, gray, gray));
      }
    }      
    ui->imageLabel->setPixmap(QPixmap::fromImage(image));
  } 
  else {
    ui->imageLabel->setPixmap(QPixmap::fromImage(image.convertToFormat(QImage::Format_Mono)));
  }
  if (width == SPLASH_WIDTH)
      ui->imageLabel->setFixedSize(SPLASH_WIDTH, SPLASH_HEIGHT);
  else
     ui->imageLabel->setFixedSize(SPLASHX9D_WIDTH, SPLASHX9D_HEIGHT);

  return true;
}

void appPreferencesDialog::on_SplashSelect_clicked()
{
  QString supportedImageFormats;
  for (int formatIndex = 0; formatIndex < QImageReader::supportedImageFormats().count(); formatIndex++) {
    supportedImageFormats += QLatin1String(" *.") + QImageReader::supportedImageFormats()[formatIndex];
  }

  QString fileName = QFileDialog::getOpenFileName(this,
          tr("Open Image to load"), g.imagesDir(), tr("Images (%1)").arg(supportedImageFormats));

  if (!fileName.isEmpty()){
    g.imagesDir(QFileInfo(fileName).dir().absolutePath());
   
    displayImage(fileName);
    ui->SplashFileName->setText(fileName);
  }
}

void appPreferencesDialog::on_clearImageButton_clicked() {
  ui->imageLabel->clear();
  ui->SplashFileName->clear();
}



