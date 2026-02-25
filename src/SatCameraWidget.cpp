#include "SatCameraWidget.h"
#include <QPainter>
#include <QPen>
#include <QPainterPath>
#include <QLinearGradient>
#include <QRadialGradient>
#include <QConicalGradient>
#include <QFont>
#include <QFontMetrics>
#include <QMouseEvent>
#include <cmath>
#include <cstdlib>

const QColor SatCameraWidget::kBg      = QColor(2, 10, 14);
const QColor SatCameraWidget::kCyan    = QColor(0, 215, 190);
const QColor SatCameraWidget::kOrange  = QColor(225, 115, 15);
const QColor SatCameraWidget::kRed     = QColor(215, 38, 38);
const QColor SatCameraWidget::kDimCyan = QColor(0, 95, 85);

// ── LCG helpers ──────────────────────────────────────────────────────────────
static inline float lcg(unsigned &s) {
    s = s * 1664525u + 1013904223u;
    return (s >> 16) / 65535.f;
}

// ── Constructor ──────────────────────────────────────────────────────────────
SatCameraWidget::SatCameraWidget(SatelliteModel *model, QWidget *parent)
    : QWidget(parent), m_model(model)
{
    setMinimumHeight(180);
    setMaximumHeight(280);
    setAttribute(Qt::WA_OpaquePaintEvent);
}

void SatCameraWidget::refresh() {
    m_noisePhase += 0.016f;
    m_time       += 0.016f;
    update();
}

void SatCameraWidget::selectSatellite(int idx) {
    if (idx >= 0 && idx < m_model->satellites.size())
        m_selectedIdx = idx;
    update();
}

// ── Star field ───────────────────────────────────────────────────────────────
static void drawStarField(QPainter &p, const QRect &r, float satAngle, float time) {
    p.setRenderHint(QPainter::Antialiasing, false);
    unsigned seed = 0xCAFEBABE;
    const int N = 120;
    float panX = fmodf(satAngle * 18.f, (float)r.width());
    for (int i = 0; i < N; i++) {
        float nx  = lcg(seed), ny = lcg(seed);
        float mag = lcg(seed), col = lcg(seed);
        float sx = r.x() + fmodf(nx * r.width() * 2.f + panX + r.width(), (float)r.width());
        float sy = r.y() + ny * r.height() * 0.50f;
        float twinkle = 0.72f + 0.28f * sinf(time * (1.8f + mag * 3.f) + i * 0.9f);
        int   bright  = (int)((0.35f + 0.65f * (1.f - mag)) * twinkle * 255);
        if (bright < 28) continue;
        int r2 = bright, g2 = bright, b2 = bright;
        if (col < 0.12f) { b2 = (int)(bright*0.7f); }
        else if (col < 0.28f) { r2 = (int)(bright*0.82f); b2 = bright; }
        if (mag < 0.15f) {
            p.setPen(QColor(r2, g2, b2, bright));
            p.drawPoint(QPointF(sx, sy));
            p.setPen(QColor(r2, g2, b2, bright/3));
            p.drawLine(QPointF(sx-1, sy), QPointF(sx+1, sy));
            p.drawLine(QPointF(sx, sy-1), QPointF(sx, sy+1));
        } else {
            p.setPen(QColor(r2, g2, b2, bright));
            p.drawPoint(QPointF(sx, sy));
        }
    }
}

// ── Earth limb ────────────────────────────────────────────────────────────────
static void drawEarthLimb(QPainter &p, const QRect &r,
                          float satAngle, float orbitRadius,
                          float sunAngle, float time)
{
    p.setRenderHint(QPainter::Antialiasing, true);

    float earthAngRad = asinf(1.f / orbitRadius);
    float fovHalf     = 35.f * float(M_PI) / 180.f;
    float earthFrac   = earthAngRad / fovHalf;

    int   cx      = r.x() + r.width() / 2;
    float pitch   = sinf(satAngle * 0.41f) * 4.f;
    float roll    = sinf(satAngle * 0.23f) * 6.f;

    int   earthR  = (int)(earthFrac * r.height() * 1.05f);
    int   horizY  = r.y() + r.height() - (int)(r.height() * 0.30f) + (int)pitch;
    int   ecx     = cx + (int)roll;
    int   ecy     = horizY + earthR;

    float sunRad  = sunAngle * float(M_PI) / 180.f;
    float sunVx   = cosf(sunRad);
    float sunVy   = sinf(sunRad) * 0.3f;

    p.save();
    p.setClipRect(r);

    // Earth base gradient
    float litDx = sunVx * earthR * 0.8f;
    float litDy = -fabsf(sunVy) * earthR * 0.5f;
    QRadialGradient earthGrad(ecx + litDx, ecy + litDy, earthR * 1.8f);
    earthGrad.setColorAt(0.00f, QColor(6,  70, 90));
    earthGrad.setColorAt(0.25f, QColor(4,  55, 75));
    earthGrad.setColorAt(0.50f, QColor(3,  40, 55));
    earthGrad.setColorAt(0.72f, QColor(5,  28, 38));
    earthGrad.setColorAt(0.88f, QColor(2,  12, 18));
    earthGrad.setColorAt(1.00f, QColor(1,   5,  9));
    p.setBrush(earthGrad);
    p.setPen(Qt::NoPen);
    p.drawEllipse(QRectF(ecx - earthR, ecy - earthR, earthR * 2, earthR * 2));

    // Latitude band lines
    for (int b = 0; b < 6; b++) {
        float frac = (float)b / 6;
        float ey   = ecy - earthR + frac * earthR * 1.8f;
        if (ey < r.y() || ey > r.y() + r.height()) continue;
        float dy  = ey - ecy;
        float hw2 = (float)earthR*(float)earthR - dy*dy;
        if (hw2 <= 0) continue;
        float hw  = sqrtf(hw2);
        p.setPen(QColor(0, 90, 80, 18));
        p.drawLine((int)(ecx-hw), (int)ey, (int)(ecx+hw), (int)ey);
    }

    // Continent streaks
    {
        unsigned cseed = 0xDEAD1234;
        for (int s = 0; s < 14; s++) {
            float bx  = ecx - earthR*0.7f + lcg(cseed)*earthR*1.4f;
            float by2 = ecy - earthR*0.9f + lcg(cseed)*earthR*1.6f;
            float len = earthR*(0.08f + lcg(cseed)*0.22f);
            float amp = 1.5f + lcg(cseed)*3.f;
            float frq = 6.f  + lcg(cseed)*12.f;
            float ph  = lcg(cseed)*float(M_PI)*2.f + satAngle*0.08f;
            float br  = 0.4f + lcg(cseed)*0.6f;
            float ddx = bx-ecx, ddy = by2-ecy;
            if (ddx*ddx + ddy*ddy > (float)earthR*(float)earthR*0.96f) continue;
            QPainterPath streak;
            bool started = false;
            for (int si = 0; si <= 20; si++) {
                float t  = (float)si/20;
                float px = bx + t*len;
                float py = by2 + sinf(t*frq + ph + time*0.05f)*amp;
                float dx2 = px-ecx, dy2 = py-ecy;
                if (dx2*dx2+dy2*dy2 > (float)earthR*(float)earthR*0.97f) continue;
                if (!started) { streak.moveTo(px,py); started=true; }
                else           streak.lineTo(px,py);
            }
            p.setPen(QPen(QColor(30,140,90,(int)(br*55)), 1.2f));
            p.setBrush(Qt::NoBrush);
            p.drawPath(streak);
        }
    }

    // Cloud wisps
    {
        unsigned clseed = 0xABC12345 ^ (unsigned)(time*7.f);
        for (int ci = 0; ci < 10; ci++) {
            float cx2 = ecx - earthR*0.85f + lcg(clseed)*earthR*1.7f;
            float cy2 = ecy - earthR*0.85f + lcg(clseed)*earthR*1.7f;
            float cw  = earthR*(0.04f + lcg(clseed)*0.12f);
            float ch  = cw*(0.25f + lcg(clseed)*0.35f);
            float dx2 = cx2-ecx, dy2 = cy2-ecy;
            if (dx2*dx2+dy2*dy2 > (float)earthR*(float)earthR*0.93f) continue;
            int alpha = (int)(lcg(clseed)*55)+15;
            p.setBrush(QColor(200,215,220,alpha));
            p.setPen(Qt::NoPen);
            p.drawEllipse(QRectF(cx2,cy2,cw,ch));
        }
    }

    // City lights on night side
    {
        unsigned nseed = 0x11223344;
        for (int ci = 0; ci < 18; ci++) {
            float theta = lcg(nseed)*float(M_PI)*2.f;
            float rr    = lcg(nseed)*(float)earthR*0.92f;
            float px    = ecx + rr*cosf(theta);
            float py    = ecy + rr*sinf(theta);
            float tsvx  = (px-ecx)/(float)earthR;
            float tsvy  = (py-ecy)/(float)earthR;
            if (tsvx*sunVx + tsvy*(-sunVy) > -0.1f) continue;
            float dx2=px-ecx, dy2=py-ecy;
            if (dx2*dx2+dy2*dy2 > (float)earthR*(float)earthR*0.92f) continue;
            float br2 = lcg(nseed);
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(255,(int)(200+br2*55),(int)(100+br2*60),(int)(br2*80)+20));
            p.drawEllipse(QPointF(px,py),1.0,1.0);
        }
    }

    // Terminator overlay
    {
        float tsvx = sunVx*(float)earthR, tsvy = -sunVy*(float)earthR;
        float tslen = sqrtf(tsvx*tsvx+tsvy*tsvy);
        if (tslen > 0.f) { tsvx /= tslen; tsvy /= tslen; }
        QLinearGradient tg(ecx-tsvx*earthR, ecy-tsvy*earthR,
                           ecx+tsvx*earthR, ecy+tsvy*earthR);
        tg.setColorAt(0.0f,  QColor(0,0,0,140));
        tg.setColorAt(0.38f, QColor(0,0,0,80));
        tg.setColorAt(0.48f, QColor(0,0,0,0));
        tg.setColorAt(1.0f,  QColor(0,0,0,0));
        p.setBrush(tg);
        p.setPen(Qt::NoPen);
        p.drawEllipse(QRectF(ecx-earthR, ecy-earthR, earthR*2, earthR*2));
    }

    // Atmosphere glow
    {
        QRadialGradient ag(ecx, ecy, earthR+14);
        ag.setColorAt(0.f,                           QColor(0,0,0,0));
        ag.setColorAt((float)(earthR-6)/(earthR+14), QColor(0,0,0,0));
        ag.setColorAt((float)(earthR+1)/(earthR+14), QColor(20,160,220,80));
        ag.setColorAt((float)(earthR+7)/(earthR+14), QColor(10,100,180,45));
        ag.setColorAt(1.f,                           QColor(0,0,0,0));
        p.setBrush(ag);
        p.setPen(Qt::NoPen);
        p.drawEllipse(QRectF(ecx-earthR-14,ecy-earthR-14,(earthR+14)*2,(earthR+14)*2));
    }

    p.restore();
}

// ── Satellite body ────────────────────────────────────────────────────────────
static void drawSatBody(QPainter &p, const QRect &r,
                        float satAngle, float time, const QColor &accent)
{
    p.setRenderHint(QPainter::Antialiasing, true);
    p.save();
    int bx = r.x() + r.width() / 2 + (int)(sinf(satAngle*0.5f + time*0.3f) * 4.f);
    int by = r.y() + 32;
    p.translate(bx, by);
    p.rotate(sinf(time*0.4f) * 5.f);

    // Body box
    p.setBrush(QColor(30,38,44));
    p.setPen(QPen(QColor(accent.red(),accent.green(),accent.blue(),200), 1));
    p.drawRect(-9,-5,18,10);

    // Solar panels
    p.setBrush(QColor(12,30,55));
    p.setPen(QPen(QColor(0,120,200,160), 1));
    p.drawRect(-39,-3,28,7);   // left
    p.drawRect( 11,-3,28,7);   // right
    // grid lines
    p.setPen(QColor(0,80,160,100));
    for (int g=1;g<4;g++) { p.drawLine(-39+g*7,-3,-39+g*7,3); p.drawLine(11+g*7,-3,11+g*7,3); }

    // glint
    float glint = (sinf(time*1.7f+satAngle)+1.f)*0.5f;
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(180,220,255,(int)(glint*130)));
    p.drawEllipse(QRectF(-38,-2,11,5));

    // Antenna
    p.setPen(QPen(QColor(accent.red(),accent.green(),accent.blue(),160), 1));
    p.drawLine(0,-5,0,-13);
    p.setBrush(Qt::NoBrush);
    p.drawEllipse(-3,-16,6,6);

    // Thrusters
    p.setPen(QPen(QColor(80,80,90,150), 1));
    p.drawLine(-3,5,-3,9);
    p.drawLine( 3,5, 3,9);

    p.restore();
}

// ── Sensor noise dots ─────────────────────────────────────────────────────────
static void drawNoise(QPainter &p, const QRect &r, float noisePhase) {
    unsigned seed = (unsigned)(noisePhase * 1000.f) ^ 0xFEEDFACE;
    p.setRenderHint(QPainter::Antialiasing, false);
    for (int i = 0; i < 60; i++) {
        float nx = lcg(seed), ny = lcg(seed), nb = lcg(seed);
        if (nb > 0.35f) continue;
        int x = r.x() + (int)(nx * r.width());
        int y = r.y() + (int)(ny * r.height());
        int a = (int)(nb * 180);
        p.setPen(QColor(0,200,170,a));
        p.drawPoint(x,y);
    }
    // Scanline flicker
    if ((int)(noisePhase * 60.f) % 90 < 3) {
        int sy = r.y() + (int)(lcg(seed) * r.height());
        p.setPen(QColor(0,215,190,18));
        p.drawLine(r.x(), sy, r.x()+r.width(), sy);
    }
}

// ── HUD overlay ───────────────────────────────────────────────────────────────
static void drawHUD(QPainter &p, const QRect &r, const SatelliteData &sat,
                    float time, const QColor &cyan, const QColor &orange,
                    const QColor &dimCyan, int selIdx)
{
    p.setRenderHint(QPainter::Antialiasing, true);
    QFont fSmall("Monospace", 6);  fSmall.setStyleHint(QFont::TypeWriter);
    QFont fMed  ("Monospace", 7);  fMed.setStyleHint(QFont::TypeWriter);
    QFont fBig  ("Monospace", 8);  fBig.setStyleHint(QFont::TypeWriter);

    int W = r.width(), H = r.height();
    int x0 = r.x(), y0 = r.y();

    // ── Corner brackets ───────────────────────────────────────────────────
    int bk = 10, bl = 5;
    auto bracket = [&](int bx, int by, int sx, int sy) {
        p.drawLine(bx, by, bx+sx*bl, by);
        p.drawLine(bx, by, bx, by+sy*bl);
    };
    p.setPen(QPen(cyan, 1));
    bracket(x0+bk,       y0+bk,        1,  1);
    bracket(x0+W-bk,     y0+bk,       -1,  1);
    bracket(x0+bk,       y0+H-bk,      1, -1);
    bracket(x0+W-bk,     y0+H-bk,     -1, -1);

    // ── Centre reticle ────────────────────────────────────────────────────
    int cx = x0+W/2, cy = y0+H/2;
    int rl = 6;
    p.setPen(QPen(QColor(cyan.red(),cyan.green(),cyan.blue(),130), 1));
    p.drawLine(cx-rl*2, cy, cx-4, cy);  p.drawLine(cx+4, cy, cx+rl*2, cy);
    p.drawLine(cx, cy-rl*2, cx, cy-4);  p.drawLine(cx, cy+4, cx, cy+rl*2);
    p.setBrush(Qt::NoBrush);
    p.drawEllipse(QRectF(cx-rl, cy-rl, rl*2, rl*2));

    // ── Horizon line ──────────────────────────────────────────────────────
    // Dashed at ~70% down
    int hY = y0 + (int)(H * 0.70f);
    p.setPen(QPen(QColor(0,215,190,50), 1, Qt::DashLine));
    p.drawLine(x0+20, hY, x0+W-20, hY);

    // ── Sat ID & status (top-left) ────────────────────────────────────────
    p.setFont(fMed);
    p.setPen(cyan);
    p.drawText(x0+14, y0+14, QString("SAT-%1").arg(selIdx+1));
    p.setFont(fSmall);
    p.setPen(QColor(cyan.red(),cyan.green(),cyan.blue(),160));
    p.drawText(x0+14, y0+23, sat.name.left(10));

    // ── Telemetry readouts (bottom) ───────────────────────────────────────
    // Use first few generic readings as proxies for telemetry display
    float alt   = sat.readings.size() > 0 ? sat.readings[0].value * 10.f + 380.f : 400.f;
    float spd   = sat.readings.size() > 1 ? sat.readings[1].value * 100.f        : 7800.f;
    float lat   = sat.readings.size() > 2 ? (sat.readings[2].value - 1.5f) * 45.f : 0.f;
    float lon   = sat.readings.size() > 3 ? (sat.readings[3].value - 50.f) * 3.6f : 0.f;
    float sig   = sat.readings.size() > 4 ? qBound(0.f, sat.readings[4].value / 100.f, 1.f) : 0.75f;

    int   bx2 = x0+8, by2 = y0+H-38;
    p.setFont(fSmall);

    auto hud_kv = [&](int bx, int by, const QString &key, const QString &val,
                       const QColor &vc) {
        p.setPen(dimCyan);
        p.drawText(bx, by, key);
        p.setPen(vc);
        p.drawText(bx+38, by, val);
    };

    hud_kv(bx2,    by2,    "ALT",  QString("%1km").arg((int)alt),   cyan);
    hud_kv(bx2,    by2+10, "SPD",  QString("%1ms").arg((int)spd),   cyan);
    hud_kv(bx2+80, by2,    "LAT",  QString("%1°").arg(lat,0,'f',1), orange);
    hud_kv(bx2+80, by2+10, "LON",  QString("%1°").arg(lon,0,'f',1), orange);
    hud_kv(bx2,    by2+20, "SIG",  QString("%1%").arg((int)(sig*100)), sig > 0.5f ? cyan : QColor(215,38,38));

    // ── Orbit arc indicator (right side mini) ─────────────────────────────
    {
        int ox = x0+W-24, oy = y0+H/2;
        int oR = 18;
        p.setBrush(Qt::NoBrush);
        p.setPen(QPen(QColor(0,215,190,40), 1));
        p.drawEllipse(QPointF(ox,oy), oR, oR);
        // satellite dot position
        float satFrac = fmodf(time*0.04f, 1.f);
        float sTheta  = satFrac * float(M_PI) * 2.f;
        int   sdx = (int)(cosf(sTheta)*oR);
        int   sdy = (int)(sinf(sTheta)*oR);
        p.setPen(Qt::NoPen);
        p.setBrush(cyan);
        p.drawEllipse(QPointF(ox+sdx, oy+sdy), 2, 2);
        // Earth dot
        p.setBrush(QColor(0,120,180,200));
        p.drawEllipse(QPointF(ox, oy), 4, 4);
    }

    // ── Blinking REC indicator (top-right) ───────────────────────────────
    if ((int)(time * 2.f) % 2 == 0) {
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(215,38,38,220));
        p.drawEllipse(QPointF(x0+W-16, y0+14), 4, 4);
        p.setFont(fSmall);
        p.setPen(QColor(215,38,38,220));
        p.drawText(x0+W-30, y0+18, "REC");
    }

    // ── Signal bar (right of status) ─────────────────────────────────────
    {
        int sbarX = x0+W-18, sbarY = y0+30, sbarH = H/3;
        p.setPen(QPen(QColor(0,215,190,40), 1));
        p.drawRect(sbarX, sbarY, 6, sbarH);
        int filled = (int)(sig * sbarH);
        QColor sigC = sig > 0.5f ? QColor(0,215,190,180) : QColor(215,38,38,180);
        p.fillRect(sbarX+1, sbarY + sbarH - filled, 4, filled, sigC);
        p.setFont(fSmall);
        p.setPen(QColor(0,215,190,120));
        p.drawText(sbarX-2, sbarY+sbarH+10, "SIG");
    }

    // ── Frame border ──────────────────────────────────────────────────────
    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(QColor(0,215,190,55), 1));
    p.drawRect(r.adjusted(1,1,-2,-2));
}

// ── Tab bar hit-testing ───────────────────────────────────────────────────────
void SatCameraWidget::mousePressEvent(QMouseEvent *e) {
    if (e->button() != Qt::LeftButton) return;
    const int M     = 8;
    const int TAB_H = 14;
    const int n     = m_model->satellites.size();
    if (n == 0) return;
    int tabW  = (width() - 2 * M) / n;
    int tabY0 = M + 14;
    int tabY1 = tabY0 + TAB_H + 2;
    if (e->pos().y() < tabY0 || e->pos().y() > tabY1) return;
    int col = (e->pos().x() - M) / tabW;
    col = qMax(0, qMin(n - 1, col));
    m_selectedIdx = col;
    update();
}

// ── paintEvent ────────────────────────────────────────────────────────────────
void SatCameraWidget::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const int W = width(), H = height();
    const int M = 8;

    // ── Black background ──────────────────────────────────────────────────
    p.fillRect(0, 0, W, H, kBg);

    // ── Satellite data ────────────────────────────────────────────────────
    const int n = m_model->satellites.size();
    if (n == 0) {
        p.setPen(kDimCyan);
        p.drawText(rect(), Qt::AlignCenter, "NO SATELLITES");
        return;
    }
    m_selectedIdx = qMax(0, qMin(n - 1, m_selectedIdx));
    const SatelliteData &sat = m_model->satellites[m_selectedIdx];

    // ── Derived orbital parameters ────────────────────────────────────────
    // altitude in km → orbitRadius = (6371 + alt) / 6371
    // Use the satellite's own orbitRadius directly from the model
    float alt        = sat.readings.size() > 0 ? sat.readings[0].value * 10.f + 380.f : 400.f;
    float orbitRadius = sat.orbitRadius;
    orbitRadius = qMax(1.05f, qMin(2.5f, orbitRadius));

    float satAngle   = sat.satAngle + m_time * 0.12f;
    float sunAngle   = m_time * 0.8f + 30.f;   // slow sun drift

    // ── Camera viewport (full widget minus tab area) ──────────────────────
    const int TAB_H  = 14;
    const int HEADER = M + TAB_H + 4;
    QRect camR(0, HEADER, W, H - HEADER);

    // 1. Deep-space gradient
    {
        QLinearGradient bg(0, camR.y(), 0, camR.y() + camR.height());
        bg.setColorAt(0.0f, QColor(2,  8, 12));
        bg.setColorAt(0.5f, QColor(3, 11, 17));
        bg.setColorAt(1.0f, QColor(4, 18, 26));
        p.fillRect(camR, bg);
    }

    // 2. Stars
    drawStarField(p, camR, satAngle, m_time);

    // 3. Earth limb
    drawEarthLimb(p, camR, satAngle, orbitRadius, sunAngle, m_time);

    // 4. Satellite body (foreground)
    drawSatBody(p, camR, satAngle, m_time, kCyan);

    // 5. Sensor noise
    drawNoise(p, camR, m_noisePhase);

    // 6. HUD overlay
    drawHUD(p, camR, sat, m_time, kCyan, kOrange, kDimCyan, m_selectedIdx);

    // ── Tab bar ───────────────────────────────────────────────────────────
    {
        QFont fTab("Monospace", 6);
        fTab.setStyleHint(QFont::TypeWriter);
        p.setFont(fTab);

        int tabW = (W - 2*M) / n;
        int tabY = M;

        for (int i = 0; i < n; i++) {
            int tx = M + i * tabW;
            bool sel = (i == m_selectedIdx);

            if (sel) {
                p.fillRect(tx, tabY, tabW-2, TAB_H, QColor(0,60,55));
                p.setPen(QPen(kCyan, 1));
                p.drawRect(tx, tabY, tabW-2, TAB_H);
            } else {
                p.fillRect(tx, tabY, tabW-2, TAB_H, QColor(0,22,20));
                p.setPen(QPen(kDimCyan, 1));
                p.drawRect(tx, tabY, tabW-2, TAB_H);
            }

            p.setPen(sel ? kCyan : kDimCyan);
            QString label = QString("S%1").arg(i+1);
            if (i < n) {
                const QString &nm = m_model->satellites[i].name;
                label = nm.length() > 6 ? nm.left(5)+"…" : nm;
            }
            QRect tr(tx+1, tabY+1, tabW-4, TAB_H-2);
            p.drawText(tr, Qt::AlignCenter, label);
        }

        // Header label
        p.setFont(fTab);
        p.setPen(QColor(kCyan.red(), kCyan.green(), kCyan.blue(), 90));
        p.drawText(W - M - 40, tabY + TAB_H - 2, "CAM-FEED");
    }
}
