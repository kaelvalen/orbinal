#include "SatelliteModel.h"
#include "CudaCompute.h"
#include <cstdlib>
#include <cmath>

float SatelliteModel::rng(float lo, float hi) {
    return lo + (hi - lo) * (float)rand() / (float)RAND_MAX;
}

static void initChannel(QVector<float> &ch, float phase, float amp, int n) {
    ch.clear();
    for (int i = 0; i < n; i++) {
        float t = i * 0.05f;
        ch.push_back(amp * sinf(t * 1.3f + phase)
                   + amp * 0.6f * sinf(t * 3.7f + phase * 0.5f)
                   + amp * 0.2f * sinf(t * 8.1f + phase * 1.2f));
    }
}

SatelliteModel::SatelliteModel() {
    geoSync.network     = "A03";
    geoSync.accessId    = "1052";
    geoSync.port        = "203.667.21";
    geoSync.calibration = "OK";
    geoSync.layers      = "ALL";
    geoSync.alignment   = "MAN";
    geoSync.nav         = "01";
    geoSync.orbit       = "LOCKED";
    geoSync.orbitChanged = false;
    geoSync.blinkTimer   = 0.f;
    geoSync.blinkOn      = true;

    cbkUnits = {
        {"CBK-01", true,  true,  0.0f},
        {"CBK-02", true,  false, 0.7f},
        {"OFFLINE", false, false, 0.0f},
        {"CBK-04", true,  false, 1.4f},
        {"OFFLINE", false, false, 0.0f},
    };

    auto makeReading = [](const QString &id, float v, bool hi, AlertLevel al = AlertLevel::Normal) -> SatReading {
        SatReading r; r.id = id; r.value = v; r.targetValue = v; r.highlighted = hi; r.alert = al;
        return r;
    };

    SatelliteData s1;
    s1.name = "SAT-01";
    s1.readings = {
        makeReading("AR01", 11.246f, false),
        makeReading("AR02", 61.507f, true,  AlertLevel::Warning),
        makeReading("AR03",  2.236f, true,  AlertLevel::Warning),
        makeReading("----",  0.f,    false),
        makeReading("AR17", 54.791f, false),
        makeReading("AR52", 37.024f, false)
    };
    s1.orbitInclination = 23.f;
    s1.orbitPhase       = 0.f;
    s1.orbitSpeed       = 0.4f;
    s1.orbitRadius      = 1.35f;
    s1.orbitColor       = QColor(220, 100, 40);
    s1.satAngle         = 0.f;
    s1.signalPhase      = 0.f;
    initChannel(s1.signalChannel, 0.f, 0.06f, 200);
    satellites.push_back(s1);

    SatelliteData s2;
    s2.name = "SAT-02";
    s2.readings = {
        makeReading("AR19", 53.436f, false),
        makeReading("AR20", 41.731f, false),
        makeReading("AR21",  2.235f, false),
        makeReading("AR22", 94.003f, true,  AlertLevel::Critical),
        makeReading("AR23", 10.256f, false),
        makeReading("AR24", 77.833f, false)
    };
    s2.orbitInclination = 55.f;
    s2.orbitPhase       = 1.2f;
    s2.orbitSpeed       = 0.28f;
    s2.orbitRadius      = 1.5f;
    s2.orbitColor       = QColor(200, 80, 30);
    s2.satAngle         = 2.0f;
    s2.signalPhase      = 1.7f;
    initChannel(s2.signalChannel, 1.7f, 0.055f, 200);
    satellites.push_back(s2);

    SatelliteData s4;
    s4.name = "SAT-04";
    s4.readings = {
        makeReading("AR49", 18.224f, true,  AlertLevel::Warning),
        makeReading("AR50", 61.715f, false),
        makeReading("AR51", 25.140f, false),
        makeReading("AR52", 68.247f, false),
        makeReading("----",  0.f,    false),
        makeReading("AR54", 19.422f, true,  AlertLevel::Warning)
    };
    s4.orbitInclination = -30.f;
    s4.orbitPhase       = 2.5f;
    s4.orbitSpeed       = 0.35f;
    s4.orbitRadius      = 1.25f;
    s4.orbitColor       = QColor(210, 90, 35);
    s4.satAngle         = 4.5f;
    s4.signalPhase      = 3.3f;
    initChannel(s4.signalChannel, 3.3f, 0.065f, 200);
    satellites.push_back(s4);

    initChannel(signalHistory,  0.0f, 0.06f, 300);
    initChannel(signalHistoryB, 1.1f, 0.04f, 300);
    initChannel(signalHistoryC, 2.3f, 0.035f, 300);
}

static const char *kTickerMessages[] = {
    "UPLINK NOMINAL - ALL SYSTEMS GO",
    "TELEMETRY STREAM ACTIVE",
    "HANDSHAKE CONFIRMED: CBK-01",
    "SIGNAL LOCK ESTABLISHED",
    "ORBITAL PARAMETER UPDATE PENDING",
    "ATTITUDE CONTROL NOMINAL",
    "THERMAL REGULATION OK",
    "POWER SUBSYSTEM: 98.2%",
    "RANGING DATA UPDATED",
    "TRANSPONDER ONLINE",
    "DOPPLER CORRECTION APPLIED",
    "NAV SOLUTION CONVERGED",
    "GROUND STATION SYNC: OK",
    "TLM BUFFER FLUSHED",
    "ANTENNA POINTING VERIFIED",
    nullptr
};
static const char *kAlertMessages[] = {
    "WARNING: ANOMALOUS READING DETECTED",
    "ALERT: ORBIT DEVIATION THRESHOLD EXCEEDED",
    "CRITICAL: SIGNAL INTEGRITY DEGRADED",
    "WARNING: POWER FLUCTUATION ON SAT-02",
    nullptr
};

void SatelliteModel::pushTicker(const QString &msg, AlertLevel lv) {
    TickerMessage tm;
    tm.text  = msg;
    tm.level = lv;
    tickerLog.prepend(tm);
    if (tickerLog.size() > 40) tickerLog.pop_back();
}

void SatelliteModel::update(float dt) {
    m_time        += dt;
    m_signalPhase += dt;
    m_signalPhaseB += dt * 1.13f;
    m_signalPhaseC += dt * 0.87f;

    for (auto &sat : satellites) {
        sat.satAngle    += sat.orbitSpeed * dt;
        sat.signalPhase += dt * 1.2f;
        sat.alertTimer  += dt;

        sat.signalChannel.pop_front();
        float sv = 0.06f * sinf(sat.signalPhase * 1.3f)
                 + 0.04f * sinf(sat.signalPhase * 3.7f)
                 + 0.012f * rng(-1.f, 1.f);
        sat.signalChannel.push_back(sv);
    }

    tickReadings(dt);

    // GPU-accelerated signal generation: compute one new sample per channel
    {
        auto &cuda = CudaCompute::instance();
        QVector<float> s;
        cuda.generateSignalSamples(m_signalPhase,  0.f, 0.06f, 0.039f, 0.018f, m_time,        s, 1);
        signalHistory.pop_front();  signalHistory.push_back(s[0]);
        cuda.generateSignalSamples(m_signalPhaseB, 0.f, 0.04f, 0.026f, 0.012f, m_time + 1.f,  s, 1);
        signalHistoryB.pop_front(); signalHistoryB.push_back(s[0]);
        cuda.generateSignalSamples(m_signalPhaseC, 0.f, 0.035f,0.023f, 0.010f, m_time + 2.f,  s, 1);
        signalHistoryC.pop_front(); signalHistoryC.push_back(s[0]);
    }

    geoSync.blinkTimer += dt;
    if (geoSync.blinkTimer > 0.5f) {
        geoSync.blinkTimer = 0.f;
        geoSync.blinkOn = !geoSync.blinkOn;
    }

    m_orbitChangedTimer += dt;
    if (m_orbitChangedTimer > 9.f) {
        m_orbitChangedTimer = 0.f;
        m_orbitChanged = !m_orbitChanged;
        geoSync.orbitChanged = m_orbitChanged;
        geoSync.orbit = m_orbitChanged ? "CHANGED" : "LOCKED";
        if (m_orbitChanged) {
            randomizeReadings();
            sirenActive = true;
            sirenTimer  = 3.5f;
            pushTicker("ORBIT CHANGE DETECTED - RECALIBRATING", AlertLevel::Critical);
        } else {
            pushTicker("ORBIT LOCK RESTORED - NOMINAL", AlertLevel::Warning);
        }
    }

    if (sirenActive) {
        sirenTimer -= dt;
        sirenFlash  = (sinf(m_time * 12.f) + 1.f) * 0.5f;
        if (sirenTimer <= 0.f) { sirenActive = false; sirenFlash = 0.f; }
    }

    updateTicker(dt);
    updateRadar(dt);

    typing.timer += dt;
    if (!typing.done && typing.timer >= typing.speed) {
        typing.timer = 0.f;
        if (typing.shown < typing.full.length())
            typing.shown++;
        else
            typing.done = true;
    }
}

void SatelliteModel::updateTicker(float dt) {
    m_tickerSpawnTimer += dt;
    float interval = sirenActive ? 1.2f : 3.8f;
    if (m_tickerSpawnTimer >= interval) {
        m_tickerSpawnTimer = 0.f;
        if (sirenActive && kAlertMessages[m_tickerMsgIdx % 4]) {
            pushTicker(kAlertMessages[m_tickerMsgIdx % 4], AlertLevel::Warning);
        } else {
            int idx = m_tickerMsgIdx % 15;
            if (kTickerMessages[idx])
                pushTicker(kTickerMessages[idx], AlertLevel::Normal);
        }
        m_tickerMsgIdx++;
    }
}

void SatelliteModel::updateRadar(float dt) {
    radarSweepAngle += dt * 90.f;
    if (radarSweepAngle >= 360.f) radarSweepAngle -= 360.f;

    for (auto &b : radarBlips) b.life -= dt * 0.35f;
    radarBlips.erase(std::remove_if(radarBlips.begin(), radarBlips.end(),
        [](const RadarBlip &b){ return b.life <= 0.f; }), radarBlips.end());

    // GPU-accelerated blip detection
    QVector<float> satAngles;
    for (const auto &sat : satellites) {
        float deg = fmodf(sat.satAngle * 180.f / float(M_PI), 360.f);
        if (deg < 0) deg += 360.f;
        satAngles.push_back(deg);
    }

    QVector<int> hits;
    CudaCompute::instance().detectRadarBlips(radarSweepAngle, 3.f, satAngles, hits);

    for (int i = 0; i < hits.size(); i++) {
        if (hits[i]) {
            RadarBlip b;
            b.angle  = satAngles[i];
            b.radius = 0.3f + rng(0.1f, 0.55f);
            b.life   = 1.f;
            b.color  = satellites[i].orbitColor;
            radarBlips.push_back(b);
        }
    }
}

void SatelliteModel::tickReadings(float dt) {
    for (auto &sat : satellites) {
        for (auto &r : sat.readings) {
            if (r.id == "----") continue;
            float diff = r.targetValue - r.value;
            r.value += diff * qMin(1.f, dt * 6.f);
            r.value += rng(-0.002f, 0.002f);
            r.value = qMax(0.f, qMin(99.999f, r.value));
        }
    }
}

void SatelliteModel::randomizeReadings() {
    for (auto &sat : satellites) {
        for (auto &r : sat.readings) {
            if (r.id == "----") continue;
            r.targetValue = rng(0.f, 99.999f);
            r.highlighted = (rng(0.f, 1.f) > 0.65f);
            if (r.highlighted) {
                r.alert = (rng(0.f, 1.f) > 0.5f) ? AlertLevel::Critical : AlertLevel::Warning;
            } else {
                r.alert = AlertLevel::Normal;
            }
        }
    }
}
