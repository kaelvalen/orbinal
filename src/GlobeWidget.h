#pragma once
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QTimer>
#include <QVector3D>
#include <QMatrix4x4>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include "SatelliteModel.h"

struct SatTrail {
    QVector<QVector3D> pts;
    static constexpr int MaxLen = 80;
};

class GlobeWidget : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT
public:
    explicit GlobeWidget(SatelliteModel *model, QWidget *parent = nullptr);
    ~GlobeWidget() override;

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

private:
    void buildGridLines();
    void buildContinentLines();
    void buildSphereMesh(int stacks, int slices);
    void buildOrbitLine(float inclination, float radius, int segments, QVector<QVector3D> &out);
    void drawLinesGradient(const QVector<QVector3D> &pts, const QVector3D &color, float alphaFront, float alphaBack);
    void drawLines(const QVector<QVector3D> &pts, const QVector3D &color, float alpha, bool loop = false);
    void drawSatellite(const QVector3D &pos, const QVector3D &forward, const QVector3D &color);
    void drawSolidSphere(const QMatrix4x4 &mvp, const QMatrix4x4 &modelMat);
    void drawAtmosphere(QPainter &p);
    void drawSatTrail(int satIdx, QPainter &p, const QMatrix4x4 &mvp);
    void drawPingDot(QPainter &p, const QVector3D &worldPos, const QMatrix4x4 &mvp, const QColor &color, float pingPhase);
    void drawPoleAxis(QPainter &p, const QMatrix4x4 &mvp);
    void drawSatLabels(QPainter &p, const QMatrix4x4 &mvp);
    void drawGroundTrack(int satIdx, QPainter &p, const QMatrix4x4 &mvp);
    void drawEquatorTicks(QPainter &p, const QMatrix4x4 &mvp);
    QVector3D orbitPosition(float inclination, float phase, float radius);
    QPointF projectToScreen(const QVector3D &world, const QMatrix4x4 &mvp);
    bool isFacingCamera(const QVector3D &worldPt, const QMatrix4x4 &mvp);

    SatelliteModel       *m_model;

    // Line shader
    QOpenGLShaderProgram *m_prog       = nullptr;
    int                   m_uMVP       = -1;
    int                   m_uColor     = -1;
    int                   m_uDepthFade = -1;

    // Solid sphere shader
    QOpenGLShaderProgram *m_sphereProg   = nullptr;
    int                   m_spUMVP       = -1;
    int                   m_spUModel     = -1;
    int                   m_spUSunDir    = -1;
    QVector<float>        m_sphereVerts; // interleaved pos(3)+normal(3)
    QVector<unsigned int> m_sphereIdx;

    QVector<QVector3D>    m_gridLines;
    QVector<QVector3D>    m_continentLines;
    QMatrix4x4            m_proj;
    float                 m_rotY      = 0.f;
    float                 m_pingPhase = 0.f;
    float                 m_sunAngle  = 0.f;
    QVector<SatTrail>     m_trails;
    QVector<QVector<QVector3D>> m_groundTracks; // per-sat
    QMatrix4x4            m_lastMVP;
};
