#pragma once
#include <QString>
#include <QVector>
#include <QColor>
#include <cmath>

enum class AlertLevel { Normal, Warning, Critical };

struct SatReading {
    QString    id;
    float      value       = 0.f;
    float      targetValue = 0.f;
    bool       highlighted = false;
    AlertLevel alert       = AlertLevel::Normal;
};

struct SatelliteData {
    QString              name;
    QVector<SatReading>  readings;
    float                orbitInclination = 0.f;
    float                orbitPhase       = 0.f;
    float                orbitSpeed       = 0.f;
    float                orbitRadius      = 1.f;
    QColor               orbitColor;
    float                satAngle         = 0.f;
    QVector<float>       signalChannel;
    float                signalPhase      = 0.f;
    bool                 alertActive      = false;
    float                alertTimer       = 0.f;
};

struct GeoSyncInfo {
    QString network;
    QString accessId;
    QString port;
    QString calibration;
    QString layers;
    QString alignment;
    QString nav;
    QString orbit;
    bool    orbitChanged = false;
    float   blinkTimer   = 0.f;
    bool    blinkOn      = true;
};

struct CbkUnit {
    QString id;
    bool    online      = true;
    bool    highlighted = false;
    float   blinkPhase  = 0.f;
};

struct TickerMessage {
    QString text;
    AlertLevel level = AlertLevel::Normal;
};

struct TypingEffect {
    QString  full;
    int      shown    = 0;
    float    timer    = 0.f;
    float    speed    = 0.045f;
    bool     done     = false;
};

struct RadarBlip {
    float angle  = 0.f;
    float radius = 0.f;
    float life   = 1.f;
    QColor color;
};

class SatelliteModel {
public:
    SatelliteModel();

    void update(float dt);

    GeoSyncInfo            geoSync;
    QVector<CbkUnit>       cbkUnits;
    QVector<SatelliteData> satellites;
    QVector<float>         signalHistory;
    QVector<float>         signalHistoryB;
    QVector<float>         signalHistoryC;

    QVector<TickerMessage> tickerLog;
    int                    tickerScroll   = 0;
    float                  tickerTimer    = 0.f;

    bool                   sirenActive    = false;
    float                  sirenTimer     = 0.f;
    float                  sirenFlash     = 0.f;

    QVector<RadarBlip>     radarBlips;
    float                  radarSweepAngle = 0.f;

    TypingEffect           typing;

    float time() const { return m_time; }
    void  pushTicker(const QString &msg, AlertLevel lv = AlertLevel::Normal);

private:
    void  randomizeReadings();
    void  tickReadings(float dt);
    void  updateTicker(float dt);
    void  updateRadar(float dt);
    float rng(float lo, float hi);
    float m_time              = 0.f;
    float m_orbitChangedTimer = 0.f;
    bool  m_orbitChanged      = false;
    float m_signalPhase       = 0.f;
    float m_signalPhaseB      = 1.7f;
    float m_signalPhaseC      = 3.3f;
    float m_tickerSpawnTimer  = 0.f;
    int   m_tickerMsgIdx      = 0;
};
