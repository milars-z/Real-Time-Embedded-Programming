#include "CoordinateTransformer.h"

CoordinateTransformer::CoordinateTransformer() {
    // 相机的分辨率是 640x480。
    fx = 600.0; 
    fy = 600.0;
    cx = 320.0; 
    cy = 240.0; 

    offsetX = 100.0; 
    offsetY = 0.0;
    offsetZ = 150.0;
}

Point3D CoordinateTransformer::pixelToBase(int u, int v, double depth) {
    Point3D camera_coords;
    Point3D base_coords;


    camera_coords.x = (u - cx) * depth / fx;
    camera_coords.y = (v - cy) * depth / fy;
    camera_coords.z = depth;

    base_coords.x = camera_coords.x + offsetX;
    base_coords.y = camera_coords.y + offsetY;
    base_coords.z = camera_coords.z + offsetZ;

    return base_coords;
}