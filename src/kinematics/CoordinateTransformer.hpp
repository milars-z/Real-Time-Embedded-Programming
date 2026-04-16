#ifndef COORDINATE_TRANSFORMER_H
#define COORDINATE_TRANSFORMER_H

#include <string>

// Struct to represent a 3D point in space
struct Point3D {
    double x;
    double y;
    double z;
};

class CoordinateTransformer {
public:
    /**
     * @brief Constructor that initializes parameters from a config file.
     * @param configFilePath Path to the camera configuration file.
     */
    CoordinateTransformer(const std::string& configFilePath);

    /**
     * @brief Transforms 2D image pixel coordinates to 3D base coordinates.
     * @param u Pixel X coordinate.
     * @param v Pixel Y coordinate.
     * @param depth Depth value in mm.
     * @return Point3D containing the physical coordinates relative to the robot base.
     */
    Point3D pixelToBase(int u, int v, double depth) const;

    /**
     * @brief Validates if the calculated target is within the safe workspace.
     * @param target The calculated 3D point.
     * @return True if safe, False if out of bounds or risking collision.
     */
    bool isSafeToMove(const Point3D& target) const;

private:
    // Camera Intrinsic Parameters
    double fx, fy, cx, cy;

    // Camera Extrinsic Parameters (Translation and Rotation)
    double tx, ty, tz;
    double roll, pitch, yaw; // Stored in degrees

    // Workspace Safety Constraints
    double maxReach;
    double minZHeight;

    /**
     * @brief Parses the configuration file and loads parameters.
     * @param filepath Path to the text file.
     * @return True if parsing was successful.
     */
    bool loadConfig(const std::string& filepath);

    /**
     * @brief Applies rotation matrix and translation vector (Homogeneous Transformation).
     * @param camPoint 3D point in the camera coordinate frame.
     * @return 3D point in the robot base coordinate frame.
     */
    Point3D applyTransformation(const Point3D& camPoint) const;
};

#endif // COORDINATE_TRANSFORMER_H