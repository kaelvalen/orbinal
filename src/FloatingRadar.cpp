#include "FloatingRadar.h"
#include <QPainter>
#include <QPen>
#include <QRadialGradient>
#include <QConicalGradient>
#include <QApplication>
#include <QScreen>
#include <QTimer>
#include <cmath>

static const QColor kBg      = QColor(2, 10, 14);
static const QColor kCyan    = QColor(0, 215, 190);
static const QColor kDimCyan = QColor(0, 70, 60);
static const QColor kGrid    = QColor(0, 50, 44);
static const QColor kBorder  = QColor(0, 80, 70);

FloatingRadar::FloatingRadar(SatelliteModel *model, QWidget *parent)
    : QWidget(parent), m_model(model)
{
    setAttribute(Qt::WA_OpaquePaintEvent);
    setFixedSize(m_size, m_size);
    setStyleSheet("background: transparent;");
    setMouseTracking(false);
}

void FloatingRadar::refresh() { update(); }

void FloatingRadar::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    int cx = width() / 2;
    int cy = height() / 2;
    int r  = (qMin(width(), height()) - 16) / 2;

    // Background with border
    p.fillRect(rect(), kBg);
    p.setPen(QPen(kBorder, 2));
    p.setBrush(QColor(0, 18, 16));
    p.drawEllipse(cx - r, cy - r, r * 2, r * 2);

    // Grid rings
    for (int ri = 1; ri <= 4; ri++) {
        int rr = r * ri / 4;
        p.setPen(QPen(kGrid, 1, Qt::DotLine));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(cx - rr, cy - rr, rr * 2, rr * 2);
    }

    // Grid axes
    p.setPen(QPen(kGrid, 1));
    p.drawLine(cx - r, cy, cx + r, cy);
    p.drawLine(cx, cy - r, cx, cy + r);
    float d45 = r * 0.707f;
    p.drawLine(cx - (int)d45, cy - (int)d45, cx + (int)d45, cy + (int)d45);
    p.drawLine(cx + (int)d45, cy - (int)d45, cx - (int)d45, cy + (int)d45);

    // Sweep gradient
    float sweepRad = m_model->radarSweepAngle * float(M_PI) / 180.f;
    QConicalGradient cg(cx, cy, -m_model->radarSweepAngle + 90.f);
    cg.setColorAt(0.00, QColor(0, 215, 160, 160));
    cg.setColorAt(0.10, QColor(0, 215, 160, 60));
    cg.setColorAt(0.25, QColor(0, 215, 160, 0));
    cg.setColorAt(1.00, QColor(0, 215, 160, 0));

    p.save();
    p.setClipRegion(QRegion(cx - r, cy - r, r * 2, r * 2, QRegion::Ellipse));
    p.setBrush(cg);
    p.setPen(Qt::NoPen);
    p.drawEllipse(cx - r, cy - r, r * 2, r * 2);
    p.restore();

    // Sweep line
    p.setPen(QPen(kCyan, 1.5f));
    p.drawLine(cx, cy,
               cx + (int)(r * cosf(sweepRad - float(M_PI)/2.f)),
               cy + (int)(r * sinf(sweepRad - float(M_PI)/2.f)));

    // Blips
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

    // Outer border
    p.setPen(QPen(kCyan, 1));
    p.setBrush(Qt::NoBrush);
    p.drawEllipse(cx - r, cy - r, r * 2, r * 2);

    // Corner brackets
    p.setPen(QPen(kDimCyan, 1));
    int m = 4, bl = 8;
    p.drawLine(m, m, m+bl, m);                p.drawLine(m, m, m, m+bl);
    p.drawLine(width()-m, m, width()-m-bl, m); p.drawLine(width()-m, m, width()-m, m+bl);
    p.drawLine(m, height()-m, m+bl, height()-m); p.drawLine(m, height()-m, m, height()-m-bl);
    p.drawLine(width()-m, height()-m, width()-m-bl, height()-m); p.drawLine(width()-m, height()-m, width()-m, height()-m-bl);

    // Label
    QFont f("Courier", 6);
    f.setBold(true);
    p.setFont(f);
    p.setPen(kDimCyan);
    p.drawText(cx - 12, cy + r + 14, "RADAR");
    p.drawText(cx + r - 16, cy - r - 4, "360°");
}

void FloatingRadar::mousePressEvent(QMouseEvent *e) {
    if (e->button() == Qt::LeftButton) {
        m_dragging = true;
        m_dragPos  = e->position().toPoint();
        setCursor(Qt::ClosedHandCursor);
    }
}
void FloatingRadar::mouseMoveEvent(QMouseEvent *e) {
    if (m_dragging && parentWidget()) {
        QPoint newPos = pos() + (e->position().toPoint() - m_dragPos);
        // Clamp to parent bounds
        QRect parentRect = parentWidget()->rect();
        newPos.setX(qMax(0, qMin(newPos.x(), parentRect.width() - width())));
        newPos.setY(qMax(0, qMin(newPos.y(), parentRect.height() - height())));
        move(newPos);
        m_dragPos = e->position().toPoint();
    }
}
void FloatingRadar::mouseReleaseEvent(QMouseEvent *) {
    m_dragging = false;
    setCursor(Qt::OpenHandCursor);
}

void FloatingRadar::moveBy(const QPoint &delta) {
    if (parentWidget()) {
        QRect parentRect = parentWidget()->rect();
        QPoint newPos = pos() + delta;
        newPos.setX(qMax(0, qMin(newPos.x(), parentRect.width() - width())));
        newPos.setY(qMax(0, qMin(newPos.y(), parentRect.height() - height())));
        move(newPos);
    }
}

void FloatingRadar::enterEvent(QEvent *) {
    if (!m_dragging) setCursor(Qt::OpenHandCursor);
}
void FloatingRadar::leaveEvent(QEvent *) {
    if (!m_dragging) setCursor(Qt::ArrowCursor);
}

void FloatingRadar::resizeEvent(QResizeEvent *e) {
    m_size = qMin(e->size().width(), e->size().height());
    QWidget::resizeEvent(e);
}
