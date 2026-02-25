#include "SignalGraphWidget.h"
#include <QPainter>
#include <QPen>
#include <QFont>
#include <QFontMetrics>
#include <QLinearGradient>
#include <QRadialGradient>
#include <cmath>
#include <algorithm>

namespace {
    const QColor kBg       = QColor(2, 10, 14);
    const QColor kCyan     = QColor(0, 215, 190);
    const QColor kOrange   = QColor(225, 115, 15);
    const QColor kRed      = QColor(215, 45, 45);
    const QColor kDimCyan  = QColor(0, 75, 65);
    const QColor kGridLine = QColor(0, 55, 48);
    const QColor kWhite    = QColor(190, 210, 205);
}

SignalGraphWidget::SignalGraphWidget(SatelliteModel *model, QWidget *parent)
    : QWidget(parent), m_model(model)
{
    setMinimumHeight(140);
    setMaximumHeight(190);
    setAttribute(Qt::WA_OpaquePaintEvent);
}

void SignalGraphWidget::refresh() {
    update();
}

// Simple DFT magnitude for spectrum view
static QVector<float> computeSpectrum(const QVector<float> &sig, int bins) {
    int n = sig.size();
    QVector<float> out(bins, 0.f);
    float inv = 1.f / n;
    for (int k = 0; k < bins; k++) {
        float re = 0.f, im = 0.f;
        float w = 2.f * float(M_PI) * k * inv;
        for (int t = 0; t < n; t++) {
            re += sig[t] * cosf(w * t);
            im += sig[t] * sinf(w * t);
        }
        out[k] = sqrtf(re*re + im*im) * inv;
    }
    return out;
}

void SignalGraphWidget::paintEvent(QPaintEvent *) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false);
    p.fillRect(rect(), kBg);

    const int M      = 8;
    const int lblW   = 28;
    const int W      = width();
    const int H      = height();
    int gx = M + lblW;
    int gw = W - gx - M;

    // Split vertically: top=waveform, bottom=spectrum
    int midH   = H / 2 - 2;
    int waveH  = midH - 2;
    int specH  = H - midH - M - 12;  // 12 for freq labels
    int waveY  = M;
    int specY  = midH + 4;

    // ── Section labels ─────────────────────────────────────────────────
    QFont hdrFont("Courier", 6);
    hdrFont.setBold(true);
    p.setFont(hdrFont);
    p.setPen(QColor(0, 120, 105));
    p.drawText(M, waveY + 8, "WAVE");
    p.drawText(M, specY + 8, "SPEC");

    // ── WAVEFORM PANEL ─────────────────────────────────────────────────
    p.setPen(QPen(kDimCyan, 1));
    p.drawRect(gx, waveY, gw, waveH);

    int midY = waveY + waveH / 2;
    // Grid lines
    for (int i = 1; i <= 3; i++) {
        p.setPen(QPen(kGridLine, 1, Qt::DotLine));
        p.drawLine(gx+1, waveY + waveH*i/8, gx+gw-1, waveY + waveH*i/8);
        p.drawLine(gx+1, waveY + waveH - waveH*i/8, gx+gw-1, waveY + waveH - waveH*i/8);
    }
    p.setPen(QPen(kDimCyan, 1, Qt::DashLine));
    p.drawLine(gx+1, midY, gx+gw-1, midY);
    for (int i = 1; i < 8; i++) {
        p.setPen(QPen(kGridLine, 1, Qt::DotLine));
        p.drawLine(gx + gw*i/8, waveY+1, gx + gw*i/8, waveY+waveH-1);
    }

    // dB labels
    QFont lblFont("Courier", 6);
    QFontMetrics lfm(lblFont);
    p.setFont(lblFont);
    p.setPen(kDimCyan);
    p.drawText(M + 2, waveY + lfm.ascent(),     "+6");
    p.drawText(M + 2, midY   + lfm.ascent()/2,  " 0");
    p.drawText(M + 2, waveY + waveH - 2,        "-6");

    const auto &histA = m_model->signalHistory;
    const auto &histB = m_model->signalHistoryB;
    const auto &histC = m_model->signalHistoryC;
    int n = std::min({histA.size(), histB.size(), histC.size()});
    if (n < 2) return;

    float scale = waveH * 0.44f;
    auto toWave = [&](int i, float v) -> QPointF {
        return QPointF(gx + 1.f + (float)i/(n-1)*(gw-2), midY - v*scale);
    };

    struct Channel { const QVector<float> *data; QColor color; };
    QVector<Channel> chs = { {&histA, kCyan}, {&histB, kOrange}, {&histC, kRed} };

    p.setRenderHint(QPainter::Antialiasing, true);
    p.setClipRect(gx+1, waveY+1, gw-2, waveH-2);

    for (const auto &ch : chs) {
        p.setPen(QPen(ch.color, 1.2f));
        for (int i = 1; i < n; i++)
            p.drawLine(toWave(i-1, (*ch.data)[i-1]), toWave(i, (*ch.data)[i]));
    }
    p.setClipping(false);

    // Peak hold lines
    for (const auto &ch : chs) {
        float peak = *std::max_element(ch.data->begin(), ch.data->end());
        float peakY = midY - peak * scale;
        QColor pc = ch.color; pc.setAlpha(100);
        p.setPen(QPen(pc, 1, Qt::DotLine));
        p.drawLine(gx+gw-20, (int)peakY, gx+gw-2, (int)peakY);
    }

    // Legend
    p.setRenderHint(QPainter::Antialiasing, false);
    struct Leg { QString tag; QColor col; };
    QVector<Leg> legend = {{"CH-A",kCyan},{"CH-B",kOrange},{"CH-C",kRed}};
    QFont legFont("Courier", 6);
    QFontMetrics legFm(legFont);
    p.setFont(legFont);
    int lx = gx + gw - 4;
    for (int li = legend.size()-1; li >= 0; li--) {
        int tw = legFm.horizontalAdvance(legend[li].tag);
        lx -= tw + 14;
        p.setPen(legend[li].col);
        p.drawText(lx+10, waveY + waveH - 3, legend[li].tag);
        p.setPen(QPen(legend[li].col, 2));
        p.drawLine(lx+2, waveY+waveH-3-legFm.ascent()/2, lx+9, waveY+waveH-3-legFm.ascent()/2);
    }

    // ── SPECTRUM PANEL ─────────────────────────────────────────────────
    p.setPen(QPen(kDimCyan, 1));
    p.drawRect(gx, specY, gw, specH);

    const int fftBins = 32;
    QVector<float> specA = computeSpectrum(histA, fftBins);
    QVector<float> specB = computeSpectrum(histB, fftBins);
    QVector<float> specC = computeSpectrum(histC, fftBins);

    // Normalize
    float maxS = 0.001f;
    for (int i = 1; i < fftBins; i++) maxS = qMax(maxS, std::max({specA[i], specB[i], specC[i]}));
    float normFactor = 1.f / maxS;

    int usableBins = fftBins - 1;
    float barW = (float)(gw - 2) / (usableBins * 3 + usableBins + 1);
    float groupW = barW * 3 + barW; // 3 bars + 1 gap

    QVector<const QVector<float>*> specs = {&specA, &specB, &specC};
    QVector<QColor> specCols = {kCyan, kOrange, kRed};

    for (int bi = 1; bi < fftBins; bi++) {
        float gStart = gx + 1 + (bi-1) * groupW;
        for (int ci = 0; ci < 3; ci++) {
            float mag   = (*specs[ci])[bi] * normFactor;
            int   barH  = (int)(mag * (specH - 2));
            float bx    = gStart + ci * barW;

            // Gradient fill bar
            QLinearGradient barGrad(bx, specY + specH - barH, bx, specY + specH);
            QColor topCol = specCols[ci]; topCol.setAlpha(200);
            QColor botCol = specCols[ci]; botCol.setAlpha(60);
            barGrad.setColorAt(0.f, topCol);
            barGrad.setColorAt(1.f, botCol);
            p.setBrush(barGrad);
            p.setPen(Qt::NoPen);
            p.drawRect(QRectF(bx, specY + specH - barH - 1, barW - 0.5f, barH));

            // Peak dot
            p.setBrush(specCols[ci]);
            p.drawRect(QRectF(bx, specY + specH - barH - 3, barW - 0.5f, 2));
        }
    }
    p.setBrush(Qt::NoBrush);

    // Frequency axis labels
    p.setFont(lblFont);
    p.setPen(kDimCyan);
    const int freqLabels = 5;
    for (int i = 0; i <= freqLabels; i++) {
        int fx = gx + gw * i / freqLabels;
        float hz = 30.f + 90.f * i / freqLabels;
        QString hzStr = QString("%1").arg((int)hz);
        p.drawText(fx - lfm.horizontalAdvance(hzStr)/2,
                   specY + specH + 11, hzStr);
        p.setPen(QPen(kGridLine, 1, Qt::DotLine));
        p.drawLine(fx, specY+1, fx, specY+specH-1);
        p.setPen(kDimCyan);
    }
    p.drawText(gx + gw - lfm.horizontalAdvance("Hz"), specY + specH + 11, "Hz");

    // Scanlines
    for (int sy = M; sy < H - M; sy += 3) {
        p.setPen(QColor(0,0,0,18));
        p.drawLine(0, sy, W, sy);
    }

    // Corner brackets
    p.setPen(QPen(kDimCyan, 1));
    int cm = 5, bl = 10;
    p.drawLine(cm,   cm,   cm+bl, cm);    p.drawLine(cm,   cm,   cm,   cm+bl);
    p.drawLine(W-cm, cm,   W-cm-bl, cm);  p.drawLine(W-cm, cm,   W-cm, cm+bl);
    p.drawLine(cm,   H-cm, cm+bl, H-cm);  p.drawLine(cm,   H-cm, cm,   H-cm-bl);
    p.drawLine(W-cm, H-cm, W-cm-bl, H-cm);p.drawLine(W-cm, H-cm, W-cm, H-cm-bl);

    // Signal strength meter (right side)
    float rms = 0.f;
    for (int i = 0; i < n; i++) rms += histA[i]*histA[i];
    rms = sqrtf(rms / n);
    int meterH = waveH + 4;
    int meterX = W - M - 6;
    int meterY = waveY;
    int filled  = (int)(qMin(1.f, rms * 18.f) * meterH);
    p.setPen(Qt::NoPen);
    for (int seg = 0; seg < meterH; seg += 3) {
        bool lit = (meterH - seg) <= filled;
        QColor sc = (seg < meterH*0.6f) ? QColor(0,200,170,lit?200:30)
                  : (seg < meterH*0.85f)? QColor(200,150,0,lit?200:30)
                                        : QColor(200,40,40,lit?220:30);
        p.setBrush(sc);
        p.drawRect(meterX, meterY + seg, 4, 2);
    }
}
