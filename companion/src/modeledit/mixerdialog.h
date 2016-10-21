#ifndef MIXERDIALOG_H
#define MIXERDIALOG_H

#include <QDialog>
#include "eeprominterface.h"

class GVarGroup;
class CurveGroup;

namespace Ui {
  class MixerDialog;
}

class MixerDialog : public QDialog {
    Q_OBJECT
  public:
    MixerDialog(QWidget *parent, ModelData & model, MixData *mixdata, GeneralSettings & generalSettings, Firmware * firmware);
    ~MixerDialog();

  protected:
    void changeEvent(QEvent *e);

  private slots:
    void valuesChanged();
    void shrink();
    void label_phases_customContextMenuRequested(const QPoint & pos);
    void fmClearAll();
    void fmSetAll();
    void fmInvertAll();
    
  private:
    Ui::MixerDialog *ui;
    ModelData & model;
    GeneralSettings & generalSettings;
    Firmware * firmware;
    MixData *md;
    bool lock;
    GVarGroup * gvWeightGroup;
    GVarGroup * gvOffsetGroup;
    CurveGroup * curveGroup;
};

#endif // MIXERDIALOG_H
