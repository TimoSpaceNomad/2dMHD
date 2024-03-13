#pragma once

#include "yaml-cpp/yaml.h"

#include <filesystem>
#include <string>

#define M_PI 3.141592653589793238462643

namespace mhd {

struct Currents {
    double time;
    double timeStep;
    unsigned int stepNumber;

    double kineticEnergy;
    double magneticEnergy;

    double maxVelocityField;
    double maxMagneticField;

    Currents()
        : time(0.),
          timeStep(0.),
          stepNumber(0),
          kineticEnergy(0.),
          magneticEnergy(0.),
          maxVelocityField(0.),
          maxMagneticField(0.) {}
};

class Configs {
private:
    struct DefaultConfigs {
        // Simulation Parameters
        static const unsigned int defaultGridLength = 1024;
        static constexpr double defaultTime = 5.;
        static constexpr double defaultDealCoef = 2. / 3.;
        static constexpr double defaultMaxTimeStep = 0.01;
        static constexpr double defaultCFL = 0.2;

        // Equation Coefficients
        static constexpr double defaultNu = 1.e-4;
        static constexpr double defaultEta = 1.e-4;

        // Initial Condition Coefficients
        static constexpr double defaultKineticEnergy = 0.5;
        static constexpr double defaultMagneticEnergy = 0.5;
        static const unsigned int defaultAverageWN = 10;

        // Output Parameters
        static constexpr double defaultOutputStep = 0.1;
        static constexpr double defaultOutputStart = 0.0;
        static const unsigned int defaultMaxOutputs = 1000;

        // Kernel Run Parameters
        static const unsigned int defaultDimBlockX = 32;
        static const unsigned int defaultDimBlockY = 16;
        static const unsigned int defaultSharedLength = 128;

        // Writer Settings
        static const bool defaultSaveData = false;
        static const bool defaultSavePNG = false;
        static const bool defaultSaveVorticity = true;
        static const bool defaultSaveCurrent = false;
        static const bool defaultSaveStream = false;
        static const bool defaultSavePotential = false;

        // Graphics Settings
        static const bool defaultShowGraphics = true;
        static const unsigned int defaultTexturesCount = 32;
        static const unsigned int defaultWindowWidth = 1024;
        static const unsigned int defaultWindowHeight = 1024;
        static constexpr char defaultColorMap[] = "winter";
    };

    std::filesystem::path _filePath;
    YAML::Node _config;

    // Simulation Parameters
    unsigned int getGridLength() const;
    double getDealCoef() const;
    double getTime() const;
    double getMaxTimeStep() const;
    double getCFL() const;

    // Equation Coefficients
    double getNu();
    double getEta();

    // Initial Condition Coefficients
    double getKineticEnergy();
    double getMagneticEnergy();
    unsigned int getAverageWN();

    // OutputParameters
    double getOutputStep();
    double getOutputStart();
    double getOutputStop();

    // KernelRunParameters
    unsigned int getDimBlockX();
    unsigned int getDimBlockY();
    unsigned int getSharedLength();

    // Writer Settings
    bool getSaveData();
    bool getSavePNG();
    bool getSaveVorticity();
    bool getSaveCurrent();
    bool getSaveStream();
    bool getSavePotential();

    // Graphics Settings
    bool getShowGraphics();
    unsigned int getTexturesCount();
    unsigned int getWindowWidth();
    unsigned int getWindowHeight();
    std::string getColorMap();

public:
    Configs(const std::filesystem::path& filePath);

    std::string ParametersPrint() const;
    void ParametersSave(const std::filesystem::path& outputDir) const;

    // Simulation Parameters
    unsigned int _gridLength;
    double _gridStep;
    double _lambda;
    unsigned int _dealWN;
    double _time;
    double _cfl;
    double _maxTimeStep;

    // Equation Coefficients
    double _nu;
    double _eta;

    // Initial Condition Coefficients
    double _kineticEnergy;
    double _magneticEnergy;
    unsigned int _averageWN;

    // Output Parameters
    double _outputStep;
    double _outputStart;
    double _outputStop;

    // Kernel Run Parameters
    unsigned int _dimBlockX;
    unsigned int _dimBlockY;
    unsigned int _sharedLength;
    unsigned int _linearLength;

    // Writer Settings
    bool _saveData;
    bool _savePNG;
    bool _saveVorticity;
    bool _saveCurrent;
    bool _saveStream;
    bool _savePotential;

    // Graphics Settings
    bool _showGraphics;
    unsigned int _texturesCount;
    unsigned int _windowWidth;
    unsigned int _windowHeight;
    std::string _colorMap;
};
}  // namespace mhd
