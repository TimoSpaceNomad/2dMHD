#include <ctime>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#include "Configs.h"
#include "Writer.h"
#include "cuda/Solver.cuh"
#include "opengl/Creater.h"

std::filesystem::path FindResPath(const std::filesystem::path& exePath) {
    if (exists(exePath / "res"))
        return exePath / "res";

    if (exists(exePath.parent_path() / "res"))
        return exePath.parent_path() / "res";

    if (exists(exePath.parent_path().parent_path() / "res"))
        return exePath.parent_path().parent_path() / "res";

    return std::filesystem::path("");
}

std::filesystem::path FindConfPath(const std::filesystem::path& exePath) {
    if (exists(exePath / "configs"))
        return exePath / "configs";

    if (exists(exePath.parent_path() / "configs"))
        return exePath.parent_path() / "configs";

    if (exists(exePath.parent_path().parent_path() / "configs"))
        return exePath.parent_path().parent_path() / "configs";

    return std::filesystem::path("");
}

std::filesystem::path CreateOutputDir(const mhd::Configs& configs,
                                      const std::filesystem::path& parantPath) {
    if (exists(parantPath) == false)
        create_directory(parantPath);

    tm timeInfo;
    time_t rawTime;
    time(&rawTime);

    char currenOutputDir[80] = "000000_000000";
    if (localtime_s(&timeInfo, &rawTime) == 0) {
        strftime(currenOutputDir, sizeof(currenOutputDir), "%Y%m%d_%H%M%S",
                 &timeInfo);
    }

    auto outputPath = parantPath / std::string(currenOutputDir);

    create_directory(outputPath);

    if (configs._saveData) {
        if (configs._saveVorticity)
            create_directory(outputPath / "vorticity");
        if (configs._saveCurrent)
            create_directory(outputPath / "current");
        if (configs._saveStream)
            create_directory(outputPath / "stream");
        if (configs._savePotential)
            create_directory(outputPath / "potential");
    }

    if (configs._savePNG) {
        if (configs._saveVorticity)
            create_directory(outputPath / "vorticityPNG");
        if (configs._saveCurrent)
            create_directory(outputPath / "currentPNG");
        if (configs._saveStream)
            create_directory(outputPath / "streamPNG");
        if (configs._savePotential)
            create_directory(outputPath / "potentialPNG");
    }

    return outputPath;
}

int main(int argc, char* argv[]) {
    std::cout << "This is two-dimensional magnetohydrodynamic simulation"
              << std::endl
              << std::endl;

    std::filesystem::path exePath =
        std::filesystem::path(argv[0]).parent_path();

    std::filesystem::path configsPath, resPath;

    if (configsPath = FindConfPath(exePath); configsPath.string() == "") {
        std::cout << "Configuration folder not exists" << std::endl;
        return -1;
    }

    if (resPath = FindResPath(exePath); resPath.string() == "") {
        std::cout << "Resources folder not exists" << std::endl;
        return -1;
    }

    std::filesystem::path configFile = configsPath;

    if (argc > 1) {
        configFile /= std::filesystem::path(argv[1]);
    } else {
        configFile /= std::filesystem::path("standart1024.yaml");
    }

    if (!exists(configFile)) {
        std::cout << "Configuration file not exists" << std::endl;
        return -1;
    }

    std::cout << "Configurations file: " << configFile.filename().string() << std::endl;

    mhd::Configs configs(configFile);

    const std::filesystem::path outputPath =
        CreateOutputDir(configs, exePath.parent_path() / "outputs");

    std::cout << "Output directory: " << outputPath.filename().string()
              << std::endl << std::endl;

    std::cout << "Main Parameters:" << std::endl;

    std::cout << configs.ParametersPrint();
    configs.ParametersSave(outputPath);

    mhd::CudaTimeCounter writerCounter;
    std::cout << "Creating writer... ";
    writerCounter.start();
    mhd::Writer writer(outputPath, configs, resPath);
    writerCounter.stop();
    std::cout << "Done. Time: " << writerCounter.getTime() << std::endl;

    mhd::CudaTimeCounter windowCounter;
    std::cout << "Creating window... ";
    windowCounter.start();
    opengl::Creater creater(configs, resPath);
    windowCounter.stop();
    std::cout << "Done. Time: " << windowCounter.getTime() << std::endl;

    mhd::CudaTimeCounter runCounter;
    runCounter.start();
    {
        mhd::CudaTimeCounter solverCounter;
        std::cout << "Creating solver... ";
        solverCounter.start();
        mhd::Solver solver(configs);
        solverCounter.stop();
        std::cout << "Done. Time: " << solverCounter.getTime() << std::endl;

        // Initial Conditions
        solver.fillNormally(static_cast<unsigned long>(std::time(nullptr)));

        // Saving fields from previous timelayer
        solver.saveOldFields();

        // Initial Energy and Time Step
        solver.updateEnergies();
        solver.updateTimeStep();

        std::cout << std::endl
                  << "Simulation starts..." << std::endl
                  << std::endl;

        // Initial Data Output
        writer.saveData(solver, creater);
        creater.PrepareToRun();

        // Main Cycle of the Program
        while (solver.shouldContinue() && creater.ShouldOpen()) {
            // Time Integration Scheme
            // Two-step Scheme

            // First step
            solver.calcKineticRigthPart();
            solver.timeSchemeKin();

            solver.calcMagneticRightPart();
            solver.timeSchemeMag();

            solver.updateStream();
            solver.updateCurrent();

            // Second step
            solver.calcKineticRigthPart();
            solver.timeSchemeKin();

            solver.calcMagneticRightPart();
            solver.timeSchemeMag();

            solver.updateStream();
            solver.updateCurrent();

            // Saving fields from previous timelayer
            solver.saveOldFields();

            // Update Parameters (Energy and Time Step)
            solver.updateEnergies();
            solver.updateTimeStep();
            solver.timeStep();

            // Data Output
            creater.Render(writer.saveData(solver, creater));

            creater.WindowUpdate();
        }
    }
    runCounter.stop();
    std::cout << std::endl
              << "Simulation time: " << runCounter.getTime() << std::endl;
}
