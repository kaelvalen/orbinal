#pragma once
#include <QWidget>
#include "SatelliteModel.h"

class GeoSyncPanel : public QWidget {
    Q_OBJECT
public:
    explicit GeoSyncPanel(SatelliteModel *model, QWidget *parent = nullptr);

    void refresh();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void drawGeoSyncInfo(QPainter &p, int x, int y);
    void drawCbkUnits(QPainter &p, int x, int y);
    void drawSatPreview(QPainter &p, int x, int y, int w, int h);
    void drawSatDataColumns(QPainter &p, int x, int y);
    void drawScanlines(QPainter &p);
    void drawVignette(QPainter &p);
    void drawCornerBrackets(QPainter &p);

    SatelliteModel *m_model;

    static const QColor kBg;
    static const QColor kCyan;
    static const QColor kOrange;
    static const QColor kRed;
    static const QColor kDimCyan;
    static const QColor kHighlight;
};
