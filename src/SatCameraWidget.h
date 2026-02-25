#pragma once
#include <QWidget>
#include "SatelliteModel.h"

class SatCameraWidget : public QWidget {
    Q_OBJECT
public:
    explicit SatCameraWidget(SatelliteModel *model, QWidget *parent = nullptr);

    void refresh();
    void selectSatellite(int idx);
    int  selectedSatellite() const { return m_selectedIdx; }

protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *e) override;

private:
    SatelliteModel *m_model;
    int             m_selectedIdx = 0;
    float           m_noisePhase  = 0.f;
    float           m_time        = 0.f;

    static const QColor kBg;
    static const QColor kCyan;
    static const QColor kOrange;
    static const QColor kRed;
    static const QColor kDimCyan;
};
