#include "calibration.h"
#include "ui_calibration.h"

void CalibrationPanel::setupSwitchConfig(int index, QLabel *label, AutoLineEdit *name, AutoComboBox *type)
{
  bool enabled = false;

  if (IS_TARANIS(firmware->getBoard())) {
    if (IS_TARANIS_X9E(firmware->getBoard())) {
      enabled = true;
      type->addItem(tr("None"), GeneralSettings::SWITCH_NONE);
    }
    else if (index < 8) {
      enabled = true;
    }
  }

  if (enabled) {
    type->addItem(tr("2 Positions Toggle"), GeneralSettings::SWITCH_TOGGLE);
    type->addItem(tr("2 Positions"), GeneralSettings::SWITCH_2POS);
    type->addItem(tr("3 Positions"), GeneralSettings::SWITCH_3POS);
    name->setField(generalSettings.switchName[index], 3, this);
    type->setField(generalSettings.switchConfig[index], this);
  }
  else {
    label->hide();
    name->hide();
    type->hide();
  }
}

void CalibrationPanel::setupPotConfig(int index, QLabel *label, AutoLineEdit *name, AutoComboBox *type)
{
  bool enabled = false;

  if (IS_TARANIS_X9E(firmware->getBoard()) && index < 4) {
    label->setText(RawSource(SOURCE_TYPE_STICK, index+NUM_STICKS).toString());
    enabled = true;
  }
  else if (IS_TARANIS_PLUS(firmware->getBoard()) && index < 3) {
    enabled = true;
  }
  else if (IS_TARANIS(firmware->getBoard()) && index < 2) {
    enabled = true;
  }

  if (enabled) {
    type->addItem(tr("None"), GeneralSettings::POT_NONE);
    type->addItem(tr("Pot with detent"), GeneralSettings::POT_WITH_DETENT);
    type->addItem(tr("Multipos switch"), GeneralSettings::POT_MULTIPOS_SWITCH);
    type->addItem(tr("Pot without detent"), GeneralSettings::POT_WITHOUT_DETENT);
    name->setField(generalSettings.potName[index], 3, this);
    type->setField(generalSettings.potConfig[index], this);
  }
  else {
    label->hide();
    name->hide();
    type->hide();
  }
}

void CalibrationPanel::setupSliderConfig(int index, QLabel *label, AutoLineEdit *name, AutoComboBox *type)
{
  bool enabled = false;

  if (IS_TARANIS(firmware->getBoard()) && index < 2) {
    type->setEnabled(false);
    enabled = true;
  }
  else if (IS_TARANIS_X9E(firmware->getBoard()) && index < 4) {
    enabled = true;
  }

  if (IS_TARANIS_X9E(firmware->getBoard())) {
    label->setText(RawSource(SOURCE_TYPE_STICK, index+NUM_STICKS+4).toString());
  }

  if (enabled) {
    type->addItem(tr("None"), GeneralSettings::SLIDER_NONE);
    type->addItem(tr("Slider with detent"), GeneralSettings::SLIDER_WITH_DETENT);
    name->setField(generalSettings.sliderName[index], 3, this);
    type->setField(generalSettings.sliderConfig[index], this);
  }
  else {
    label->hide();
    name->hide();
    type->hide();
  }
}

CalibrationPanel::CalibrationPanel(QWidget * parent, GeneralSettings & generalSettings, Firmware * firmware):
  GeneralPanel(parent, generalSettings, firmware),
  ui(new Ui::Calibration)
{
  ui->setupUi(this);

  if (!firmware->getCapability(MultiposPots)) {
    ui->potsTypeSeparator->hide();
    ui->potsTypeSeparator_2->hide();
  }

  if (IS_TARANIS(firmware->getBoard())) {
    ui->rudName->setField(generalSettings.stickName[0], 3, this);
    ui->eleName->setField(generalSettings.stickName[1], 3, this);
    ui->thrName->setField(generalSettings.stickName[2], 3, this);
    ui->ailName->setField(generalSettings.stickName[3], 3, this);
  }
  else {
    ui->rudLabel->hide();
    ui->rudName->hide();
    ui->eleLabel->hide();
    ui->eleName->hide();
    ui->thrLabel->hide();
    ui->thrName->hide();
    ui->ailLabel->hide();
    ui->ailName->hide();
    ui->switchline_1->hide();
    ui->switchline_2->hide();
    ui->switchline_3->hide();
    ui->stickline_1->hide();
    ui->stickline_2->hide();
    ui->stickline_3->hide();
  }

  setupPotConfig(0, ui->pot1Label, ui->pot1Name, ui->pot1Type);
  setupPotConfig(1, ui->pot2Label, ui->pot2Name, ui->pot2Type);
  setupPotConfig(2, ui->pot3Label, ui->pot3Name, ui->pot3Type);
  setupPotConfig(3, ui->pot4Label, ui->pot4Name, ui->pot4Type);

  setupSliderConfig(0, ui->lsLabel, ui->lsName, ui->lsType);
  setupSliderConfig(1, ui->rsLabel, ui->rsName, ui->rsType);
  setupSliderConfig(2, ui->ls2Label, ui->ls2Name, ui->ls2Type);
  setupSliderConfig(3, ui->rs2Label, ui->rs2Name, ui->rs2Type);

  setupSwitchConfig(0, ui->saLabel, ui->saName, ui->saType);
  setupSwitchConfig(1, ui->sbLabel, ui->sbName, ui->sbType);
  setupSwitchConfig(2, ui->scLabel, ui->scName, ui->scType);
  setupSwitchConfig(3, ui->sdLabel, ui->sdName, ui->sdType);
  setupSwitchConfig(4, ui->seLabel, ui->seName, ui->seType);
  setupSwitchConfig(5, ui->sfLabel, ui->sfName, ui->sfType);
  setupSwitchConfig(6, ui->sgLabel, ui->sgName, ui->sgType);
  setupSwitchConfig(7, ui->shLabel, ui->shName, ui->shType);
  setupSwitchConfig(8, ui->siLabel, ui->siName, ui->siType);
  setupSwitchConfig(9, ui->sjLabel, ui->sjName, ui->sjType);
  setupSwitchConfig(10, ui->skLabel, ui->skName, ui->skType);
  setupSwitchConfig(11, ui->slLabel, ui->slName, ui->slType);
  setupSwitchConfig(12, ui->smLabel, ui->smName, ui->smType);
  setupSwitchConfig(13, ui->snLabel, ui->snName, ui->snType);
  setupSwitchConfig(14, ui->soLabel, ui->soName, ui->soType);
  setupSwitchConfig(15, ui->spLabel, ui->spName, ui->spType);
  setupSwitchConfig(16, ui->sqLabel, ui->sqName, ui->sqType);
  setupSwitchConfig(17, ui->srLabel, ui->srName, ui->srType);

  int potsCount = GetCurrentFirmware()->getCapability(Pots);
  if (potsCount == 3) {
    ui->ana8Neg->hide();
    ui->ana8Mid->hide();
    ui->ana8Pos->hide();
  }

  if (IS_TARANIS(firmware->getBoard())) {
    ui->serialPortMode->setCurrentIndex(generalSettings.hw_uartMode);
  }
  else {
    ui->serialPortMode->hide();
    ui->serialPortLabel->hide();
  }

  disableMouseScrolling();
}

CalibrationPanel::~CalibrationPanel()
{
  delete ui;
}

/*void CalibrationPanel::update()
{
}
*/
void CalibrationPanel::on_PPM_MultiplierDSB_editingFinished()
{
  generalSettings.PPM_Multiplier = (int)(ui->PPM_MultiplierDSB->value()*10)-10;
  emit modified();
}

void CalibrationPanel::on_PPM1_editingFinished()
{
  generalSettings.trainer.calib[0] = ui->PPM1->value();
  emit modified();
}

void CalibrationPanel::on_PPM2_editingFinished()
{
  generalSettings.trainer.calib[1] = ui->PPM2->value();
  emit modified();
}

void CalibrationPanel::on_PPM3_editingFinished()
{
  generalSettings.trainer.calib[2] = ui->PPM3->value();
  emit modified();
}

void CalibrationPanel::on_PPM4_editingFinished()
{
  generalSettings.trainer.calib[3] = ui->PPM4->value();
  emit modified();
}


void CalibrationPanel::on_CurrentCalib_SB_editingFinished()
{
  generalSettings.currentCalib = ui->CurrentCalib_SB->value();
  emit modified();
}

void CalibrationPanel::setValues()
{
  ui->battCalibDSB->setValue((double)generalSettings.vBatCalib/10);
  ui->CurrentCalib_SB->setValue((double)generalSettings.currentCalib);

  ui->ana1Neg->setValue(generalSettings.calibSpanNeg[0]);
  ui->ana2Neg->setValue(generalSettings.calibSpanNeg[1]);
  ui->ana3Neg->setValue(generalSettings.calibSpanNeg[2]);
  ui->ana4Neg->setValue(generalSettings.calibSpanNeg[3]);
  ui->ana5Neg->setValue(generalSettings.calibSpanNeg[4]);
  ui->ana6Neg->setValue(generalSettings.calibSpanNeg[5]);
  ui->ana7Neg->setValue(generalSettings.calibSpanNeg[6]);
  ui->ana8Neg->setValue(generalSettings.calibSpanNeg[7]);

  ui->ana1Mid->setValue(generalSettings.calibMid[0]);
  ui->ana2Mid->setValue(generalSettings.calibMid[1]);
  ui->ana3Mid->setValue(generalSettings.calibMid[2]);
  ui->ana4Mid->setValue(generalSettings.calibMid[3]);
  ui->ana5Mid->setValue(generalSettings.calibMid[4]);
  ui->ana6Mid->setValue(generalSettings.calibMid[5]);
  ui->ana7Mid->setValue(generalSettings.calibMid[6]);
  ui->ana8Mid->setValue(generalSettings.calibMid[7]);

  ui->ana1Pos->setValue(generalSettings.calibSpanPos[0]);
  ui->ana2Pos->setValue(generalSettings.calibSpanPos[1]);
  ui->ana3Pos->setValue(generalSettings.calibSpanPos[2]);
  ui->ana4Pos->setValue(generalSettings.calibSpanPos[3]);
  ui->ana5Pos->setValue(generalSettings.calibSpanPos[4]);
  ui->ana6Pos->setValue(generalSettings.calibSpanPos[5]);
  ui->ana7Pos->setValue(generalSettings.calibSpanPos[6]);
  ui->ana8Pos->setValue(generalSettings.calibSpanPos[7]);

  ui->PPM1->setValue(generalSettings.trainer.calib[0]);
  ui->PPM2->setValue(generalSettings.trainer.calib[1]);
  ui->PPM3->setValue(generalSettings.trainer.calib[2]);
  ui->PPM4->setValue(generalSettings.trainer.calib[3]);
  ui->PPM_MultiplierDSB->setValue((qreal)(generalSettings.PPM_Multiplier+10)/10);
}

void CalibrationPanel::on_battCalibDSB_editingFinished()
{
  generalSettings.vBatCalib = ui->battCalibDSB->value()*10;
  emit modified();
}


void CalibrationPanel::on_ana1Neg_editingFinished()
{
  generalSettings.calibSpanNeg[0] = ui->ana1Neg->value();
  emit modified();
}

void CalibrationPanel::on_ana2Neg_editingFinished()
{
  generalSettings.calibSpanNeg[1] = ui->ana2Neg->value();
  emit modified();
}

void CalibrationPanel::on_ana3Neg_editingFinished()
{
  generalSettings.calibSpanNeg[2] = ui->ana3Neg->value();
  emit modified();
}

void CalibrationPanel::on_ana4Neg_editingFinished()
{
  generalSettings.calibSpanNeg[3] = ui->ana4Neg->value();
  emit modified();
}

void CalibrationPanel::on_ana5Neg_editingFinished()
{
  generalSettings.calibSpanNeg[4] = ui->ana5Neg->value();
  emit modified();
}

void CalibrationPanel::on_ana6Neg_editingFinished()
{
  generalSettings.calibSpanNeg[5] = ui->ana6Neg->value();
  emit modified();
}

void CalibrationPanel::on_ana7Neg_editingFinished()
{
  generalSettings.calibSpanNeg[6] = ui->ana7Neg->value();
  emit modified();
}

void CalibrationPanel::on_ana8Neg_editingFinished()
{
  generalSettings.calibSpanNeg[7] = ui->ana8Neg->value();
  emit modified();
}

void CalibrationPanel::on_ana1Mid_editingFinished()
{
  generalSettings.calibMid[0] = ui->ana1Mid->value();
  emit modified();
}

void CalibrationPanel::on_ana2Mid_editingFinished()
{
  generalSettings.calibMid[1] = ui->ana2Mid->value();
  emit modified();
}

void CalibrationPanel::on_ana3Mid_editingFinished()
{
  generalSettings.calibMid[2] = ui->ana3Mid->value();
  emit modified();
}

void CalibrationPanel::on_ana4Mid_editingFinished()
{
  generalSettings.calibMid[3] = ui->ana4Mid->value();
  emit modified();
}

void CalibrationPanel::on_ana5Mid_editingFinished()
{
  generalSettings.calibMid[4] = ui->ana5Mid->value();
  emit modified();
}

void CalibrationPanel::on_ana6Mid_editingFinished()
{
  generalSettings.calibMid[5] = ui->ana6Mid->value();
  emit modified();
}

void CalibrationPanel::on_ana7Mid_editingFinished()
{
  generalSettings.calibMid[6] = ui->ana7Mid->value();
  emit modified();
}

void CalibrationPanel::on_ana8Mid_editingFinished()
{
  generalSettings.calibMid[7] = ui->ana8Mid->value();
  emit modified();
}


void CalibrationPanel::on_ana1Pos_editingFinished()
{
  generalSettings.calibSpanPos[0] = ui->ana1Pos->value();
  emit modified();
}

void CalibrationPanel::on_ana2Pos_editingFinished()
{
  generalSettings.calibSpanPos[1] = ui->ana2Pos->value();
  emit modified();
}

void CalibrationPanel::on_ana3Pos_editingFinished()
{
  generalSettings.calibSpanPos[2] = ui->ana3Pos->value();
  emit modified();
}

void CalibrationPanel::on_ana4Pos_editingFinished()
{
  generalSettings.calibSpanPos[3] = ui->ana4Pos->value();
  emit modified();
}

void CalibrationPanel::on_ana5Pos_editingFinished()
{
  generalSettings.calibSpanPos[4] = ui->ana5Pos->value();
  emit modified();
}

void CalibrationPanel::on_ana6Pos_editingFinished()
{
  generalSettings.calibSpanPos[5] = ui->ana6Pos->value();
  emit modified();
}

void CalibrationPanel::on_ana7Pos_editingFinished()
{
  generalSettings.calibSpanPos[6] = ui->ana7Pos->value();
  emit modified();
}

void CalibrationPanel::on_ana8Pos_editingFinished()
{
  generalSettings.calibSpanPos[7] = ui->ana8Pos->value();
  emit modified();
}

void CalibrationPanel::on_serialPortMode_currentIndexChanged(int index)
{
  generalSettings.hw_uartMode = index;
  emit modified();
}
