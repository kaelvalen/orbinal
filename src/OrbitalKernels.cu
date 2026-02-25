// OrbitalKernels.cu
// GPU kernels: orbit position batch + signal generation batch

#include "OrbitalKernels.cuh"
#include <math.h>

// ── Orbit position kernel ─────────────────────────────────────────────────────
// Computes N orbit positions at different phases for a given inclination/radius.
__global__ void computeOrbitPositions(
    float  inclination,   // radians
    float  radius,
    float  phaseOffset,   // starting phase
    float  phaseStep,     // step per thread
    float3 *out,          // output positions [N]
    int     N
) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= N) return;

    float phase = phaseOffset + idx * phaseStep;
    float x = radius * cosf(phase);
    float y = radius * sinf(phase) * sinf(inclination);
    float z = radius * sinf(phase) * cosf(inclination);
    out[idx] = make_float3(x, y, z);
}

// ── Signal generation kernel ──────────────────────────────────────────────────
// Generates N signal samples using summed sinusoids + noise seed.
__global__ void generateSignalSamples(
    float  phaseBase,
    float  phaseStep,
    float  ampA,
    float  ampB,
    float  ampC,
    float  noiseSeed,
    float *out,
    int    N
) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= N) return;

    float t = phaseBase + idx * phaseStep;

    // LCG noise per thread
    unsigned int state = (unsigned int)(idx * 1664525u + 1013904223u
                                      + (unsigned int)(noiseSeed * 1e6f));
    state = state * 1664525u + 1013904223u;
    float noise = ((float)(state & 0xFFFFu) / 32767.5f) - 1.0f;

    float v = ampA * sinf(t * 1.3f)
            + ampB * sinf(t * 3.7f + 0.5f)
            + ampC * sinf(t * 7.1f + 1.2f)
            + ampC * 0.4f * noise;

    out[idx] = v;
}

// ── Radar sweep blip detection kernel ────────────────────────────────────────
// For each satellite angle, check if sweep is near it.
__global__ void detectRadarBlips(
    float  sweepAngle,    // degrees
    float  tolerance,     // degrees
    float *satAngles,     // satellite angles in degrees [numSats]
    int    numSats,
    int   *hitFlags       // out: 1 if hit, 0 otherwise [numSats]
) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= numSats) return;

    float diff = fabsf(satAngles[idx] - sweepAngle);
    if (diff > 180.f) diff = 360.f - diff;
    hitFlags[idx] = (diff < tolerance) ? 1 : 0;
}

// ── Host-side launcher implementations ───────────────────────────────────────

extern "C" void cuda_computeOrbitPositions(
    float inclination, float radius,
    float phaseOffset, float phaseStep,
    float3 *d_out, int N)
{
    int threads = 256;
    int blocks  = (N + threads - 1) / threads;
    computeOrbitPositions<<<blocks, threads>>>(
        inclination, radius, phaseOffset, phaseStep, d_out, N);
    cudaDeviceSynchronize();
}

extern "C" void cuda_generateSignalSamples(
    float phaseBase, float phaseStep,
    float ampA, float ampB, float ampC,
    float noiseSeed,
    float *d_out, int N)
{
    int threads = 256;
    int blocks  = (N + threads - 1) / threads;
    generateSignalSamples<<<blocks, threads>>>(
        phaseBase, phaseStep, ampA, ampB, ampC, noiseSeed, d_out, N);
    cudaDeviceSynchronize();
}

extern "C" void cuda_detectRadarBlips(
    float sweepAngle, float tolerance,
    float *d_satAngles, int numSats,
    int *d_hitFlags)
{
    int threads = 32;
    int blocks  = (numSats + threads - 1) / threads;
    detectRadarBlips<<<blocks, threads>>>(
        sweepAngle, tolerance, d_satAngles, numSats, d_hitFlags);
    cudaDeviceSynchronize();
}
