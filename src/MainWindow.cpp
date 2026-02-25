#include "MainWindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QPainter>
#include <QPen>
#include <QFont>
#include <QFontMetrics>
#include <QApplication>
#include <QScreen>
#include <QResizeEvent>
#include <cmath>

// ── TickerPanel ──────────────────────────────────────────────────────────────

TickerPanel::TickerPanel(SatelliteModel *model, QWidget *parent)
    : QWidget(parent), m_model(model)
{
    setMinimumHeight(90);
    setMaximumHeight(110);
    setAttribute(Qt::WA_OpaquePaintEvent);
}

void TickerPanel::refresh() { update(); }

void TickerPanel::paintEvent(QPaintEvent *) {
    static const QColor kBg      = QColor(2, 10, 14);
    static const QColor kCyan    = QColor(0, 215, 190);
    static const QColor kOrange  = QColor(225, 115, 15);
    static const QColor kRed     = QColor(215, 38, 38);
    static const QColor kDimCyan = QColor(0, 80, 70);
    static const QColor kBorder  = QColor(0, 60, 52);

    QPainter p(this);
    p.fillRect(rect(), kBg);

    int m = 8;
    p.setPen(QPen(kBorder, 1));
    p.drawRect(m, m, width() - 2*m, height() - 2*m);

    QFont hf("Courier", 7);
    hf.setBold(true);
    p.setFont(hf);
    p.setPen(kDimCyan);
    p.drawText(m + 4, m + 10, "EVENT LOG");

    p.setPen(QPen(kDimCyan, 1));
    p.drawLine(m + 4, m + 13, width() - m - 4, m + 13);

    QFont df("Courier", 7);
    QFontMetrics fm(df);
    p.setFont(df);
    int lh  = fm.height() + 2;
    int maxLines = (height() - 2*m - 18) / lh;
    const auto &log = m_model->tickerLog;

    for (int i = 0; i < qMin(maxLines, (int)log.size()); i++) {
        const auto &msg = log[i];
        int ry = m + 18 + i * lh;
        float fade = 1.f - (float)i / qMax(1, maxLines);
        QColor col;
        if      (msg.level == AlertLevel::Critical) col = kRed;
        else if (msg.level == AlertLevel::Warning)  col = kOrange;
        else                                        col = kCyan;
        col.setAlphaF(fade * 0.9f + 0.1f);
        p.setPen(col);
        if (i == 0) {
            QString prefix = QString::fromUtf8("\xE2\x96\xB6 ");
            p.drawText(m + 6, ry + fm.ascent(), prefix + msg.text);
        } else {
            p.drawText(m + 6, ry + fm.ascent(), "  " + msg.text);
        }
    }

    // scanlines
    for (int y = 0; y < height(); y += 3) {
        p.setPen(QColor(0, 0, 0, 20));
        p.drawLine(0, y, width(), y);
    }

    // corner brackets
    p.setPen(QPen(kDimCyan, 1));
    int bl = 10;
    p.drawLine(m, m, m+bl, m);                p.drawLine(m, m, m, m+bl);
    p.drawLine(width()-m, m, width()-m-bl, m); p.drawLine(width()-m, m, width()-m, m+bl);
    p.drawLine(m, height()-m, m+bl, height()-m); p.drawLine(m, height()-m, m, height()-m-bl);
    p.drawLine(width()-m, height()-m, width()-m-bl, height()-m); p.drawLine(width()-m, height()-m, width()-m, height()-m-bl);
}

// ── TitleBar ─────────────────────────────────────────────────────────────────

TitleBar::TitleBar(QWidget *parent) : QWidget(parent) {
    setFixedHeight(32);
    setAttribute(Qt::WA_OpaquePaintEvent);
}

void TitleBar::paintEvent(QPaintEvent *) {
    static const QColor kBg     = QColor(0, 6, 10);
    static const QColor kCyan   = QColor(0, 215, 190);
    static const QColor kDim    = QColor(0, 70, 62);
    static const QColor kOrange = QColor(225, 115, 15);

    QPainter p(this);
    p.fillRect(rect(), kBg);

    p.setPen(QPen(kDim, 1));
    p.drawLine(0, height()-1, width(), height()-1);

    QFont f("Courier", 9);
    f.setBold(true);
    p.setFont(f);

    // Left: version tag
    p.setPen(kDim);
    p.drawText(10, 21, "v2.4.1");

    // Left tick marks
    for (int x = 52; x < width()/2 - 130; x += 16) {
        int h2 = (x % 48 == 0) ? 8 : 4;
        p.setPen(QPen(kDim, 1));
        p.drawLine(x, height() - h2 - 2, x, height() - 2);
    }

    // Centre title
    p.setPen(kCyan);
    QString title = "GLOBAL SAT POSITIONS  //  ORBINAL";
    QFontMetrics fm(f);
    int tw = fm.horizontalAdvance(title);
    p.drawText(width()/2 - tw/2, 21, title);

    // Right tick marks
    for (int x = width()/2 + 130; x < width() - 50; x += 16) {
        int h2 = (x % 48 == 0) ? 8 : 4;
        p.setPen(QPen(kDim, 1));
        p.drawLine(x, height() - h2 - 2, x, height() - 2);
    }

    // Help button
    int hx = width() - 60, hy = 7, hs = 18;
    p.setPen(QPen(kDim, 1));
    p.drawRect(hx, hy, hs, hs);
    p.setPen(QPen(kCyan, 1));
    QFont hf("Courier", 8);
    hf.setBold(true);
    p.setFont(hf);
    p.drawText(hx + 2, hy + 13, "?");

    // Close button area
    int bx = width() - 28, by = 7, bs = 18;
    p.setPen(QPen(kDim, 1));
    p.drawRect(bx, by, bs, bs);
    p.setPen(QPen(kOrange, 1));
    p.drawLine(bx+4, by+4, bx+bs-4, by+bs-4);
    p.drawLine(bx+bs-4, by+4, bx+4, by+bs-4);
}

void TitleBar::mousePressEvent(QMouseEvent *e) {
    if (e->button() == Qt::LeftButton) {
        int hx = width() - 60;
        int bx = width() - 28;
        if (e->pos().x() >= hx && e->pos().x() < hx + 18 && e->pos().y() >= 7 && e->pos().y() < 25) {
            emit helpRequested();
            return;
        }
        if (e->pos().x() >= bx) { emit closeRequested(); return; }
        m_dragging = true;
        m_dragPos  = e->globalPosition().toPoint() - window()->frameGeometry().topLeft();
    }
}
void TitleBar::mouseMoveEvent(QMouseEvent *e) {
    if (m_dragging) window()->move(e->globalPosition().toPoint() - m_dragPos);
}
void TitleBar::mouseReleaseEvent(QMouseEvent *) { m_dragging = false; }

// ── MainWindow ───────────────────────────────────────────────────────────────

MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::Window);
    setAttribute(Qt::WA_TranslucentBackground, false);
    setStyleSheet("background-color: #00060a;");

    m_model = new SatelliteModel();
    m_model->pushTicker("SYSTEM INIT - ALL NODES ACTIVE", AlertLevel::Normal);
    m_model->pushTicker("UPLINK ESTABLISHED: CBK-01", AlertLevel::Normal);
    m_model->pushTicker("ORBIT LOCK CONFIRMED", AlertLevel::Normal);

    m_titleBar   = new TitleBar(this);
    m_geoPanel   = new GeoSyncPanel(m_model, this);
    m_satCamera  = new SatCameraWidget(m_model, this);
    m_signalGraph= new SignalGraphWidget(m_model, this);
    m_ticker     = new TickerPanel(m_model, this);
    m_globe      = new GlobeWidget(m_model, this);
    m_crt        = new CrtOverlay(this);

    connect(m_titleBar, &TitleBar::closeRequested, this, &MainWindow::close);
    connect(m_titleBar, &TitleBar::helpRequested, this, &MainWindow::showHelp);

    // ── Main layout ──────────────────────────────────────────────────────────
    QVBoxLayout *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);
    root->addWidget(m_titleBar);

    QWidget *body = new QWidget(this);
    body->setStyleSheet("background-color: #00060a;");
    root->addWidget(body, 1);

    QHBoxLayout *hbox = new QHBoxLayout(body);
    hbox->setContentsMargins(6, 6, 6, 6);
    hbox->setSpacing(6);

    // Left panel: geo info + signal graph + ticker
    QWidget *leftPanel = new QWidget(body);
    leftPanel->setStyleSheet("background-color: #00060a;");
    QVBoxLayout *leftVbox = new QVBoxLayout(leftPanel);
    leftVbox->setContentsMargins(0, 0, 0, 0);
    leftVbox->setSpacing(4);
    leftVbox->addWidget(m_geoPanel,    1);
    leftVbox->addWidget(m_satCamera,   0);
    leftVbox->addWidget(m_signalGraph, 0);
    leftVbox->addWidget(m_ticker,      0);

    // Divider
    QFrame *div = new QFrame(body);
    div->setFrameShape(QFrame::VLine);
    div->setStyleSheet("background-color: #003830;");
    div->setFixedWidth(1);

    // Right panel: globe (top, stretched) + radar (bottom-right, fixed)
    QWidget *rightPanel = new QWidget(body);
    rightPanel->setStyleSheet("background-color: #00060a;");
    QVBoxLayout *rightVbox = new QVBoxLayout(rightPanel);
    rightVbox->setContentsMargins(0, 0, 0, 0);
    rightVbox->setSpacing(4);

    // Globe fills all available vertical space
    rightVbox->addWidget(m_globe, 1);

    // Bottom row: spacer + radar fixed-size
    QWidget *bottomRow = new QWidget(rightPanel);
    bottomRow->setStyleSheet("background-color: #00060a;");
    QHBoxLayout *bottomHbox = new QHBoxLayout(bottomRow);
    bottomHbox->setContentsMargins(0, 0, 0, 0);
    bottomHbox->setSpacing(0);
    m_radar = new RadarWidget(m_model, bottomRow);
    bottomHbox->addStretch(1);
    bottomHbox->addWidget(m_radar);
    rightVbox->addWidget(bottomRow, 0);

    hbox->addWidget(leftPanel,  5);
    hbox->addWidget(div);
    hbox->addWidget(rightPanel, 5);

    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &MainWindow::onTick);
    m_timer->start(16);

    resize(1160, 580);

    // CRT overlay on top of everything
    m_crt->raise();
    m_crt->resize(size());
}

void MainWindow::resizeEvent(QResizeEvent *e) {
    QWidget::resizeEvent(e);
    m_crt->resize(e->size());
}

void MainWindow::onTick() {
    m_model->update(0.016f);
    m_geoPanel->refresh();
    m_satCamera->refresh();
    m_signalGraph->refresh();
    m_ticker->refresh();
    m_radar->refresh();
    m_crt->setFlash(m_model->sirenFlash);
    m_crt->update();
    update();
}

void MainWindow::showHelp() {
    QDialog *dlg = new QDialog(this);
    dlg->setWindowTitle("Orbinal — Help");
    dlg->resize(640, 480);

    QVBoxLayout *lay = new QVBoxLayout(dlg);
    QTextEdit *txt = new QTextEdit(dlg);
    txt->setReadOnly(true);
    txt->setFont(QFont("Courier", 9));

    QFile file("README.md");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        txt->setPlainText(QString::fromUtf8(file.readAll()));
    } else {
        txt->setPlainText("README.md not found.\n\nOrbinal is a real‑time satellite telemetry HUD.\n\nFeatures:\n• 3D globe with GPU‑accelerated rendering\n• Live telemetry with real units\n• Signal waveform + FFT spectrum\n• Radar sweep with CUDA acceleration\n• CRT post‑process effects\n\nBuild:\n  cmake .. && cmake --build .\n\nRun:\n  ./orbinal");
    }

    lay->addWidget(txt);
    dlg->exec();
    dlg->deleteLater();
}
