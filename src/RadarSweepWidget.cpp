#include "RadarSweepWidget.h"
#include <QPainter>
#include <QPen>
#include <QRadialGradient>
#include <QConicalGradient>
#include <cmath>

static const QColor kBg      = QColor(2, 10, 14);
static const QColor kCyan    = QColor(0, 215, 190);
static const QColor kDimCyan = QColor(0, 70, 60);
static const QColor kGrid    = QColor(0, 50, 44);

RadarSweepWidget::RadarSweepWidget(SatelliteModel *model, QWidget *parent)
    : QWidget(parent), m_model(model)
{
    setMinimumSize(160, 160);
    setMaximumSize(200, 200);
    setAttribute(Qt::WA_OpaquePaintEvent);
}

void RadarSweepWidget::refresh() { update(); }

void RadarSweepWidget::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    int  side = qMin(width(), height()) - 16;
    int  cx   = width()  / 2;
    int  cy   = height() / 2;
    int  r    = side / 2;

    p.fillRect(rect(), kBg);

    p.setPen(QPen(kDimCyan, 1));
    p.setBrush(QColor(0, 18, 16));
    p.drawEllipse(cx - r, cy - r, side, side);

    for (int ri = 1; ri <= 4; ri++) {
        int rr = r * ri / 4;
        p.setPen(QPen(kGrid, 1, Qt::DotLine));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(cx - rr, cy - rr, rr * 2, rr * 2);
    }
    p.setPen(QPen(kGrid, 1));
    p.drawLine(cx - r, cy, cx + r, cy);
    p.drawLine(cx, cy - r, cx, cy + r);
    float d45 = r * 0.707f;
    p.drawLine(cx - (int)d45, cy - (int)d45, cx + (int)d45, cy + (int)d45);
    p.drawLine(cx + (int)d45, cy - (int)d45, cx - (int)d45, cy + (int)d45);

    float sweepRad = m_model->radarSweepAngle * float(M_PI) / 180.f;

    QConicalGradient cg(cx, cy, -m_model->radarSweepAngle + 90.f);
    cg.setColorAt(0.00, QColor(0, 215, 160, 160));
    cg.setColorAt(0.10, QColor(0, 215, 160, 60));
    cg.setColorAt(0.25, QColor(0, 215, 160, 0));
    cg.setColorAt(1.00, QColor(0, 215, 160, 0));

    p.save();
    p.setClipRegion(QRegion(cx - r, cy - r, side, side, QRegion::Ellipse));
    p.setBrush(cg);
    p.setPen(Qt::NoPen);
    p.drawEllipse(cx - r, cy - r, side, side);
    p.restore();

    p.setPen(QPen(kCyan, 1.5f));
    p.drawLine(cx, cy,
               cx + (int)(r * cosf(sweepRad - float(M_PI)/2.f)),
               cy + (int)(r * sinf(sweepRad - float(M_PI)/2.f)));

    for (const auto &blip : m_model->radarBlips) {
        float bRad  = blip.angle * float(M_PI) / 180.f;
        float bR    = blip.radius * r;
        int   bx    = cx + (int)(bR * cosf(bRad - float(M_PI)/2.f));
        int   by    = cy + (int)(bR * sinf(bRad - float(M_PI)/2.f));
        int   alpha = (int)(blip.life * 220);
        int   size  = 3 + (int)((1.f - blip.life) * 4);

        p.setPen(Qt::NoPen);
        p.setBrush(QColor(blip.color.red(), blip.color.green(), blip.color.blue(), alpha / 3));
        p.drawEllipse(bx - size, by - size, size * 2, size * 2);
        p.setBrush(QColor(blip.color.red(), blip.color.green(), blip.color.blue(), alpha));
        p.drawEllipse(bx - 2, by - 2, 4, 4);
    }

    p.setPen(QPen(kCyan, 1));
    p.setBrush(Qt::NoBrush);
    p.drawEllipse(cx - r, cy - r, side, side);

    p.setPen(QPen(kDimCyan, 1));
    int m = 4, bl = 10;
    int x1 = cx - r - 4, y1 = cy - r - 4;
    int x2 = cx + r + 4, y2 = cy + r + 4;
    p.drawLine(x1, y1, x1 + bl, y1);       p.drawLine(x1, y1, x1, y1 + bl);
    p.drawLine(x2, y1, x2 - bl, y1);       p.drawLine(x2, y1, x2, y1 + bl);
    p.drawLine(x1, y2, x1 + bl, y2);       p.drawLine(x1, y2, x1, y2 - bl);
    p.drawLine(x2, y2, x2 - bl, y2);       p.drawLine(x2, y2, x2, y2 - bl);

    QFont f("Courier", 6);
    f.setBold(true);
    p.setFont(f);
    p.setPen(kDimCyan);
    p.drawText(cx - 12, cy + r + 14, "RADAR");
    p.drawText(cx + r - 16, cy - r - 4, "360°");
}
