#pragma once
#include <QWidget>
#include "SatelliteModel.h"

class RadarSweepWidget : public QWidget {
    Q_OBJECT
public:
    explicit RadarSweepWidget(SatelliteModel *model, QWidget *parent = nullptr);
    void refresh();

protected:
    void paintEvent(QPaintEvent *) override;

private:
    SatelliteModel *m_model;
};
