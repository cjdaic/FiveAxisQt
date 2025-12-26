#ifndef LASER_PARAM_CALCULATOR_H
#define LASER_PARAM_CALCULATOR_H

#include <vector>
#include <tuple>
#include <string>

// Structure to hold input parameters for the laser computation
struct LaserParams {
    double spotSize;      // Laser spot diameter (mm)
    double overlap;       // Pulse overlap ratio (0 to 1)
    double hatchSpacing;  // Hatch line spacing on the surface (mm)
    double targetEnergy;  // Target energy per unit area E_A (J/mm^2)
    double minPower;      // Minimum laser power (W)
    double maxPower;      // Maximum laser power (W)
    double minFreq;       // Minimum pulse frequency (Hz)
    double maxFreq;       // Maximum pulse frequency (Hz)
    double curvWeight;    // Curvature compensation weight k_c (in mm, e.g. spot size)

    // Constructor with defaults (example values can be adjusted)
    LaserParams()
        : spotSize(0.1), overlap(0.5), hatchSpacing(0.05), targetEnergy(5.0),
        minPower(10.0), maxPower(400.0), minFreq(10.0), maxFreq(200.0),
        curvWeight(0.1) {
    }
};

// Each vertex's output parameters (Power, Speed, Frequency)
struct VertexLaserParams {
    double P;   // Power at vertex (W)
    double v;   // Scan speed at vertex (mm/s)
    double f;   // Pulse frequency at vertex (Hz)
};

// Function to compute laser parameters for each vertex of a mesh.
// `objFile` is the path to the input OBJ model. 
// `params` contains the input settings (spot size, overlap, etc.).
// Output: a vector of VertexLaserParams for each vertex index (size = number of mesh vertices).
// Returns true on success, false on failure (e.g., file not found or OpenMesh error).
bool computeLaserParameters(const std::string& objFile,
    const LaserParams& params,
    std::vector<VertexLaserParams>& vertexParamsOut);

#endif // LASER_PARAM_CALCULATOR_H