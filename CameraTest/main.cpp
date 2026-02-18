#include "CameraEngine.h"
#include "YoloDetector.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>

static std::vector<std::string> loadClassNames(const std::string& path) {
    std::vector<std::string> names;
    std::ifstream ifs(path);
    if (!ifs.is_open()) return names;
    std::string line;
    while (std::getline(ifs, line)) {
        if (!line.empty()) names.push_back(line);
    }
    return names;
}

int main(int argc, char** argv) {
    // Usage:
    // 1) Test image:    CameraTest test <image_path> <model.onnx> [class_names.txt]
    // 2) Camera stream: CameraTest camera <model.onnx> [class_names.txt]
    // 3) Video file:    CameraTest video <video_path> <model.onnx> [class_names.txt]
    //    or:           CameraTest <model.onnx> [class_names.txt]

    bool isTest = false;
    bool isVideo = false;
    std::string mode = "camera";
    std::string imagePath;
    std::string videoPath;
    std::string modelPath = "yolov5s.onnx";
    std::string classFile;

    if (argc >= 2) {
        mode = argv[1];
        if (mode == "test") isTest = true;
        if (mode == "video") isVideo = true;
    }

    if (isTest) {
        if (argc >= 4) {
            imagePath = argv[2];
            modelPath = argv[3];
            if (argc >= 5) classFile = argv[4];
        } else {
            std::cerr << "Usage: " << argv[0] << " test <image_path> <model.onnx> [class_names.txt]" << std::endl;
            return 1;
        }
    } else if (isVideo) {
        if (argc >= 4) {
            videoPath = argv[2];
            modelPath = argv[3];
            if (argc >= 5) classFile = argv[4];
        } else {
            std::cerr << "Usage: " << argv[0] << " video <video_path> <model.onnx> [class_names.txt]" << std::endl;
            return 1;
        }
    } else {
        // camera mode
        if (argc >= 3) {
            // CameraTest camera <model> [class]
            modelPath = argv[2];
            if (argc >= 4) classFile = argv[3];
        } else if (argc == 2) {
            // CameraTest <model>
            modelPath = argv[1];
        }
    }

    std::vector<std::string> classNames;
    if (!classFile.empty()) classNames = loadClassNames(classFile);

    YoloDetector detector(modelPath);
    if (!detector.isValid()) {
        std::cerr << "YoloDetector failed to initialize with model: " << modelPath << std::endl;
        return 1;
    }

    if (isTest) {
        cv::Mat img = cv::imread(imagePath);
        if (img.empty()) {
            std::cerr << "Failed to read image: " << imagePath << std::endl;
            return 1;
        }
        auto dets = detector.detect(img);
        detector.drawDetections(img, dets, classNames);
        cv::imshow("Detections", img);
        std::cout << "Press any key to exit..." << std::endl;
        cv::waitKey(0);
        return 0;
    }

    CameraEngine cam;
    std::mutex frameMutex;
    std::condition_variable frameCv;
    cv::Mat latestFrame;
    std::atomic<bool> hasFrame(false);
    std::atomic<bool> running(true);

    cam.onFrame([&](const cv::Mat& img) {
        if (!running || img.empty()) return;
        {
            std::lock_guard<std::mutex> lock(frameMutex);
            latestFrame = img.clone();
            hasFrame = true;
        }
        frameCv.notify_one();
    });

    std::thread inferThread([&]() {
        const char* winName = isVideo ? "Video Detections" : "Camera Detections";
        while (running) {
            cv::Mat frame;
            {
                std::unique_lock<std::mutex> lock(frameMutex);
                frameCv.wait(lock, [&]() { return !running || hasFrame.load(); });
                if (!running) break;
                frame = latestFrame.clone();
                hasFrame = false;
            }

            if (frame.empty()) continue;
            auto dets = detector.detect(frame);
            detector.drawDetections(frame, dets, classNames);
            cv::imshow(winName, frame);
            cv::waitKey(1);
        }
    });

    if (isVideo) {
        if (cam.startFromFile(videoPath)) {
            std::cout << "运行中，按 Enter 键停止..." << std::endl;
            std::cin.get();
        }
        running = false;
        frameCv.notify_all();
        cam.stop();
        if (inferThread.joinable()) inferThread.join();
        return 0;
    }

    if (cam.start(640, 480, 30)) {
        std::cout << "运行中，按 Enter 键停止..." << std::endl;
        std::cin.get();
    }

    running = false;
    frameCv.notify_all();
    cam.stop();
    if (inferThread.joinable()) inferThread.join();
    return 0;
}