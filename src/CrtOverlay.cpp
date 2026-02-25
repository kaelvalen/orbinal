#include "CrtOverlay.h"
#include <QPainter>
#include <QPen>
#include <QRadialGradient>
#include <cmath>

CrtOverlay::CrtOverlay(QWidget *parent) : QWidget(parent) {
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_TranslucentBackground);
    setWindowFlags(Qt::FramelessWindowHint);
}

void CrtOverlay::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false);

    // Scanlines
    for (int y = 0; y < height(); y += 2) {
        p.setPen(QColor(0, 0, 0, 32));
        p.drawLine(0, y, width(), y);
    }

    // Radial vignette
    QRadialGradient vg(width() / 2, height() / 2, qMax(width(), height()) * 0.75f);
    vg.setColorAt(0.0, QColor(0, 0, 0,  0));
    vg.setColorAt(0.5, QColor(0, 0, 0,  0));
    vg.setColorAt(0.8, QColor(0, 0, 0, 35));
    vg.setColorAt(1.0, QColor(0, 0, 0,110));
    p.setBrush(vg);
    p.setPen(Qt::NoPen);
    p.drawRect(rect());

    // Phosphor green tint (very subtle)
    p.setBrush(QColor(0, 30, 20, 8));
    p.drawRect(rect());

    // Alert pulse — subtle amber border only, no full-screen tint
    if (m_flash > 0.01f) {
        int alpha = (int)(m_flash * 160);
        int bw = 2;
        p.setBrush(Qt::NoBrush);
        p.setPen(QPen(QColor(220, 140, 0, alpha), bw));
        p.drawRect(bw, bw, width() - bw * 2, height() - bw * 2);

        // Corner accent dots
        int cs = 12;
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(220, 140, 0, alpha / 2));
        p.drawRect(0, 0, cs, cs);
        p.drawRect(width() - cs, 0, cs, cs);
        p.drawRect(0, height() - cs, cs, cs);
        p.drawRect(width() - cs, height() - cs, cs, cs);
    }
}
