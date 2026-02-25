#pragma once
#include <QWidget>

class CrtOverlay : public QWidget {
    Q_OBJECT
public:
    explicit CrtOverlay(QWidget *parent = nullptr);
    void setFlash(float v) { m_flash = v; update(); }

protected:
    void paintEvent(QPaintEvent *) override;

private:
    float m_flash = 0.f;
};
