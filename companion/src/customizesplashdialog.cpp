#include "customizesplashdialog.h"
#include "ui_customizesplashdialog.h"
#include "flashfirmwaredialog.h"
#include "appdata.h"
#include "helpers.h"
#include "firmwareinterface.h"
#include "splashlibrarydialog.h"

//*** Side Class ***

Side::Side(){
  imageLabel = 0;
  fileNameEdit = 0;
  saveButton = 0;
  loadFwButton=0;
  loadPictButton = 0;
  loadProfileButton = 0;
  saveToFileName = new QString("");
  source = new Source(UNDEFINED);
  format = new LCDFormat(LCDTARANIS);
}

void Side::markSourceButton()
{
    loadFwButton->setChecked(*source == FW ? true : false );
    loadPictButton->setChecked(*source == PICT ? true : false );
    loadProfileButton->setChecked(*source == PROFILE ? true : false );
}

void Side::copyImage( Side side )
{
 if ((*source!=UNDEFINED) && (*side.source!=UNDEFINED))
    imageLabel->setPixmap(*side.imageLabel->pixmap());
}

bool Side::displayImage( QString fileName, Source pictSource )
{
  QImage image;

  if (fileName.isEmpty())
    return false;

  // Determine which picture format to use
  if (pictSource == FW ){
    FirmwareInterface firmware(fileName);
    if (!firmware.hasSplash())
      return false;
    else
      image = firmware.getSplash();
    *format = (firmware.getSplashWidth()==WIDTH_TARANIS ? LCDTARANIS : LCD9X);
  }
  else {
    image.load(fileName);
    if (pictSource== PICT)
      *format = image.width()>WIDTH_9X ? LCDTARANIS : LCD9X;
    else if (pictSource == PROFILE)
      *format = (g.profile[g.id()].fwType().contains("x9")) ? LCDTARANIS : LCD9X; 
  }
  if (image.isNull()) {
    return false;
  }
  // Load image
  if (*format==LCDTARANIS) {
    imageLabel->setPixmap( makePixMap( image, "x9" ));
    imageLabel->setFixedSize(WIDTH_TARANIS*2, HEIGHT_TARANIS*2);
  }
  else {
    imageLabel->setPixmap( makePixMap( image, "9x" )); 
    imageLabel->setFixedSize(WIDTH_9X*2, HEIGHT_9X*2);
  }

  switch (pictSource){
  case FW:
    fileNameEdit->setText(QObject::tr("FW: %1").arg(fileName));
    *saveToFileName = fileName;
    *source=FW;
    break;
  case PICT:
    fileNameEdit->setText(QObject::tr("Pict: %1").arg(fileName));
    *saveToFileName = fileName;
    *source=PICT;
    break;
  case PROFILE:
    fileNameEdit->setText(QObject::tr("Profile image"));
    *saveToFileName = fileName;
    *source=PROFILE;
    break;
  default:
    break;
  }
  saveButton->setEnabled(true);
  libraryButton->setEnabled(true);
  invertButton->setEnabled(true);
  return true;
}

bool Side::refreshImage()
{
  return displayImage( *saveToFileName, *source );
}

bool Side::saveImage()
{
  if (*source == FW )
  {
    FirmwareInterface firmware(*saveToFileName);
    if (!firmware.hasSplash()) {
      return false;
    }
    QImage image = imageLabel->pixmap()->toImage().scaled(firmware.getSplashWidth(), firmware.getSplashHeight());
    if (firmware.setSplash(image) && (firmware.save(*saveToFileName) > 0)) {
      g.flashDir( QFileInfo(*saveToFileName).dir().absolutePath() );
    }
    else {
      return false;
    }
  }
  else if (*source == PICT) {
    QImage image = imageLabel->pixmap()->toImage().scaled(imageLabel->width()/2, imageLabel->height()/2).convertToFormat(QImage::Format_Indexed8);
    if (image.save(*saveToFileName)) {
      g.imagesDir( QFileInfo(*saveToFileName).dir().absolutePath() );
    }
    else {
      return false;
    }
  }
  else if (*source == PROFILE) {
    QImage image = imageLabel->pixmap()->toImage().scaled(imageLabel->width()/2, imageLabel->height()/2).convertToFormat(QImage::Format_Indexed8);
    if (!image.save(*saveToFileName)) {
      return false;
    }
  }
  return true;
}

//*** customizeSplashDialog Class ***

customizeSplashDialog::customizeSplashDialog(QWidget *parent) :
  QDialog(parent),
  ui(new Ui::customizeSplashDialog)
{
  ui->setupUi(this);
  setWindowIcon(CompanionIcon("paintbrush.png"));
  ui->leftLibraryButton->setIcon(CompanionIcon("library.png"));
  ui->rightLibraryButton->setIcon(CompanionIcon("library.png"));
  
  left.imageLabel =  ui->leftImageLabel;
  right.imageLabel = ui->rightImageLabel;
  left.fileNameEdit =  ui->leftFileNameEdit;
  right.fileNameEdit = ui->rightFileNameEdit;
  left.saveButton =  ui->leftSaveButton;
  right.saveButton =  ui->rightSaveButton;
  left.libraryButton = ui->leftLibraryButton;
  right.libraryButton = ui->rightLibraryButton;
  left.invertButton = ui->leftInvertButton;
  right.invertButton = ui->rightInvertButton;
  
  left.loadFwButton =  ui->leftLoadFwButton;
  right.loadFwButton =  ui->rightLoadFwButton;
  left.loadPictButton =  ui->leftLoadPictButton;
  right.loadPictButton =  ui->rightLoadPictButton;
  left.loadProfileButton =  ui->leftLoadProfileButton;
  right.loadProfileButton =  ui->rightLoadProfileButton;

  loadProfile(left);
  left.markSourceButton();

  resize(0,0);
}

customizeSplashDialog::~customizeSplashDialog()
{
  delete ui;
}

void customizeSplashDialog::on_copyRightToLeftButton_clicked() {
  left.copyImage(right);
}
void customizeSplashDialog::on_copyLeftToRightButton_clicked() {
right.copyImage(left);
}


void customizeSplashDialog::on_leftLoadFwButton_clicked() {loadFirmware(left);}
void customizeSplashDialog::on_rightLoadFwButton_clicked() {loadFirmware(right);}
void customizeSplashDialog::loadFirmware(Side side)
{
  QString fileName = QFileDialog::getOpenFileName(this, tr("Open Firmware File"), g.flashDir(), FLASH_FILES_FILTER);
  if (!fileName.isEmpty()) {
    if (!side.displayImage( fileName, FW ))
      QMessageBox::critical(this, tr("Error"), tr("Can not load embedded image from firmware file %1.").arg(fileName));
    else
    g.flashDir( QFileInfo(fileName).dir().absolutePath() );
  }
  side.markSourceButton();
}

void customizeSplashDialog::on_leftLoadPictButton_clicked() {loadPicture(left);}
void customizeSplashDialog::on_rightLoadPictButton_clicked() {loadPicture(right);}
void customizeSplashDialog::loadPicture(Side side)
{
  QString supportedImageFormats;
  for (int formatIndex = 0; formatIndex < QImageReader::supportedImageFormats().count(); formatIndex++) {
    supportedImageFormats += QLatin1String(" *.") + QImageReader::supportedImageFormats()[formatIndex];
  }
  QString fileName = QFileDialog::getOpenFileName(this,
          tr("Open Image to load"), g.imagesDir(), tr("Images (%1)").arg(supportedImageFormats));

  if (!fileName.isEmpty()) {
    if (!side.displayImage( fileName, PICT ))
      QMessageBox::critical(this, tr("Error"), tr("Cannot load the image file %1.").arg(fileName));
    else
      g.imagesDir( QFileInfo(fileName).dir().absolutePath() );
  }
  side.markSourceButton();
}

void customizeSplashDialog::on_leftLoadProfileButton_clicked() {loadProfile(left);}
void customizeSplashDialog::on_rightLoadProfileButton_clicked() {loadProfile(right);}
void customizeSplashDialog::loadProfile(Side side)
{
  QString fileName=g.profile[g.id()].splashFile();

  if (!fileName.isEmpty()) {
    if (!side.displayImage( fileName, PROFILE ))
      QMessageBox::critical(this, tr("Error"), tr("Cannot load profile image %1.").arg(fileName));
  }
  side.markSourceButton();
}

void customizeSplashDialog::on_leftLibraryButton_clicked(){libraryButton_clicked(left);}
void customizeSplashDialog::on_rightLibraryButton_clicked(){libraryButton_clicked(right);}
void customizeSplashDialog::libraryButton_clicked( Side side )
{
  QString fileName;
  SplashLibraryDialog *ld = new SplashLibraryDialog(this,&fileName);
  ld->exec();
  if (!fileName.isEmpty()) {
    if (!side.displayImage( fileName, UNDEFINED ))
      QMessageBox::critical(this, tr("Error"), tr("Cannot load the library image %1.").arg(fileName));
  }
}

void customizeSplashDialog::on_leftSaveButton_clicked(){saveButton_clicked(left);}
void customizeSplashDialog::on_rightSaveButton_clicked(){saveButton_clicked(right);}
void customizeSplashDialog::saveButton_clicked( Side side )
{
  if (side.saveImage()){
    QMessageBox::information(this, tr("File Saved"), tr("The image was saved to the file %1").arg(*side.saveToFileName));
    if ( !side.refreshImage()){
      QMessageBox::critical(this, tr("Image Refresh Error"), tr("Failed to refresh image from file %1").arg(*side.saveToFileName));
    }
  }
  else
    QMessageBox::critical(this, tr("File Save Error"), tr("Failed to write image to %1").arg(*side.saveToFileName));
}

void customizeSplashDialog::on_leftInvertButton_clicked(){invertButton_clicked(left);}
void customizeSplashDialog::on_rightInvertButton_clicked(){invertButton_clicked(right);}
void customizeSplashDialog::invertButton_clicked(Side side)
{
  QImage image = side.imageLabel->pixmap()->toImage();
  image.invertPixels();
  side.imageLabel->setPixmap(QPixmap::fromImage(image));
}


