#pragma once
#include <QWidget>
#include <QPoint>
#include <QMouseEvent>
#include <QEvent>
#include "SatelliteModel.h"

class FloatingRadar : public QWidget {
    Q_OBJECT
public:
    explicit FloatingRadar(SatelliteModel *model, QWidget *parent = nullptr);
    void refresh();

protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *) override;
    void resizeEvent(QResizeEvent *e) override;
    void moveBy(const QPoint &delta);

private:
    SatelliteModel *m_model;
    bool   m_dragging = false;
    QPoint m_dragPos;
    int    m_size = 160;
};
