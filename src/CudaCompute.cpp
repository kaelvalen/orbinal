#include "CudaCompute.h"
#include <cmath>
#include <cstring>

#ifdef ORBINAL_CUDA_ENABLED
#  include <cuda_runtime.h>
#  include "OrbitalKernels.cuh"
#endif

CudaCompute &CudaCompute::instance() {
    static CudaCompute inst;
    return inst;
}

CudaCompute::CudaCompute() {
#ifdef ORBINAL_CUDA_ENABLED
    int deviceCount = 0;
    cudaError_t err = cudaGetDeviceCount(&deviceCount);
    if (err == cudaSuccess && deviceCount > 0) {
        cudaSetDevice(0);
        m_available = true;
    }
#endif
}

CudaCompute::~CudaCompute() {
#ifdef ORBINAL_CUDA_ENABLED
    if (m_d_orbitOut)  cudaFree(m_d_orbitOut);
    if (m_d_signalOut) cudaFree(m_d_signalOut);
    if (m_d_satAngles) cudaFree(m_d_satAngles);
    if (m_d_hitFlags)  cudaFree(m_d_hitFlags);
#endif
}

// ── Orbit positions ───────────────────────────────────────────────────────────

void CudaCompute::computeOrbitPositions(
    float inclination, float radius,
    float phaseOffset, float phaseStep,
    QVector<QVector3D> &out, int N)
{
    out.resize(N);

#ifdef ORBINAL_CUDA_ENABLED
    if (m_available) {
        if (m_orbitBufN < N) {
            if (m_d_orbitOut) cudaFree(m_d_orbitOut);
            cudaMalloc(&m_d_orbitOut, N * sizeof(float3));
            m_orbitBufN = N;
        }
        cuda_computeOrbitPositions(
            inclination, radius, phaseOffset, phaseStep,
            (float3 *)m_d_orbitOut, N);

        QVector<float3> tmp(N);
        cudaMemcpy(tmp.data(), m_d_orbitOut, N * sizeof(float3), cudaMemcpyDeviceToHost);
        for (int i = 0; i < N; i++)
            out[i] = QVector3D(tmp[i].x, tmp[i].y, tmp[i].z);
        return;
    }
#endif

    // CPU fallback
    float inc = inclination;
    for (int i = 0; i < N; i++) {
        float phase = phaseOffset + i * phaseStep;
        out[i] = QVector3D(
            radius * cosf(phase),
            radius * sinf(phase) * sinf(inc),
            radius * sinf(phase) * cosf(inc)
        );
    }
}

// ── Signal generation ─────────────────────────────────────────────────────────

void CudaCompute::generateSignalSamples(
    float phaseBase, float phaseStep,
    float ampA, float ampB, float ampC,
    float noiseSeed,
    QVector<float> &out, int N)
{
    out.resize(N);

#ifdef ORBINAL_CUDA_ENABLED
    if (m_available) {
        if (m_signalBufN < N) {
            if (m_d_signalOut) cudaFree(m_d_signalOut);
            cudaMalloc(&m_d_signalOut, N * sizeof(float));
            m_signalBufN = N;
        }
        cuda_generateSignalSamples(
            phaseBase, phaseStep, ampA, ampB, ampC, noiseSeed,
            (float *)m_d_signalOut, N);
        cudaMemcpy(out.data(), m_d_signalOut, N * sizeof(float), cudaMemcpyDeviceToHost);
        return;
    }
#endif

    // CPU fallback
    for (int i = 0; i < N; i++) {
        float t = phaseBase + i * phaseStep;
        unsigned int state = (unsigned int)(i * 1664525u + 1013904223u
                                          + (unsigned int)(noiseSeed * 1e6f));
        state = state * 1664525u + 1013904223u;
        float noise = ((float)(state & 0xFFFFu) / 32767.5f) - 1.0f;
        out[i] = ampA * sinf(t * 1.3f)
               + ampB * sinf(t * 3.7f + 0.5f)
               + ampC * sinf(t * 7.1f + 1.2f)
               + ampC * 0.4f * noise;
    }
}

// ── Radar blip detection ──────────────────────────────────────────────────────

void CudaCompute::detectRadarBlips(
    float sweepAngle, float tolerance,
    const QVector<float> &satAngles,
    QVector<int> &hitFlags)
{
    int n = satAngles.size();
    hitFlags.resize(n, 0);

#ifdef ORBINAL_CUDA_ENABLED
    if (m_available && n > 0) {
        if (m_satBufN < n) {
            if (m_d_satAngles) cudaFree(m_d_satAngles);
            if (m_d_hitFlags)  cudaFree(m_d_hitFlags);
            cudaMalloc(&m_d_satAngles, n * sizeof(float));
            cudaMalloc(&m_d_hitFlags,  n * sizeof(int));
            m_satBufN = n;
        }
        cudaMemcpy(m_d_satAngles, satAngles.constData(), n * sizeof(float), cudaMemcpyHostToDevice);
        cuda_detectRadarBlips(sweepAngle, tolerance,
            (float *)m_d_satAngles, n, (int *)m_d_hitFlags);
        cudaMemcpy(hitFlags.data(), m_d_hitFlags, n * sizeof(int), cudaMemcpyDeviceToHost);
        return;
    }
#endif

    // CPU fallback
    for (int i = 0; i < n; i++) {
        float diff = fabsf(satAngles[i] - sweepAngle);
        if (diff > 180.f) diff = 360.f - diff;
        hitFlags[i] = (diff < tolerance) ? 1 : 0;
    }
}
