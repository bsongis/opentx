#include "expodialog.h"
#include "ui_expodialog.h"
#include "helpers.h"

ExpoDialog::ExpoDialog(QWidget *parent, ModelData & model, ExpoData *expoData, GeneralSettings & generalSettings, 
                          Firmware * firmware, QString & inputName) :
  QDialog(parent),
  ui(new Ui::ExpoDialog),
  model(model),
  generalSettings(generalSettings),
  firmware(firmware),
  ed(expoData),
  inputName(inputName),
  modelPrinter(firmware, generalSettings, model),
  lock(false)
{
  ui->setupUi(this);
  QLabel * lb_fp[] = {ui->lb_FP0,ui->lb_FP1,ui->lb_FP2,ui->lb_FP3,ui->lb_FP4,ui->lb_FP5,ui->lb_FP6,ui->lb_FP7,ui->lb_FP8 };
  QCheckBox * cb_fp[] = {ui->cb_FP0,ui->cb_FP1,ui->cb_FP2,ui->cb_FP3,ui->cb_FP4,ui->cb_FP5,ui->cb_FP6,ui->cb_FP7,ui->cb_FP8 };

  setWindowTitle(tr("Edit %1").arg(modelPrinter.printInputName(ed->chn)));
  QRegExp rx(CHAR_FOR_NAMES_REGEX);

  if (IS_TARANIS(GetEepromInterface()->getBoard())) {
    gvWeightGroup = new GVarGroup(ui->weightGV, ui->weightSB, ui->weightCB, ed->weight, model, 100, -100, 100);
    gvOffsetGroup = new GVarGroup(ui->offsetGV, ui->offsetSB, ui->offsetCB, ed->offset, model, 0, -100, 100);
  }
  else {
    gvWeightGroup = new GVarGroup(ui->weightGV, ui->weightSB, ui->weightCB, ed->weight, model, 100, 0, 100);
    gvOffsetGroup = NULL;
    ui->offsetLabel->hide();
    ui->offsetGV->hide();
    ui->offsetSB->hide();
    ui->offsetCB->hide();
  }

  curveGroup = new CurveGroup(ui->curveTypeCB, ui->curveGVarCB, ui->curveValueCB, ui->curveValueSB, ed->curve, model,
                              firmware->getCapability(HasInputDiff) ? 0 : (HIDE_DIFF | HIDE_NEGATIVE_CURVES));

  populateSwitchCB(ui->switchesCB, ed->swtch, generalSettings, MixesContext);

  ui->sideCB->setCurrentIndex(ed->mode-1);

  if (!firmware->getCapability(FlightModes)) {
    ui->label_phases->hide();
    for (int i=0; i<9; i++) {
      lb_fp[i]->hide();
      cb_fp[i]->hide();
    }
  }
  else {
    ui->label_phases->setToolTip(tr("Click to access popup menu"));
    ui->label_phases->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->label_phases, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(label_phases_customContextMenuRequested(const QPoint &)));
    int mask = 1;
    for (int i=0; i<9; i++) {
      if ((ed->flightModes & mask) == 0) {
        cb_fp[i]->setChecked(true);
      }
      mask <<= 1;
    }
    for (int i=firmware->getCapability(FlightModes); i<9; i++) {
      lb_fp[i]->hide();
      cb_fp[i]->hide();
    }
  }

  if (firmware->getCapability(VirtualInputs)) {
    ui->inputName->setMaxLength(4);
    populateSourceCB(ui->sourceCB, ed->srcRaw, generalSettings, &model, POPULATE_NONE | POPULATE_SOURCES | 
                                                  POPULATE_SWITCHES | POPULATE_TRIMS | POPULATE_TELEMETRY);
    ui->sourceCB->removeItem(0);
    ui->inputName->setValidator(new QRegExpValidator(rx, this));
    ui->inputName->setText(inputName);
  }
  else {
    ui->inputNameLabel->hide();
    ui->inputName->hide();
    ui->sourceLabel->hide();
    ui->sourceCB->hide();
    ui->trimLabel->hide();
    ui->trimCB->hide();
  }

  for(int i=0; i < NUM_STICKS; i++) {
    ui->trimCB->addItem(AnalogString(i), i+1);
  }
  ui->trimCB->setCurrentIndex(1 - ed->carryTrim);

  int expolength = firmware->getCapability(HasExpoNames);
  if (!expolength) {
    ui->lineNameLabel->hide();
    ui->lineName->hide();
  }
  else {
    ui->lineName->setMaxLength(expolength);
  }

  ui->lineName->setValidator(new QRegExpValidator(rx, this));
  ui->lineName->setText(ed->name);

  updateScale();
  valuesChanged();

  connect(ui->lineName, SIGNAL(editingFinished()), this, SLOT(valuesChanged()));
  connect(ui->sourceCB, SIGNAL(currentIndexChanged(int)), this, SLOT(valuesChanged()));
  connect(ui->scale, SIGNAL(editingFinished()), this, SLOT(valuesChanged()));
  connect(ui->trimCB, SIGNAL(currentIndexChanged(int)), this, SLOT(valuesChanged()));
  connect(ui->switchesCB, SIGNAL(currentIndexChanged(int)), this, SLOT(valuesChanged()));
  connect(ui->sideCB, SIGNAL(currentIndexChanged(int)), this, SLOT(valuesChanged()));
  for (int i=0; i<9; i++) {
    connect(cb_fp[i], SIGNAL(toggled(bool)), this, SLOT(valuesChanged()));
  }
  if (firmware->getCapability(VirtualInputs))
    connect(ui->inputName, SIGNAL(editingFinished()), this, SLOT(valuesChanged()));

  QTimer::singleShot(0, this, SLOT(shrink()));
}

ExpoDialog::~ExpoDialog()
{
  delete gvWeightGroup;
  delete gvOffsetGroup;
  delete curveGroup;
  delete ui;
}

void ExpoDialog::updateScale()
{
  if (firmware->getCapability(VirtualInputs) && ed->srcRaw.type == SOURCE_TYPE_TELEMETRY) {
    RawSourceRange range = ed->srcRaw.getRange(&model, generalSettings);
    ui->scaleLabel->show();
    ui->scale->show();
    ui->scale->setDecimals(range.decimals);
    ui->scale->setMinimum(range.min);
    ui->scale->setMaximum(range.max);
    ui->scale->setValue(round(range.step * ed->scale));
  }
  else {
    ui->scaleLabel->hide();
    ui->scale->hide();
  }
}

void ExpoDialog::valuesChanged()
{
  if (!lock) {
    lock = true;
    QCheckBox * cb_fp[] = {ui->cb_FP0,ui->cb_FP1,ui->cb_FP2,ui->cb_FP3,ui->cb_FP4,ui->cb_FP5,ui->cb_FP6,ui->cb_FP7,ui->cb_FP8 };

    RawSource srcRaw = RawSource(ui->sourceCB->itemData(ui->sourceCB->currentIndex()).toInt());
    if (ed->srcRaw != srcRaw) {
      ed->srcRaw = srcRaw;
      updateScale();
    }

    RawSourceRange range = srcRaw.getRange(&model, generalSettings);
    ed->scale = round(float(ui->scale->value()) / range.step);
    ed->carryTrim = 1 - ui->trimCB->currentIndex();
    ed->swtch = RawSwitch(ui->switchesCB->itemData(ui->switchesCB->currentIndex()).toInt());
    ed->mode = ui->sideCB->currentIndex() + 1;

    strcpy(ed->name, ui->lineName->text().toAscii().data());
    if (firmware->getCapability(VirtualInputs)) {
      inputName = ui->inputName->text();
    }

    ed->flightModes = 0;
    for (int i=8; i>=0 ; i--) {
      if (!cb_fp[i]->checkState()) {
        ed->flightModes++;
      }
      ed->flightModes <<= 1;
    }
    ed->flightModes >>= 1;
    lock = false;
  }
}

void ExpoDialog::shrink()
{
  resize(0, 0);
}
void ExpoDialog::label_phases_customContextMenuRequested(const QPoint & pos)
{
  QLabel *label = (QLabel *)sender();
  QPoint globalPos = label->mapToGlobal(pos);
  QMenu contextMenu;
  contextMenu.addAction(tr("Clear All"), this, SLOT(fmClearAll()));
  contextMenu.addAction(tr("Set All"), this, SLOT(fmSetAll()));
  contextMenu.addAction(tr("Invert All"), this, SLOT(fmInvertAll()));
  contextMenu.exec(globalPos);
}

void ExpoDialog::fmClearAll()
{
  lock = true;
  QCheckBox * cb_fp[] = {ui->cb_FP0,ui->cb_FP1,ui->cb_FP2,ui->cb_FP3,ui->cb_FP4,ui->cb_FP5,ui->cb_FP6,ui->cb_FP7,ui->cb_FP8 };
  for (int i=0; i<9 ; i++) {
    cb_fp[i]->setChecked(false);
  }
  lock = false;
  valuesChanged();
}

void ExpoDialog::fmSetAll()
{
  lock = true;
  QCheckBox * cb_fp[] = {ui->cb_FP0,ui->cb_FP1,ui->cb_FP2,ui->cb_FP3,ui->cb_FP4,ui->cb_FP5,ui->cb_FP6,ui->cb_FP7,ui->cb_FP8 };
  for (int i=0; i<9 ; i++) {
    cb_fp[i]->setChecked(true);
  }
  lock = false;
  valuesChanged();
}

void ExpoDialog::fmInvertAll()
{
  lock = true;
  QCheckBox * cb_fp[] = {ui->cb_FP0,ui->cb_FP1,ui->cb_FP2,ui->cb_FP3,ui->cb_FP4,ui->cb_FP5,ui->cb_FP6,ui->cb_FP7,ui->cb_FP8 };
  for (int i=0; i<9 ; i++) {
    cb_fp[i]->setChecked(not cb_fp[i]->checkState());
  }
  lock = false;
  valuesChanged();
}

