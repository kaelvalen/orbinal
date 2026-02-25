#pragma once
#include <QWidget>
#include "SatelliteModel.h"

class SignalGraphWidget : public QWidget {
    Q_OBJECT
public:
    explicit SignalGraphWidget(SatelliteModel *model, QWidget *parent = nullptr);

    void refresh();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    SatelliteModel *m_model;
};
