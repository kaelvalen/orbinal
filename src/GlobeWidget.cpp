#include "GlobeWidget.h"
#include "ContinentData.h"
#include <QOpenGLFunctions>
#include <QPainter>
#include <QPainterPath>
#include <QRadialGradient>
#include <QConicalGradient>
#include <cmath>

// ── Line shader ──────────────────────────────────────────────────────────────
static const char *kVert = R"(
#version 120
attribute vec3 position;
uniform mat4 mvp;
varying float vDepth;
void main() {
    vec4 p = mvp * vec4(position, 1.0);
    gl_Position = p;
    vDepth = p.z / p.w;
}
)";

static const char *kFrag = R"(
#version 120
uniform vec4 uColor;
uniform float uDepthFade;
varying float vDepth;
void main() {
    float fade = uDepthFade > 0.5 ? mix(0.18, 1.0, clamp(1.0 - vDepth * 0.85, 0.0, 1.0)) : 1.0;
    gl_FragColor = vec4(uColor.rgb, uColor.a * fade);
}
)";

// ── Solid sphere shader ───────────────────────────────────────────────────────
static const char *kSphereVert = R"(
#version 120
attribute vec3 aPos;
attribute vec3 aNorm;
uniform mat4 uMVP;
uniform mat4 uModel;
varying vec3 vNorm;
varying vec3 vWorldPos;
void main() {
    vec4 world = uModel * vec4(aPos, 1.0);
    vWorldPos   = world.xyz;
    vNorm       = normalize(mat3(uModel) * aNorm);
    gl_Position = uMVP * vec4(aPos, 1.0);
}
)";

static const char *kSphereFrag = R"(
#version 120
uniform vec3 uSunDir;
varying vec3 vNorm;
varying vec3 vWorldPos;
void main() {
    vec3 n    = normalize(vNorm);
    float nDotL = dot(n, normalize(uSunDir));

    // Day side: deep ocean-teal with bright lit edges
    vec3 dayColor   = mix(vec3(0.00, 0.10, 0.14), vec3(0.00, 0.40, 0.38), max(0.0, nDotL));
    // Night side: very dark with slight blue tint
    vec3 nightColor = vec3(0.004, 0.012, 0.020);
    // Smooth terminator blend
    float blend = smoothstep(-0.12, 0.18, nDotL);
    vec3 color  = mix(nightColor, dayColor, blend);

    // Fresnel rim glow
    vec3 viewDir = normalize(vec3(0.0, 0.3, 3.2) - vWorldPos);
    float rim    = 1.0 - max(0.0, dot(n, viewDir));
    rim          = pow(rim, 3.5);
    color       += rim * 0.18 * vec3(0.0, 0.85, 0.75);

    // Subtle grid-like surface noise (fake land masses hint)
    float lat = asin(clamp(n.y, -1.0, 1.0));
    float lon = atan(n.z, n.x);
    float grid = abs(sin(lat * 6.0)) * 0.015 + abs(sin(lon * 5.0)) * 0.010;
    color += grid * blend * vec3(0.0, 0.6, 0.5);

    gl_FragColor = vec4(color, 1.0);
}
)";

static QVector3D spherePt(float lat, float lon) {
    float rlat = lat * M_PI / 180.f;
    float rlon = lon * M_PI / 180.f;
    return QVector3D(cosf(rlat)*cosf(rlon), sinf(rlat), cosf(rlat)*sinf(rlon));
}

GlobeWidget::GlobeWidget(SatelliteModel *model, QWidget *parent)
    : QOpenGLWidget(parent), m_model(model)
{
    setMinimumSize(420, 360);
    for (int i = 0; i < (int)model->satellites.size(); i++) {
        m_trails.push_back(SatTrail{});
        m_groundTracks.push_back(QVector<QVector3D>{});
    }
}

GlobeWidget::~GlobeWidget() {
    makeCurrent();
    delete m_prog;
    delete m_sphereProg;
    doneCurrent();
}

void GlobeWidget::initializeGL() {
    initializeOpenGLFunctions();
    glClearColor(0.008f, 0.012f, 0.018f, 1.f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);

    // Line shader
    m_prog = new QOpenGLShaderProgram(this);
    m_prog->addShaderFromSourceCode(QOpenGLShader::Vertex,   kVert);
    m_prog->addShaderFromSourceCode(QOpenGLShader::Fragment, kFrag);
    m_prog->link();
    m_uMVP       = m_prog->uniformLocation("mvp");
    m_uColor     = m_prog->uniformLocation("uColor");
    m_uDepthFade = m_prog->uniformLocation("uDepthFade");

    // Solid sphere shader
    m_sphereProg = new QOpenGLShaderProgram(this);
    m_sphereProg->addShaderFromSourceCode(QOpenGLShader::Vertex,   kSphereVert);
    m_sphereProg->addShaderFromSourceCode(QOpenGLShader::Fragment, kSphereFrag);
    m_sphereProg->link();
    m_spUMVP    = m_sphereProg->uniformLocation("uMVP");
    m_spUModel  = m_sphereProg->uniformLocation("uModel");
    m_spUSunDir = m_sphereProg->uniformLocation("uSunDir");

    buildGridLines();
    buildContinentLines();
    buildSphereMesh(48, 72);
}

void GlobeWidget::buildSphereMesh(int stacks, int slices) {
    m_sphereVerts.clear();
    m_sphereIdx.clear();

    for (int i = 0; i <= stacks; i++) {
        float phi = float(M_PI) * i / stacks - float(M_PI) / 2.f;
        for (int j = 0; j <= slices; j++) {
            float theta = 2.f * float(M_PI) * j / slices;
            float x = cosf(phi) * cosf(theta);
            float y = sinf(phi);
            float z = cosf(phi) * sinf(theta);
            m_sphereVerts << x << y << z;   // pos
            m_sphereVerts << x << y << z;   // normal = pos on unit sphere
        }
    }

    for (int i = 0; i < stacks; i++) {
        for (int j = 0; j < slices; j++) {
            unsigned int a = i * (slices + 1) + j;
            unsigned int b = a + slices + 1;
            m_sphereIdx << a << b << a+1;
            m_sphereIdx << b << b+1 << a+1;
        }
    }
}

void GlobeWidget::drawSolidSphere(const QMatrix4x4 &mvp, const QMatrix4x4 &modelMat) {
    m_sphereProg->bind();
    m_sphereProg->setUniformValue(m_spUMVP,   mvp);
    m_sphereProg->setUniformValue(m_spUModel, modelMat);

    // Sun direction rotates slowly — world space
    QVector3D sunDir(
        cosf(m_sunAngle * float(M_PI) / 180.f),
        0.25f,
        sinf(m_sunAngle * float(M_PI) / 180.f)
    );
    m_sphereProg->setUniformValue(m_spUSunDir, sunDir.normalized());

    int posLoc  = m_sphereProg->attributeLocation("aPos");
    int normLoc = m_sphereProg->attributeLocation("aNorm");

    const float *vData = m_sphereVerts.constData();
    int stride = 6 * sizeof(float);

    m_sphereProg->enableAttributeArray(posLoc);
    m_sphereProg->setAttributeArray(posLoc,  GL_FLOAT, vData,     3, stride);
    m_sphereProg->enableAttributeArray(normLoc);
    m_sphereProg->setAttributeArray(normLoc, GL_FLOAT, vData + 3, 3, stride);

    glDrawElements(GL_TRIANGLES, m_sphereIdx.size(), GL_UNSIGNED_INT, m_sphereIdx.constData());

    m_sphereProg->disableAttributeArray(posLoc);
    m_sphereProg->disableAttributeArray(normLoc);
    m_sphereProg->release();
}

void GlobeWidget::drawGroundTrack(int satIdx, QPainter &p, const QMatrix4x4 &mvp) {
    if (satIdx >= m_groundTracks.size()) return;
    const auto &track = m_groundTracks[satIdx];
    if (track.size() < 2) return;

    const auto &sat = m_model->satellites[satIdx];
    QColor col = sat.orbitColor;
    p.setRenderHint(QPainter::Antialiasing, true);

    int n = track.size();
    for (int i = 1; i < n; i++) {
        // Ground track: project surface point (radius = 1.0)
        QPointF a = projectToScreen(track[i-1].normalized(), mvp);
        QPointF b = projectToScreen(track[i].normalized(),   mvp);
        if (!isFacingCamera(track[i].normalized() * 0.9f, mvp)) continue;
        float t = (float)i / n;
        int alpha = (int)(t * 90);
        p.setPen(QPen(QColor(col.red(), col.green(), col.blue(), alpha), 1, Qt::DotLine));
        p.drawLine(a, b);
    }
}

void GlobeWidget::resizeGL(int w, int h) {
    m_proj.setToIdentity();
    float aspect = (float)w / (float)(h ? h : 1);
    m_proj.perspective(40.f, aspect, 0.1f, 100.f);
}

void GlobeWidget::buildGridLines() {
    m_gridLines.clear();
    const int latSteps = 12;
    const int lonSteps = 18;
    const int seg = 80;

    for (int i = 0; i <= latSteps; i++) {
        float lat = -90.f + 180.f * i / latSteps;
        for (int j = 0; j < seg; j++) {
            float l0 = 360.f * j / seg;
            float l1 = 360.f * (j+1) / seg;
            m_gridLines.push_back(spherePt(lat, l0));
            m_gridLines.push_back(spherePt(lat, l1));
        }
    }
    for (int i = 0; i <= lonSteps; i++) {
        float lon = 360.f * i / lonSteps;
        for (int j = 0; j < seg; j++) {
            float la0 = -90.f + 180.f * j / seg;
            float la1 = -90.f + 180.f * (j+1) / seg;
            m_gridLines.push_back(spherePt(la0, lon));
            m_gridLines.push_back(spherePt(la1, lon));
        }
    }
}

void GlobeWidget::buildOrbitLine(float inclination, float radius, int segments, QVector<QVector3D> &out) {
    out.clear();
    float inc = inclination * M_PI / 180.f;
    for (int i = 0; i <= segments; i++) {
        float angle = 2.f * M_PI * i / segments;
        float x = radius * cosf(angle);
        float y = radius * sinf(angle) * sinf(inc);
        float z = radius * sinf(angle) * cosf(inc);
        out.push_back(QVector3D(x, y, z));
    }
}

void GlobeWidget::buildContinentLines() {
    m_continentLines.clear();
    for (int c = 0; c < kContinentCount; c++) {
        const auto &cont = kContinents[c];
        QVector3D prev;
        bool hasPrev = false;
        for (int i = 0; i < cont.count; i++) {
            const auto &gp = cont.pts[i];
            if (gp.lat < -990.f) {
                hasPrev = false;
                continue;
            }
            float rlat = gp.lat * float(M_PI) / 180.f;
            float rlon = gp.lon * float(M_PI) / 180.f;
            QVector3D cur(
                cosf(rlat) * cosf(rlon),
                sinf(rlat),
                cosf(rlat) * sinf(rlon)
            );
            if (hasPrev) {
                m_continentLines.push_back(prev);
                m_continentLines.push_back(cur);
            }
            prev    = cur;
            hasPrev = true;
        }
    }
}

bool GlobeWidget::isFacingCamera(const QVector3D &worldPt, const QMatrix4x4 &mvp) {
    QVector4D clip = mvp * QVector4D(worldPt, 1.f);
    return clip.z() / clip.w() < 0.98f;
}

QPointF GlobeWidget::projectToScreen(const QVector3D &world, const QMatrix4x4 &mvp) {
    QVector4D clip = mvp * QVector4D(world, 1.f);
    if (qAbs(clip.w()) < 1e-6f) return QPointF(-9999, -9999);
    float ndcX = clip.x() / clip.w();
    float ndcY = clip.y() / clip.w();
    float sx = (ndcX * 0.5f + 0.5f) * width();
    float sy = (1.f - (ndcY * 0.5f + 0.5f)) * height();
    return QPointF(sx, sy);
}

QVector3D GlobeWidget::orbitPosition(float inclination, float phase, float radius) {
    float inc = inclination * M_PI / 180.f;
    float x = radius * cosf(phase);
    float y = radius * sinf(phase) * sinf(inc);
    float z = radius * sinf(phase) * cosf(inc);
    return QVector3D(x, y, z);
}

void GlobeWidget::drawLines(const QVector<QVector3D> &pts, const QVector3D &color, float alpha, bool loop) {
    if (pts.isEmpty()) return;
    m_prog->setUniformValue(m_uColor, color.x(), color.y(), color.z(), alpha);
    m_prog->setUniformValue(m_uDepthFade, 0.f);

    int loc = m_prog->attributeLocation("position");
    m_prog->enableAttributeArray(loc);
    m_prog->setAttributeArray(loc, GL_FLOAT, pts.constData(), 3);

    if (loop) {
        glDrawArrays(GL_LINE_LOOP, 0, pts.size());
    } else {
        glDrawArrays(GL_LINES, 0, pts.size());
    }
    m_prog->disableAttributeArray(loc);
}

void GlobeWidget::drawLinesGradient(const QVector<QVector3D> &pts, const QVector3D &color, float alphaFront, float) {
    if (pts.isEmpty()) return;
    m_prog->setUniformValue(m_uColor, color.x(), color.y(), color.z(), alphaFront);
    m_prog->setUniformValue(m_uDepthFade, 1.f);

    int loc = m_prog->attributeLocation("position");
    m_prog->enableAttributeArray(loc);
    m_prog->setAttributeArray(loc, GL_FLOAT, pts.constData(), 3);
    glDrawArrays(GL_LINES, 0, pts.size());
    m_prog->disableAttributeArray(loc);

    m_prog->setUniformValue(m_uDepthFade, 0.f);
}

void GlobeWidget::drawSatellite(const QVector3D &pos, const QVector3D &forward, const QVector3D &color) {
    QVector3D up(0,1,0);
    QVector3D right = QVector3D::crossProduct(forward, up).normalized();
    if (right.length() < 0.01f) {
        up = QVector3D(1,0,0);
        right = QVector3D::crossProduct(forward, up).normalized();
    }
    up = QVector3D::crossProduct(right, forward).normalized();

    float bodyLen = 0.07f;
    float wingSpan = 0.14f;
    float wingWidth = 0.025f;

    QVector<QVector3D> body;
    body.push_back(pos - forward * bodyLen * 0.5f);
    body.push_back(pos + forward * bodyLen * 0.5f);

    QVector<QVector3D> wing1;
    wing1.push_back(pos - right * wingSpan * 0.5f - up * wingWidth);
    wing1.push_back(pos - right * wingSpan * 0.5f + up * wingWidth);
    wing1.push_back(pos + right * wingSpan * 0.5f + up * wingWidth);
    wing1.push_back(pos + right * wingSpan * 0.5f - up * wingWidth);

    m_prog->setUniformValue(m_uColor, color.x(), color.y(), color.z(), 1.0f);

    int loc = m_prog->attributeLocation("position");

    m_prog->enableAttributeArray(loc);
    m_prog->setAttributeArray(loc, GL_FLOAT, body.constData(), 3);
    glDrawArrays(GL_LINES, 0, 2);

    m_prog->setAttributeArray(loc, GL_FLOAT, wing1.constData(), 3);
    glDrawArrays(GL_LINE_LOOP, 0, 4);
    m_prog->disableAttributeArray(loc);
}

void GlobeWidget::drawPoleAxis(QPainter &p, const QMatrix4x4 &mvp) {
    QVector3D north(0.f,  1.05f, 0.f);
    QVector3D south(0.f, -1.05f, 0.f);
    QVector3D center(0.f, 0.f, 0.f);
    QPointF pN = projectToScreen(north,  mvp);
    QPointF pC = projectToScreen(center, mvp);
    QPointF pS = projectToScreen(south,  mvp);

    p.setRenderHint(QPainter::Antialiasing, true);
    p.setPen(QPen(QColor(0, 170, 150, 120), 1, Qt::DashLine));
    p.drawLine(pS, pN);

    QFont f("Courier", 7);
    f.setBold(true);
    p.setFont(f);
    p.setPen(QColor(0, 190, 170, 200));
    p.drawText(pN + QPointF(4, 4), "N");
    p.drawText(pS + QPointF(4, -2), "S");
}

void GlobeWidget::drawEquatorTicks(QPainter &p, const QMatrix4x4 &mvp) {
    p.setRenderHint(QPainter::Antialiasing, false);
    QFont f("Courier", 6);
    p.setFont(f);
    for (int lon = 0; lon < 360; lon += 30) {
        float rlon = lon * float(M_PI) / 180.f;
        QVector3D outer(1.08f * cosf(rlon), 0.f, 1.08f * sinf(rlon));
        QVector3D inner(0.97f * cosf(rlon), 0.f, 0.97f * sinf(rlon));
        if (!isFacingCamera(outer, mvp)) continue;
        QPointF po = projectToScreen(outer, mvp);
        QPointF pi = projectToScreen(inner, mvp);
        p.setPen(QPen(QColor(0, 180, 155, 140), 1));
        p.drawLine(pi, po);
        p.setPen(QColor(0, 150, 130, 160));
        p.drawText(po + QPointF(2, 4), QString::number(lon) + QString::fromUtf8("\xC2\xB0"));
    }
}

void GlobeWidget::drawSatLabels(QPainter &p, const QMatrix4x4 &mvp) {
    QFont f("Courier", 8);
    f.setBold(true);
    p.setFont(f);
    p.setRenderHint(QPainter::Antialiasing, true);

    const auto &sats = m_model->satellites;
    for (int si = 0; si < (int)sats.size(); si++) {
        const auto &sat = sats[si];
        QVector3D pos = orbitPosition(sat.orbitInclination, sat.satAngle, sat.orbitRadius);
        if (!isFacingCamera(pos * 0.9f, mvp)) continue;

        QPointF sc = projectToScreen(pos, mvp);
        QColor col = sat.orbitColor;

        p.setPen(QPen(QColor(col.red(), col.green(), col.blue(), 180), 1));
        p.drawLine(sc, sc + QPointF(12, -14));

        p.setPen(QColor(col.red(), col.green(), col.blue(), 220));
        p.drawText(sc + QPointF(14, -16), sat.name);

        bool hasAlert = false;
        for (const auto &rd : sat.readings)
            if (rd.alert == AlertLevel::Critical) hasAlert = true;
        if (hasAlert && m_model->geoSync.blinkOn) {
            p.setPen(QPen(QColor(220, 30, 30, 200), 1));
            p.setBrush(Qt::NoBrush);
            p.drawEllipse(sc, 10, 10);
        }
    }
}

void GlobeWidget::drawAtmosphere(QPainter &p) {
    int cx = width() / 2;
    int cy = height() / 2 + 10;
    int rx = (int)(width()  * 0.41f);
    int ry = (int)(height() * 0.44f);

    QRadialGradient grad(cx, cy, qMax(rx, ry) + 18);
    grad.setColorAt(0.70, QColor(0, 0, 0, 0));
    grad.setColorAt(0.82, QColor(0, 180, 160, 18));
    grad.setColorAt(0.91, QColor(0, 220, 190, 38));
    grad.setColorAt(0.97, QColor(0, 200, 175, 18));
    grad.setColorAt(1.00, QColor(0, 0, 0, 0));
    p.setBrush(grad);
    p.setPen(Qt::NoPen);
    p.drawEllipse(cx - rx - 20, cy - ry - 20, (rx + 20) * 2, (ry + 20) * 2);
}

void GlobeWidget::drawSatTrail(int satIdx, QPainter &p, const QMatrix4x4 &mvp) {
    if (satIdx >= m_trails.size()) return;
    const auto &trail = m_trails[satIdx];
    if (trail.pts.size() < 2) return;

    const auto &sat = m_model->satellites[satIdx];
    QColor col = sat.orbitColor;

    p.setRenderHint(QPainter::Antialiasing, true);
    int n = trail.pts.size();
    for (int i = 1; i < n; i++) {
        QPointF a = projectToScreen(trail.pts[i-1], mvp);
        QPointF b = projectToScreen(trail.pts[i],   mvp);
        float t = (float)i / n;
        int alpha = (int)(t * t * 180);
        p.setPen(QPen(QColor(col.red(), col.green(), col.blue(), alpha), 1.5f));
        p.drawLine(a, b);
    }
}

void GlobeWidget::drawPingDot(QPainter &p, const QVector3D &worldPos, const QMatrix4x4 &mvp,
                               const QColor &color, float pingPhase) {
    QPointF sc = projectToScreen(worldPos, mvp);
    if (sc.x() < -200) return;

    float pulse = (sinf(pingPhase * 4.f) + 1.f) * 0.5f;
    int r1 = 3;
    int r2 = (int)(5 + pulse * 7);
    int alpha2 = (int)((1.f - pulse) * 160);

    p.setRenderHint(QPainter::Antialiasing, true);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(color.red(), color.green(), color.blue(), alpha2));
    p.drawEllipse(sc, r2, r2);
    p.setBrush(QColor(color.red(), color.green(), color.blue(), 230));
    p.drawEllipse(sc, r1, r1);

    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(QColor(color.red(), color.green(), color.blue(), (int)(pulse * 200)), 1));
    p.drawEllipse(sc, r2 + 2, r2 + 2);
}

void GlobeWidget::paintGL() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_pingPhase += 0.016f;
    m_rotY      += 0.08f;   if (m_rotY > 360.f) m_rotY -= 360.f;
    m_sunAngle  += 0.004f;  if (m_sunAngle > 360.f) m_sunAngle -= 360.f;

    QMatrix4x4 view;
    view.lookAt(QVector3D(0, 0.3f, 3.2f), QVector3D(0,0,0), QVector3D(0,1,0));

    QMatrix4x4 modelMat;
    modelMat.rotate(m_rotY, 0, 1, 0);
    modelMat.rotate(-18.f, 1, 0, 0);

    QMatrix4x4 mvp = m_proj * view * modelMat;
    m_lastMVP = mvp;

    // ── Pass 1: solid sphere (depth-tested, lit) ──────────────────────────────
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    drawSolidSphere(mvp, modelMat);

    // ── Pass 2: wireframe lines on top (additive blending, depth-read-only) ───
    glDepthMask(GL_FALSE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    m_prog->bind();
    m_prog->setUniformValue(m_uMVP, mvp);

    glLineWidth(1.0f);
    drawLinesGradient(m_gridLines, QVector3D(0.04f, 0.55f, 0.48f), 0.38f, 0.06f);

    glLineWidth(1.4f);
    drawLinesGradient(m_continentLines, QVector3D(0.08f, 0.95f, 0.80f), 0.90f, 0.18f);

    const auto &sats = m_model->satellites;
    for (int si = 0; si < (int)sats.size(); si++) {
        const auto &sat = sats[si];

        QVector<QVector3D> orbitPts;
        buildOrbitLine(sat.orbitInclination, sat.orbitRadius, 200, orbitPts);
        QVector<QVector3D> orbitLines;
        for (int i = 0; i < orbitPts.size()-1; i++) {
            orbitLines.push_back(orbitPts[i]);
            orbitLines.push_back(orbitPts[i+1]);
        }
        float rc = sat.orbitColor.redF();
        float gc = sat.orbitColor.greenF();
        float bc = sat.orbitColor.blueF();
        glLineWidth(1.5f);
        drawLinesGradient(orbitLines, QVector3D(rc, gc, bc), 0.85f, 0.18f);

        float phase = sat.satAngle;
        QVector3D satPos  = orbitPosition(sat.orbitInclination, phase,         sat.orbitRadius);
        QVector3D satPos2 = orbitPosition(sat.orbitInclination, phase + 0.10f, sat.orbitRadius);
        QVector3D fwd = (satPos2 - satPos).normalized();
        glLineWidth(2.5f);
        drawSatellite(satPos, fwd, QVector3D(qMin(1.f, rc*1.5f), qMin(1.f, gc*1.5f), qMin(1.f, bc*1.2f)));

        // Update trail
        if (si < m_trails.size()) {
            auto &trail = m_trails[si];
            trail.pts.push_back(satPos);
            if (trail.pts.size() > SatTrail::MaxLen) trail.pts.pop_front();
        }
        // Update ground track (surface projection)
        if (si < m_groundTracks.size()) {
            auto &gt = m_groundTracks[si];
            gt.push_back(satPos);
            if (gt.size() > 300) gt.pop_front();
        }
    }

    m_prog->release();

    // Restore normal blending
    glDepthMask(GL_TRUE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // ── QPainter overlays ─────────────────────────────────────────────────────
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false);

    drawAtmosphere(painter);
    drawPoleAxis(painter, mvp);
    drawEquatorTicks(painter, mvp);

    for (int si = 0; si < (int)sats.size(); si++)
        drawGroundTrack(si, painter, mvp);

    for (int si = 0; si < (int)sats.size(); si++)
        drawSatTrail(si, painter, mvp);

    for (int si = 0; si < (int)sats.size(); si++) {
        QVector3D satPos = orbitPosition(sats[si].orbitInclination, sats[si].satAngle, sats[si].orbitRadius);
        drawPingDot(painter, satPos, mvp, sats[si].orbitColor, m_pingPhase + si * 1.4f);
    }

    drawSatLabels(painter, mvp);

    // Scanlines
    for (int y = 0; y < height(); y += 3) {
        painter.setPen(QColor(0, 0, 0, 22));
        painter.drawLine(0, y, width(), y);
    }

    // Corner HUD
    QColor cyan(0, 210, 185);
    QColor dimCyan(0, 90, 80);
    int w = width(), h = height();
    int bLen = 22, margin = 8;

    painter.setPen(QPen(cyan, 2));
    painter.drawLine(margin, margin, margin+bLen, margin);         painter.drawLine(margin, margin, margin, margin+bLen);
    painter.drawLine(w-margin, margin, w-margin-bLen, margin);     painter.drawLine(w-margin, margin, w-margin, margin+bLen);
    painter.drawLine(margin, h-margin, margin+bLen, h-margin);     painter.drawLine(margin, h-margin, margin, h-margin-bLen);
    painter.drawLine(w-margin, h-margin, w-margin-bLen, h-margin); painter.drawLine(w-margin, h-margin, w-margin, h-margin-bLen);

    painter.setPen(QPen(dimCyan, 1));
    painter.drawLine(margin+bLen+4, margin+1, w/2-40, margin+1);
    painter.drawLine(w/2+40,        margin+1, w-margin-bLen-4, margin+1);

    QFont font("Courier", 8);
    font.setBold(true);
    painter.setFont(font);
    painter.setPen(cyan);

    QString label = QString("LEO TRACK  //  %1").arg(m_rotY, 6, 'f', 1);
    QFontMetrics fm(font);
    painter.drawText(w/2 - fm.horizontalAdvance(label)/2, margin + 14, label);

    // CUDA status indicator
    painter.setPen(QColor(0, 180, 140, 180));
    QFont tiny("Courier", 6);
    painter.setFont(tiny);
    painter.drawText(margin + 2, h - margin - 4, "CUDA:ON");

    painter.end();
    update();
}
