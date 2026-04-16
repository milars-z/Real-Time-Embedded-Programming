#include "CoordinateTransformer.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <cmath>
#include <stdexcept>

// Mathematical constant for degree to radian conversion
const double PI = 3.14159265358979323846;

CoordinateTransformer::CoordinateTransformer(const std::string& configFilePath) {
    if (!loadConfig(configFilePath)) {
        throw std::runtime_error("Failed to load camera configuration file.");
    }
}

bool CoordinateTransformer::loadConfig(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "[Error] Cannot open config file: " << filepath << std::endl;
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        // Skip comments and empty lines
        if (line.empty() || line[0] == '#') continue;

        std::istringstream is_line(line);
        std::string key;
        if (std::getline(is_line, key, '=')) {
            std::string value;
            if (std::getline(is_line, value)) {
                double val = std::stod(value);
                if (key == "fx") fx = val;
                else if (key == "fy") fy = val;
                else if (key == "cx") cx = val;
                else if (key == "cy") cy = val;
                else if (key == "tx") tx = val;
                else if (key == "ty") ty = val;
                else if (key == "tz") tz = val;
                else if (key == "roll") roll = val;
                else if (key == "pitch") pitch = val;
                else if (key == "yaw") yaw = val;
                else if (key == "max_reach") maxReach = val;
                else if (key == "min_z_height") minZHeight = val;
            }
        }
    }
    return true;
}

Point3D CoordinateTransformer::pixelToBase(int u, int v, double depth) const {

    std::cout << "DEBUG: fx=" << fx << ", fy=" << fy << ", depth=" << depth << std::endl;

    
    // Step 1: Pixel to Camera 3D Frame
    Point3D cameraPoint;
    cameraPoint.x = (u - cx) * depth / fx;
    cameraPoint.y = (v - cy) * depth / fy;
    cameraPoint.z = depth;

    // Step 2: Camera 3D Frame to Robot Base Frame
    Point3D basePoint = applyTransformation(cameraPoint);

    // Step 3: Safety Check
    if (!isSafeToMove(basePoint)) {
        std::cerr << "[Warning] Target point out of safe workspace constraints!" << std::endl;
        // Depending on system architecture, you might return a safe default or throw an exception
    }

    return basePoint;
}

Point3D CoordinateTransformer::applyTransformation(const Point3D& camPoint) const {
    Point3D result;

    // Convert Euler angles from degrees to radians
    double rRad = roll * PI / 180.0;
    double pRad = pitch * PI / 180.0;
    double yRad = yaw * PI / 180.0;

    // Pre-compute sine and cosine values
    double cx = std::cos(rRad), sx = std::sin(rRad);
    double cy = std::cos(pRad), sy = std::sin(pRad);
    double cz = std::cos(yRad), sz = std::sin(yRad);

    // Composite Rotation Matrix calculation (ZYX convention)
    double r11 = cy * cz;
    double r12 = cz * sx * sy - cx * sz;
    double r13 = cx * cz * sy + sx * sz;
    
    double r21 = cy * sz;
    double r22 = cx * cz + sx * sy * sz;
    double r23 = -cz * sx + cx * sy * sz;
    
    double r31 = -sy;
    double r32 = cy * sx;
    double r33 = cx * cy;

    // Apply Rotation and Translation
    result.x = (r11 * camPoint.x + r12 * camPoint.y + r13 * camPoint.z) + tx;
    result.y = (r21 * camPoint.x + r22 * camPoint.y + r23 * camPoint.z) + ty;
    result.z = (r31 * camPoint.x + r32 * camPoint.y + r33 * camPoint.z) + tz;

    return result;
}

bool CoordinateTransformer::isSafeToMove(const Point3D& target) const {
    // Constraint 1: Do not crash into the table
    if (target.z < minZHeight) {
        return false;
    }

    // Constraint 2: Do not exceed maximum physical reach of the arm
    double distanceFromBase = std::sqrt(target.x * target.x + target.y * target.y + target.z * target.z);
    if (distanceFromBase > maxReach) {
        return false;
    }

    return true;
}