#pragma once
#include <QWidget>
#include "SatelliteModel.h"

// Compact radar widget — embedded in layout, no floating/dragging
class RadarWidget : public QWidget {
    Q_OBJECT
public:
    explicit RadarWidget(SatelliteModel *model, QWidget *parent = nullptr);
    void refresh();

protected:
    void paintEvent(QPaintEvent *) override;

private:
    SatelliteModel *m_model;
};
