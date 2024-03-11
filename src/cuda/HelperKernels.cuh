#pragma once
#include "cuda_runtime.h"

#include <cufft.h>
#include <curand_kernel.h>

#define M_PI 3.141592653589793238462643

namespace mhd {
// Multiplication Kernels
__global__ static void MultDouble_kernel(double* input, unsigned int gridLength,
                                         double value, double* output) {
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;
    int idx = gridLength * x + y;

    output[idx] = input[idx] * value;
}

__global__ static void MultComplex_kernel(const cufftDoubleComplex* input,
                                          unsigned int gridLength, double value,
                                          cufftDoubleComplex* output) {
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;
    int idx = (gridLength / 2 + 1) * x + y;

    output[idx].x = input[idx].x * value;
    output[idx].y = input[idx].y * value;
}

// Differentiation Kernels
__global__ static void DiffByX_kernel(const cufftDoubleComplex* input,
                                      unsigned int gridLength,
                                      cufftDoubleComplex* output) {
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;
    int idx = (gridLength / 2 + 1) * x + y;

    if (x > gridLength / 2) {
        x = x - gridLength;
    }

    output[idx].x = -(double)x * input[idx].y;
    output[idx].y = (double)x * input[idx].x;
}

__global__ static void DiffByY_kernel(const cufftDoubleComplex* input,
                                      unsigned int gridLength,
                                      cufftDoubleComplex* output) {
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;
    int idx = (gridLength / 2 + 1) * x + y;

    output[idx].x = -(double)y * input[idx].y;
    output[idx].y = (double)y * input[idx].x;
}

__global__ static void LaplasOperator_kernel(const cufftDoubleComplex* input,
                                             unsigned int gridLength,
                                             cufftDoubleComplex* output) {
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;
    int idx = (gridLength / 2 + 1) * x + y;

    if (x > gridLength / 2) {
        x = x - gridLength;
    }
    double value = -(double)(x * x + y * y);

    output[idx].x = value * input[idx].x;
    output[idx].y = value * input[idx].y;
}

__global__ static void MinusLaplasOperator_kernel(
    const cufftDoubleComplex* input, unsigned int gridLength,
    cufftDoubleComplex* output) {
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;
    int idx = (gridLength / 2 + 1) * x + y;

    if (x > gridLength / 2) {
        x = x - gridLength;
    }
    double value = (double)(x * x + y * y);

    output[idx].x = value * input[idx].x;
    output[idx].y = value * input[idx].y;
}

__global__ static void InverseLaplasOperator_kernel(
    const cufftDoubleComplex* input, unsigned int gridLength,
    cufftDoubleComplex* output) {
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;
    int idx = (gridLength / 2 + 1) * x + y;

    if (x > gridLength / 2) {
        x = x - gridLength;
    }
    double value = (idx == 0) ? 0.0 : (-1.) / (double)(x * x + y * y);

    output[idx].x = value * input[idx].x;
    output[idx].y = value * input[idx].y;
}

__global__ static void MinusInverseLaplasOperator_kernel(
    const cufftDoubleComplex* input, unsigned int gridLength,
    cufftDoubleComplex* output) {
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;
    int idx = (gridLength / 2 + 1) * x + y;

    if (x > gridLength / 2) {
        x = x - gridLength;
    }
    double value = (idx == 0) ? 0.0 : 1. / (double)(x * x + y * y);

    output[idx].x = value * input[idx].x;
    output[idx].y = value * input[idx].y;
}

// Shared Memory Kernels
__global__ static void Max_kernel(const double* input, double* output) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;

    extern __shared__ double sharedBuffer[];
    sharedBuffer[threadIdx.x] = fabs(input[idx]);

    __syncthreads();

    for (unsigned int i = 2; i < blockDim.x + 1; i = i * 2) {
        if (idx % i == 0) {
            sharedBuffer[threadIdx.x] =
                (sharedBuffer[threadIdx.x + i / 2] > sharedBuffer[threadIdx.x])
                    ? sharedBuffer[threadIdx.x + i / 2]
                    : sharedBuffer[threadIdx.x];
        }
        __syncthreads();
    }

    output[blockIdx.x] = sharedBuffer[0];
}

__global__ static void EnergyTransform_kernel(double* velocityX,
                                              double* velocityY, double* energy,
                                              unsigned int gridLength,
                                              double lambda) {
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;
    int idx = gridLength * x + y;

    velocityX[idx] *= lambda;
    velocityY[idx] *= lambda;

    energy[idx] =
        (velocityX[idx] * velocityX[idx] + velocityY[idx] * velocityY[idx]) /
        2.;
}

__global__ static void EnergyIntegrate_kernel(double* field, double* sum) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;

    extern __shared__ double sharedBuffer[];
    sharedBuffer[threadIdx.x] = field[idx];

    __syncthreads();

    for (unsigned int i = 2; i < blockDim.x + 1; i = i * 2) {
        if (idx % i == 0) {
            sharedBuffer[threadIdx.x] =
                sharedBuffer[threadIdx.x] + sharedBuffer[threadIdx.x + i / 2];
        }
        __syncthreads();
    }

    sum[blockIdx.x] = sharedBuffer[0];
}

// Initial Conditions
__global__ static void FillStates(curandState* state, unsigned int gridLength,
                                  unsigned long seed) {
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;
    int idx = (gridLength / 2 + 1) * x + y;

    curand_init(seed, idx, 0, &state[idx]);
}

__global__ static void FillNormally_kernel(cufftDoubleComplex* f,
                                           curandState* state,
                                           unsigned int gridLength,
                                           unsigned int averageWN) {
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;
    int idx = (gridLength / 2 + 1) * x + y;

    if (x > gridLength / 2) {
        x = x - gridLength;
    }
    double k = sqrt((double)(x * x + y * y));

    double value =
        (k > 0) ? (double)(gridLength * gridLength) *
                      exp(-(k * k) / (2.f * (double)(averageWN * averageWN))) /
                      sqrt(k)
                : 0.0;

    double phase = 2.f * M_PI * curand_uniform(&state[idx]);

    f[idx].x = cos(phase) * value;
    f[idx].y = sin(phase) * value;
}
}  // namespace mhd
