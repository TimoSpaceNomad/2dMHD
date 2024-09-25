﻿#pragma once

#include <filesystem>

#include "../Configs.h"

#include "Buffers.cuh"
#include "FastFourierTransformator.cuh"
#include "KernelCaller.cuh"

namespace mhd {
class Helper {
protected:
    // Simulation configurations
    Configs _configs;

    // Auxiliary classes
    FastFourierTransformator _transformator;
    KernelCaller _caller;

    struct Fields {
        // Calculated Fields
        GpuComplexBuffer2D _vorticity;
        GpuComplexBuffer2D _stream;
        GpuComplexBuffer2D _current;
        GpuComplexBuffer2D _potential;

        // Auxiliary Fields: Equations
        GpuComplexBuffer2D _oldVorticity, _oldPotential, _rightPart;

        // Auxiliary Fields: Temporary GPU
        GpuComplexBuffer2D _complexBuffer;
        GpuDoubleBuffer2D _doubleBufferA, _doubleBufferB, _doubleBufferC;

        // Auxiliary Fields: Temporary CPU
        CpuDoubleBuffer1D _cpuLinearBufferX;
        CpuDoubleBuffer1D _cpuLinearBufferY;

        // Output buffer: Temporary CPU
        CpuDoubleBuffer2D _output;

        Fields(unsigned int gridLength, unsigned int linearLength)
            : _vorticity(gridLength),
              _stream(gridLength),
              _current(gridLength),
              _potential(gridLength),
              _oldVorticity(gridLength),
              _oldPotential(gridLength),
              _rightPart(gridLength),
              _complexBuffer(gridLength),
              _doubleBufferA(gridLength),
              _doubleBufferB(gridLength),
              _doubleBufferC(gridLength),
              _cpuLinearBufferX(linearLength),
              _cpuLinearBufferY(linearLength),
              _output(gridLength) {}
    } _fields;

    void normallize(GpuComplexBuffer2D& field, double ratio);
    double maxRotorAmplitude(const GpuComplexBuffer2D& field);
    double calcEnergy(const GpuComplexBuffer2D& field);

public:
    Currents _currents;

    Helper(const Configs& configs);

    void fillNormally(unsigned long seed, int offset = 1);

    void updateStream();
    void updateVorticity();
    void updatePotential();
    void updateCurrent();

    void timeStep();
    void updateTimeStep();
    void updateEnergies();

    void saveOldFields();

    const GpuDoubleBuffer2D& getVorticity();
    const GpuDoubleBuffer2D& getStream();
    const GpuDoubleBuffer2D& getCurrent();
    const GpuDoubleBuffer2D& getPotential();

    GpuComplexBuffer2D& Vorticity();
    GpuComplexBuffer2D& Stream();
    GpuComplexBuffer2D& Current();
    GpuComplexBuffer2D& Potential();
    GpuComplexBuffer2D& OldVorticity();
    GpuComplexBuffer2D& OldPotential();
    GpuComplexBuffer2D& RightPart();
    GpuComplexBuffer2D& ComplexBuffer();
    GpuDoubleBuffer2D& DoubleBufferA();
    GpuDoubleBuffer2D& DoubleBufferB();
    GpuDoubleBuffer2D& DoubleBufferC();
    CpuDoubleBuffer1D& CpuLinearBufferX();
    CpuDoubleBuffer1D& CpuLinearBufferY();
    CpuDoubleBuffer2D& Output();

    bool shouldContinue();
};

class CudaTimeCounter {
private:
    cudaEvent_t _start, _stop;
    float time;

public:
    CudaTimeCounter();
    ~CudaTimeCounter();

    void start();
    void stop();

    float getTime();
};
}  // namespace mhd
