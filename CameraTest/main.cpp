#include "CameraEngine.h"

int main() {
    CameraEngine cam;

    cam.onFrame([](const cv::Mat& img) {
        if (img.empty()) return;
        // cv::Mat processed;
        // cv::cvtColor(img, processed, cv::COLOR_BGR2GRAY); // gray
        // cv::imshow("Gray Video", processed);
        cv::Mat colorFrame = img.clone(); 
        cv::imshow("Color Camera", colorFrame);

        cv::waitKey(1);
    });

    if (cam.start(640, 480, 30)) {
        std::cout << "运行中，按 Enter 键停止..." << std::endl;
        std::cin.get();
    }

    cam.stop();
    return 0;
}