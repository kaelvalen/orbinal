#pragma once
#include <QVector>
#include <QVector3D>

// C++ interface to CUDA kernels.
// Falls back to CPU if CUDA device unavailable.
class CudaCompute {
public:
    static CudaCompute &instance();

    bool isAvailable() const { return m_available; }

    // Compute N orbit positions for given inclination/radius starting at phaseOffset
    void computeOrbitPositions(
        float inclination, float radius,
        float phaseOffset, float phaseStep,
        QVector<QVector3D> &out, int N);

    // Generate N signal samples
    void generateSignalSamples(
        float phaseBase, float phaseStep,
        float ampA, float ampB, float ampC,
        float noiseSeed,
        QVector<float> &out, int N);

    // Returns which satellite indices were hit by radar sweep
    void detectRadarBlips(
        float sweepAngle, float tolerance,
        const QVector<float> &satAngles,
        QVector<int> &hitFlags);

private:
    CudaCompute();
    ~CudaCompute();

    bool   m_available = false;

    // Device buffers (reused across calls)
    void  *m_d_orbitOut  = nullptr;
    void  *m_d_signalOut = nullptr;
    void  *m_d_satAngles = nullptr;
    void  *m_d_hitFlags  = nullptr;
    int    m_orbitBufN   = 0;
    int    m_signalBufN  = 0;
    int    m_satBufN     = 0;
};
