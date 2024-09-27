#include <ctime>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#include "Configs.h"
#include "Writer.h"
#include "cuda/Solver.cuh"
#include "openGL/Creater.h"

std::filesystem::path FindConfPath(std::filesystem::path& exePath) {
    if (exists(exePath / "configs")) {
        return exePath / "configs";
    }

    if (exists(exePath.parent_path() / "configs")) {
        exePath = exePath.parent_path();
        return exePath / "configs";
    }

    if (exists(exePath.parent_path().parent_path() / "configs")) {
        exePath = exePath.parent_path().parent_path();
        return exePath / "configs";
    }

    return std::filesystem::path("");
}

std::filesystem::path FindResPath(std::filesystem::path& exePath) {
    if (exists(exePath / "res")) {
        return exePath / "res";
    }

    if (exists(exePath.parent_path() / "res")) {
        exePath = exePath.parent_path();
        return exePath / "res";
    }

    if (exists(exePath.parent_path().parent_path() / "res")) {
        exePath = exePath.parent_path().parent_path();
        return exePath / "res";
    }

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

#if defined(_MSC_VER)
    if (localtime_s(&timeInfo, &rawTime) == 0) {
        strftime(currenOutputDir, sizeof(currenOutputDir), "%Y%m%d_%H%M%S",
                 &timeInfo);
    }
#else
    if (localtime_r(&rawTime, &timeInfo) != nullptr) {
        strftime(currenOutputDir, sizeof(currenOutputDir), "%Y%m%d_%H%M%S",
                 &timeInfo);
    }
#endif

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

    std::filesystem::path projectPath =
        std::filesystem::canonical(argv[0]).parent_path();

    std::filesystem::path configsPath, resPath;

    if (configsPath = FindConfPath(projectPath); configsPath.string() == "") {
        std::cout << "Configuration folder not exists" << std::endl;
        return -1;
    }

    if (resPath = FindResPath(projectPath); resPath.string() == "") {
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

    std::cout << "Configurations file: " << configFile.filename().string()
              << std::endl;

    mhd::Configs configs(configFile);

    const std::filesystem::path outputPath =
        CreateOutputDir(configs, projectPath / "bin" / "outputs");

    std::cout << "Output directory: " << outputPath.filename().string()
              << std::endl
              << std::endl;

    std::cout << "Main Parameters:" << std::endl;

    std::cout << configs.ParametersPrint();
    configs.ParametersSave(outputPath);

    mhd::CudaTimeCounter counter("Creating window... ");
    opengl::Creater creater(configs, resPath);
    counter.done("Done. Time: ");

    counter.restart("Creating writer... ");
    mhd::Writer writer(outputPath, configs, resPath);
    counter.done("Done. Time: ");

    mhd::CudaTimeCounter runCounter;
    {
        counter.restart("Creating solver... ");
        mhd::Solver solver(configs);
        counter.done("Done. Time: ");

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

            // Update Time Step
            solver.updateTimeStep();
            solver.timeStep();

            // Data Output
            creater.Render(writer.saveData(solver, creater));

            creater.WindowUpdate();
        }
    }
    runCounter.done("\nSimulation time: ");
}
