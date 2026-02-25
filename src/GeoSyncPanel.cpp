#include "GeoSyncPanel.h"
#include <QPainter>
#include <QPen>
#include <QPainterPath>
#include <QRadialGradient>
#include <QLinearGradient>
#include <QFont>
#include <QFontMetrics>
#include <QDateTime>
#include <cmath>

const QColor GeoSyncPanel::kBg        = QColor(2, 10, 14);
const QColor GeoSyncPanel::kCyan      = QColor(0, 215, 190);
const QColor GeoSyncPanel::kOrange    = QColor(225, 115, 15);
const QColor GeoSyncPanel::kRed       = QColor(215, 38, 38);
const QColor GeoSyncPanel::kDimCyan   = QColor(0, 95, 85);
const QColor GeoSyncPanel::kHighlight = QColor(200, 80, 20);

GeoSyncPanel::GeoSyncPanel(SatelliteModel *model, QWidget *parent)
    : QWidget(parent), m_model(model)
{
    setMinimumSize(480, 360);
    setAttribute(Qt::WA_OpaquePaintEvent);
}

void GeoSyncPanel::refresh() {
    update();
}

static QString utcTimestamp() {
    return QDateTime::currentDateTimeUtc().toString("yyyy-MM-dd  HH:mm:ss.zzz") + " UTC";
}

void GeoSyncPanel::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false);
    p.fillRect(rect(), kBg);

    const int M = 10;
    const int W = width();

    // ── Title bar ────────────────────────────────────────────────
    QFont titleFont("Courier", 10);
    titleFont.setBold(true);
    p.setFont(titleFont);
    p.setPen(kCyan);
    p.drawText(M, M + 14, "GEO-SYNC TELEMETRY  //  UPLINK ACTIVE");

    // UTC timestamp right-aligned
    QFont tsFont("Courier", 7);
    p.setFont(tsFont);
    p.setPen(kDimCyan);
    QString ts = utcTimestamp();
    QFontMetrics tsFm(tsFont);
    p.drawText(W - M - tsFm.horizontalAdvance(ts), M + 14, ts);

    // Top separator with gradient effect
    QLinearGradient sepGrad(M, 0, W - M, 0);
    sepGrad.setColorAt(0.0f, QColor(0,  60, 55));
    sepGrad.setColorAt(0.3f, QColor(0, 215, 190));
    sepGrad.setColorAt(0.7f, QColor(0, 215, 190));
    sepGrad.setColorAt(1.0f, QColor(0,  60, 55));
    p.setPen(QPen(QBrush(sepGrad), 1));
    p.drawLine(M, M + 20, W - M, M + 20);

    int topY = M + 30;
    int leftW  = 148;
    int cbkX   = M + leftW + 8;
    int cbkW   = 108;
    int prevX  = cbkX + cbkW + 8;
    int prevW  = W - prevX - M;

    drawGeoSyncInfo(p, M, topY);
    drawCbkUnits(p, cbkX, topY);
    drawSatPreview(p, prevX, topY - 2, prevW, 118);

    // Mid separator
    int satDataY = topY + 124;
    p.setPen(QPen(kDimCyan, 1));
    p.drawLine(M, satDataY - 5, W - M, satDataY - 5);
    QFont secFont("Courier", 7);
    secFont.setBold(true);
    p.setFont(secFont);
    p.setPen(QColor(0, 130, 115));
    p.drawText(M + 2, satDataY - 8, "SATELLITE TELEMETRY READOUTS");

    drawSatDataColumns(p, M, satDataY);

    drawScanlines(p);
    drawVignette(p);
    drawCornerBrackets(p);
}

void GeoSyncPanel::drawGeoSyncInfo(QPainter &p, int x, int y) {
    QFont font("Courier", 8);
    font.setBold(true);
    p.setFont(font);
    QFontMetrics fm(font);
    int lh = fm.height() + 3;

    const auto &gs = m_model->geoSync;

    struct Row { QString label; QString value; bool highlight; bool isCritical; };
    bool orbitCrit = gs.orbitChanged;
    QVector<Row> rows = {
        {"GEO SYNC",    "",             false, false},
        {"NETWORK:",    gs.network,     false, false},
        {"ACCESS ID",   gs.accessId,    false, false},
        {"PORT:",       gs.port,        true,  false},
        {"CALIBRATION",gs.calibration, false, false},
        {"LAYERS:",     gs.layers,      false, false},
        {"ALIGNMENT:",  gs.alignment,   false, false},
        {"NAV:",        gs.nav,         false, false},
        {"ORBIT:",      gs.orbit,       orbitCrit, orbitCrit},
    };

    for (int i = 0; i < rows.size(); i++) {
        const auto &row = rows[i];
        int ry = y + i * lh;

        if (row.label == "GEO SYNC") {
            p.setPen(kCyan);
            p.drawText(x, ry + fm.ascent(), row.label);
            continue;
        }

        if (row.label == "ORBIT:" && gs.orbitChanged) {
            if (gs.blinkOn) {
                p.setPen(QPen(kRed, 1));
                QString fullText = row.label + " " + row.value;
                int tw = fm.horizontalAdvance(fullText) + 6;
                p.drawRect(x - 2, ry - 1, tw, lh - 1);
                p.setPen(kRed);
                p.drawText(x, ry + fm.ascent(), fullText);
            } else {
                p.setPen(kDimCyan);
                p.drawText(x, ry + fm.ascent(), row.label + " " + row.value);
            }
            continue;
        }

        if (row.highlight) {
            p.setPen(QPen(kOrange, 1));
            QString fullText = row.label + " " + row.value;
            int tw = fm.horizontalAdvance(fullText) + 6;
            p.drawRect(x - 2, ry - 1, tw, lh - 1);
            p.setPen(kOrange);
            p.drawText(x, ry + fm.ascent(), fullText);
        } else {
            p.setPen(kCyan);
            p.drawText(x, ry + fm.ascent(), row.label + " " + row.value);
        }
    }
}

void GeoSyncPanel::drawCbkUnits(QPainter &p, int x, int y) {
    QFont font("Courier", 8);
    font.setBold(true);
    p.setFont(font);
    QFontMetrics fm(font);
    int lh = fm.height() + 6;

    const auto &units = m_model->cbkUnits;
    for (int i = 0; i < units.size(); i++) {
        const auto &u = units[i];
        int uy = y + i * lh;
        int bw = fm.horizontalAdvance(u.id) + 10;
        int bh = lh - 2;

        if (!u.online) {
            p.setPen(QPen(kDimCyan, 1));
            p.drawRect(x, uy, bw, bh);
            p.setPen(kDimCyan);
            p.drawText(x + 5, uy + fm.ascent() + 1, u.id);
        } else if (u.highlighted) {
            p.setPen(QPen(kOrange, 1));
            p.drawRect(x, uy, bw, bh);
            p.setPen(kOrange);
            p.drawText(x + 5, uy + fm.ascent() + 1, u.id);
        } else {
            p.setPen(QPen(kCyan, 1));
            p.drawRect(x, uy, bw, bh);
            p.setPen(kCyan);
            p.drawText(x + 5, uy + fm.ascent() + 1, u.id);
        }
    }
}

void GeoSyncPanel::drawSatPreview(QPainter &p, int x, int y, int w, int h) {
    p.setRenderHint(QPainter::Antialiasing, true);

    // Panel border + background
    p.setPen(QPen(kDimCyan, 1));
    p.setBrush(QColor(0, 8, 12));
    p.drawRect(x, y, w, h);
    p.setBrush(Qt::NoBrush);

    // Header
    QFont hf("Courier", 6);
    hf.setBold(true);
    p.setFont(hf);
    p.setPen(QColor(0, 110, 95));
    p.drawText(x + 4, y + 10, "ORBIT PHASE");
    p.setPen(QPen(QColor(0, 60, 52), 1));
    p.drawLine(x + 2, y + 13, x + w - 2, y + 13);

    int cx = x + w / 2;
    int cy = y + 16 + (h - 26) / 2;
    int maxR = (qMin(w, h - 20)) / 2 - 4;

    // Earth dot
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0, 80, 100));
    p.drawEllipse(cx - 5, cy - 5, 10, 10);
    p.setBrush(QColor(0, 160, 180, 80));
    p.drawEllipse(cx - 7, cy - 7, 14, 14);

    // Draw each satellite orbit ring + position dot
    const auto &sats = m_model->satellites;
    const bool blink = m_model->geoSync.blinkOn;

    for (int si = 0; si < (int)sats.size(); si++) {
        const auto &sat = sats[si];
        // Scale orbit radius to preview space (orbitRadius ~1.2-1.6)
        float normR = (sat.orbitRadius - 1.0f) / 0.8f;  // 0..1
        int r = (int)(maxR * (0.45f + normR * 0.55f));

        // Orbit ellipse (inclined = squished vertically)
        float incFactor = cosf(sat.orbitInclination * float(M_PI) / 180.f);
        int ry2 = qMax(4, (int)(r * incFactor));

        QColor orbitCol = sat.orbitColor;
        orbitCol.setAlpha(60);
        p.setPen(QPen(orbitCol, 1, Qt::DotLine));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(cx - r, cy - ry2, r * 2, ry2 * 2);

        // Satellite position on the ring
        float angle = sat.satAngle;  // radians
        int sx = cx + (int)(r   * cosf(angle - float(M_PI)/2.f));
        int sy = cy + (int)(ry2 * sinf(angle - float(M_PI)/2.f));

        // Alert ring
        bool hasAlert = false;
        for (const auto &rd : sat.readings)
            if (rd.alert == AlertLevel::Critical) hasAlert = true;

        if (hasAlert && blink) {
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(200, 40, 40, 60));
            p.drawEllipse(sx - 6, sy - 6, 12, 12);
        }

        // Satellite dot
        QColor dotCol = sat.orbitColor;
        p.setPen(Qt::NoPen);
        p.setBrush(dotCol);
        p.drawEllipse(sx - 3, sy - 3, 6, 6);

        // Label
        QFont lf("Courier", 5);
        p.setFont(lf);
        p.setPen(dotCol);
        p.drawText(sx + 5, sy + 4, sat.name.left(4));
    }

    // Crosshair at centre
    p.setPen(QPen(QColor(0, 80, 70), 1));
    p.drawLine(cx - 8, cy, cx + 8, cy);
    p.drawLine(cx, cy - 8, cx, cy + 8);

    // Footer label
    QFont ff("Courier", 6);
    p.setFont(ff);
    p.setPen(QColor(0, 80, 70));
    QString phaseStr = QString("T+%1s").arg((int)m_model->time(), 5);
    QFontMetrics ffm(ff);
    p.drawText(x + w - ffm.horizontalAdvance(phaseStr) - 3, y + h - 3, phaseStr);
    p.drawText(x + 3, y + h - 3, "TOP VIEW");
    p.setRenderHint(QPainter::Antialiasing, false);
}

// Returns a unit string and scaling for a reading id
static void telemetryFormat(const QString &id, float value,
                             QString &outVal, QString &outUnit) {
    if (id.startsWith("FREQ") || id.startsWith("UPLINK") || id.startsWith("DNLINK")) {
        float mhz = 2400.f + value * 40.f;
        outVal = QString("%1").arg((double)mhz, 8, 'f', 3);
        outUnit = "MHz";
    } else if (id.startsWith("SNR") || id.startsWith("NOISE") || id.startsWith("RSSI")) {
        float dbm = -80.f + value * 30.f;
        outVal = QString("%1").arg((double)dbm, 7, 'f', 2);
        outUnit = "dBm";
    } else if (id.startsWith("TEMP")) {
        float k = 250.f + value * 60.f;
        outVal = QString("%1").arg((double)k, 7, 'f', 1);
        outUnit = "K   ";
    } else if (id.startsWith("LATENCY") || id.startsWith("PING")) {
        float ms = 20.f + value * 80.f;
        outVal = QString("%1").arg((double)ms, 7, 'f', 2);
        outUnit = "ms  ";
    } else if (id.startsWith("ALT") || id.startsWith("ORBIT")) {
        float km = 400.f + value * 600.f;
        outVal = QString("%1").arg((double)km, 8, 'f', 1);
        outUnit = "km  ";
    } else if (id.startsWith("VEL")) {
        float kms = 7.5f + value * 0.5f;
        outVal = QString("%1").arg((double)kms, 7, 'f', 3);
        outUnit = "km/s";
    } else {
        outVal = QString("%1").arg((double)value, 7, 'f', 3);
        outUnit = "    ";
    }
}

void GeoSyncPanel::drawSatDataColumns(QPainter &p, int x, int y) {
    QFont headerFont("Courier", 9);
    headerFont.setBold(true);
    QFont dataFont("Courier", 7);
    dataFont.setBold(true);
    QFont unitFont("Courier", 7);
    QFontMetrics fmH(headerFont);
    QFontMetrics fmD(dataFont);
    QFontMetrics fmU(unitFont);
    int lh   = fmD.height() + 3;
    int colW = (width() - x * 2) / (int)m_model->satellites.size();

    const auto &sats  = m_model->satellites;
    const bool  blink = m_model->geoSync.blinkOn;

    for (int col = 0; col < (int)sats.size(); col++) {
        const auto &sat = sats[col];
        int cx = x + col * colW;

        // Column divider
        if (col > 0) {
            p.setPen(QPen(kDimCyan, 1));
            p.drawLine(cx - 5, y - 4, cx - 5,
                       y + fmH.height() + lh * (int)sat.readings.size() + 10);
        }

        // Satellite name header with color-coded alert
        bool satHasAlert = false;
        for (const auto &rd : sat.readings)
            if (rd.alert == AlertLevel::Critical) satHasAlert = true;

        p.setFont(headerFont);
        QColor hdrCol = (satHasAlert && blink) ? kRed
                      : (col == 2)            ? kOrange
                                               : kCyan;
        p.setPen(hdrCol);
        p.drawText(cx, y + fmH.ascent(), sat.name);

        // Sub-header: orbit radius
        QFont tiny("Courier", 6);
        p.setFont(tiny);
        p.setPen(kDimCyan);
        p.drawText(cx, y + fmH.height() + 2,
                   QString("R=%1km  INC=%2deg")
                       .arg((int)(sat.orbitRadius * 6371))
                       .arg((int)sat.orbitInclination));

        p.setFont(dataFont);
        int rowBaseY = y + fmH.height() + lh + 2;

        for (int r = 0; r < (int)sat.readings.size(); r++) {
            const auto &rd = sat.readings[r];
            int ry = rowBaseY + r * lh;

            if (rd.id == "----") {
                p.setPen(QColor(0, 50, 45));
                p.drawText(cx, ry + fmD.ascent(), "...  ........");
                continue;
            }

            QString valStr, unitStr;
            telemetryFormat(rd.id, rd.value, valStr, unitStr);

            // ID field (fixed 8 chars)
            QString idField = rd.id.leftJustified(8, ' ');

            QColor rowCol;
            if (rd.highlighted) {
                rowCol = (rd.alert == AlertLevel::Critical) ? kRed : kOrange;
                bool showBox = (rd.alert != AlertLevel::Critical) || blink;
                if (showBox) {
                    // Highlight box behind the row
                    int bw = colW - 10;
                    p.setPen(QPen(rowCol, 1));
                    QColor fillCol = rowCol; fillCol.setAlpha(18);
                    p.setBrush(fillCol);
                    p.drawRect(cx - 2, ry - 1, bw, lh - 1);
                    p.setBrush(Qt::NoBrush);
                }
            } else {
                rowCol = kCyan;
            }

            // Draw: ID  VALUE  UNIT
            p.setFont(dataFont);
            p.setPen(kDimCyan);
            p.drawText(cx, ry + fmD.ascent(), idField);

            int vx = cx + fmD.horizontalAdvance("XXXXXXXX") + 2;
            p.setPen(rowCol);
            p.drawText(vx, ry + fmD.ascent(), valStr);

            p.setFont(unitFont);
            p.setPen(QColor(0, 140, 120));
            int ux = vx + fmD.horizontalAdvance(valStr) + 3;
            p.drawText(ux, ry + fmU.ascent(), unitStr);
            p.setFont(dataFont);
        }
    }
}

void GeoSyncPanel::drawScanlines(QPainter &p) {
    for (int y = 0; y < height(); y += 3) {
        p.setPen(QColor(0, 0, 0, 22));
        p.drawLine(0, y, width(), y);
    }
}

void GeoSyncPanel::drawVignette(QPainter &p) {
    QRadialGradient grad(width() / 2, height() / 2, qMax(width(), height()) * 0.65f);
    grad.setColorAt(0.0, QColor(0, 0, 0, 0));
    grad.setColorAt(0.6, QColor(0, 0, 0, 0));
    grad.setColorAt(1.0, QColor(0, 0, 0, 90));
    p.setBrush(grad);
    p.setPen(Qt::NoPen);
    p.drawRect(rect());
}

void GeoSyncPanel::drawCornerBrackets(QPainter &p) {
    int m = 6, bLen = 14;
    p.setPen(QPen(kDimCyan, 1));
    int w = width(), h = height();
    p.drawLine(m, m, m + bLen, m);         p.drawLine(m, m, m, m + bLen);
    p.drawLine(w-m, m, w-m-bLen, m);       p.drawLine(w-m, m, w-m, m+bLen);
    p.drawLine(m, h-m, m+bLen, h-m);       p.drawLine(m, h-m, m, h-m-bLen);
    p.drawLine(w-m, h-m, w-m-bLen, h-m);   p.drawLine(w-m, h-m, w-m, h-m-bLen);
}
