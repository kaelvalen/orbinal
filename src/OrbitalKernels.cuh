#pragma once
#include <cuda_runtime.h>

#ifdef __cplusplus
extern "C" {
#endif

void cuda_computeOrbitPositions(
    float inclination, float radius,
    float phaseOffset, float phaseStep,
    float3 *d_out, int N);

void cuda_generateSignalSamples(
    float phaseBase, float phaseStep,
    float ampA, float ampB, float ampC,
    float noiseSeed,
    float *d_out, int N);

void cuda_detectRadarBlips(
    float sweepAngle, float tolerance,
    float *d_satAngles, int numSats,
    int *d_hitFlags);

#ifdef __cplusplus
}
#endif
