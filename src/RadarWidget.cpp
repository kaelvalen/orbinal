#include "RadarWidget.h"
#include <QPainter>
#include <QPen>
#include <QConicalGradient>
#include <cmath>

static const QColor kBg      = QColor(2, 10, 14);
static const QColor kCyan    = QColor(0, 215, 190);
static const QColor kDimCyan = QColor(0, 70, 60);
static const QColor kGrid    = QColor(0, 50, 44);
static const QColor kBorder  = QColor(0, 90, 78);

RadarWidget::RadarWidget(SatelliteModel *model, QWidget *parent)
    : QWidget(parent), m_model(model)
{
    setFixedSize(160, 160);
    setAttribute(Qt::WA_OpaquePaintEvent);
}

void RadarWidget::refresh() { update(); }

void RadarWidget::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    int cx = width()  / 2;
    int cy = height() / 2;
    int r  = (qMin(width(), height()) - 14) / 2;

    // Background fill
    p.fillRect(rect(), kBg);

    // Disc background
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0, 20, 16));
    p.drawEllipse(cx - r, cy - r, r * 2, r * 2);

    // Concentric rings
    for (int ri = 1; ri <= 4; ri++) {
        int rr = r * ri / 4;
        p.setPen(QPen(kGrid, 1, Qt::DotLine));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(cx - rr, cy - rr, rr * 2, rr * 2);
    }

    // Cross + diagonals
    p.setPen(QPen(kGrid, 1));
    p.drawLine(cx - r, cy, cx + r, cy);
    p.drawLine(cx, cy - r, cx, cy + r);
    float d45 = r * 0.707f;
    p.drawLine(cx-(int)d45, cy-(int)d45, cx+(int)d45, cy+(int)d45);
    p.drawLine(cx+(int)d45, cy-(int)d45, cx-(int)d45, cy+(int)d45);

    // Sweep conical glow
    QConicalGradient cg(cx, cy, -m_model->radarSweepAngle + 90.f);
    cg.setColorAt(0.00f, QColor(0, 220, 165, 170));
    cg.setColorAt(0.12f, QColor(0, 220, 165,  55));
    cg.setColorAt(0.28f, QColor(0, 220, 165,   0));
    cg.setColorAt(1.00f, QColor(0, 220, 165,   0));

    p.save();
    p.setClipRegion(QRegion(cx - r, cy - r, r * 2, r * 2, QRegion::Ellipse));
    p.setBrush(cg);
    p.setPen(Qt::NoPen);
    p.drawEllipse(cx - r, cy - r, r * 2, r * 2);
    p.restore();

    // Sweep line
    float sweepRad = m_model->radarSweepAngle * float(M_PI) / 180.f;
    p.setPen(QPen(kCyan, 1.5f));
    p.drawLine(cx, cy,
               cx + (int)(r * cosf(sweepRad - float(M_PI) / 2.f)),
               cy + (int)(r * sinf(sweepRad - float(M_PI) / 2.f)));

    // Blips
    for (const auto &blip : m_model->radarBlips) {
        float bRad  = blip.angle * float(M_PI) / 180.f;
        float bR    = blip.radius * r;
        int   bx    = cx + (int)(bR * cosf(bRad - float(M_PI) / 2.f));
        int   by    = cy + (int)(bR * sinf(bRad - float(M_PI) / 2.f));
        int   alpha = (int)(blip.life * 220);
        int   sz    = 3 + (int)((1.f - blip.life) * 4);

        p.setPen(Qt::NoPen);
        p.setBrush(QColor(blip.color.red(), blip.color.green(), blip.color.blue(), alpha / 3));
        p.drawEllipse(bx - sz, by - sz, sz * 2, sz * 2);
        p.setBrush(QColor(blip.color.red(), blip.color.green(), blip.color.blue(), alpha));
        p.drawEllipse(bx - 2, by - 2, 4, 4);
    }

    // Outer ring
    p.setPen(QPen(kBorder, 1));
    p.setBrush(Qt::NoBrush);
    p.drawEllipse(cx - r, cy - r, r * 2, r * 2);

    // Corner brackets
    p.setPen(QPen(kDimCyan, 1));
    int m = 3, bl = 8;
    p.drawLine(m, m, m+bl, m);                   p.drawLine(m, m, m, m+bl);
    p.drawLine(width()-m, m, width()-m-bl, m);    p.drawLine(width()-m, m, width()-m, m+bl);
    p.drawLine(m, height()-m, m+bl, height()-m);  p.drawLine(m, height()-m, m, height()-m-bl);
    p.drawLine(width()-m, height()-m, width()-m-bl, height()-m);
    p.drawLine(width()-m, height()-m, width()-m, height()-m-bl);

    // Labels
    QFont f("Courier", 6);
    f.setBold(true);
    p.setFont(f);
    p.setPen(kDimCyan);
    p.drawText(cx - 14, cy + r + 13, "RADAR");
    p.drawText(cx + r - 18, cy - r - 3, "360" + QString::fromUtf8("\xC2\xB0"));
}
