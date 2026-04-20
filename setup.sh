#!/bin/bash
set -e

echo "===== Starting Environment Setup for CogniArm Project ====="

sudo apt update

PACKAGES=(
    "libasound2-dev"
    "nlohmann-json3-dev"
    "libopencv-dev"
    "libsdl2-dev"
    "libcamera-apps"
    "gstreamer1.0-libcamera"
    "libcamera-v4l2"
    "libonnxruntime-dev"
    "cmake"  
    "build-essential"
)

echo "Installing dependencies..."
sudo apt install -y "${PACKAGES[@]}"

echo "===== Setup Completed Successfully! ====="