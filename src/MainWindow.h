#pragma once
#include <QWidget>
#include <QTimer>
#include <QPoint>
#include <QMouseEvent>
#include <QDialog>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QFile>
#include "SatelliteModel.h"
#include "GlobeWidget.h"
#include "GeoSyncPanel.h"
#include "SignalGraphWidget.h"
#include "RadarWidget.h"
#include "CrtOverlay.h"

class TickerPanel : public QWidget {
    Q_OBJECT
public:
    explicit TickerPanel(SatelliteModel *model, QWidget *parent = nullptr);
    void refresh();
protected:
    void paintEvent(QPaintEvent *) override;
private:
    SatelliteModel *m_model;
};

class TitleBar : public QWidget {
    Q_OBJECT
public:
    explicit TitleBar(QWidget *parent = nullptr);
signals:
    void closeRequested();
    void helpRequested();
protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
private:
    bool   m_dragging = false;
    QPoint m_dragPos;
};

class MainWindow : public QWidget {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override = default;

protected:
    void resizeEvent(QResizeEvent *e) override;

private slots:
    void onTick();
    void showHelp();

private:
    SatelliteModel    *m_model;
    GlobeWidget       *m_globe;
    GeoSyncPanel      *m_geoPanel;
    SignalGraphWidget  *m_signalGraph;
    RadarWidget      *m_radar;
    TickerPanel       *m_ticker;
    TitleBar          *m_titleBar;
    CrtOverlay        *m_crt;
    QTimer            *m_timer;
};
