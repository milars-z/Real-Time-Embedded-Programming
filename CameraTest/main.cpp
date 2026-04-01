#include "AttentionDetector.hpp"
#include "FeatureManager.hpp"
#include "CameraEngine.hpp" 
#include <iostream>
#include <mutex>
#include <cstdlib>

using namespace std;
using namespace cv;

mutex frame_mtx;
Mat shared_frame;

int main() {
    string model_path = "../model/mobilenet_v2_slice.onnx";
    
    // single core computing
    cv::setNumThreads(1);

    setenv("OMP_NUM_THREADS", "1", 1);
    setenv("OPENBLAS_NUM_THREADS", "1", 1);
    setenv("MKL_NUM_THREADS", "1", 1);

    // Image detection module module
    AttentionDetector detector(model_path);
    // Feature Management Module
    FeatureManager feat_mgr("my_features.yml"); 
    // Camera hardware layer
    CameraEngine cam;

    //Open the thread and save the current frame to shared_frame
    cam.onFrame([&](const Mat& img){
        if(img.empty()) return;
        lock_guard<mutex> lock(frame_mtx);
        img.copyTo(shared_frame);
    });

    // Start camera thread to receive data
    cam.start(); 

    namedWindow("Demo", WINDOW_AUTOSIZE);
    Mat current_frame, display_frame;

    while(true) {
        {
            //Check if there are any images in shared_frame, and process img data if there are
            lock_guard<mutex> lock(frame_mtx);
            if(shared_frame.empty()) {
                waitKey(10); continue;
            }
            shared_frame.copyTo(current_frame);
        }
        auto start_time = std::chrono::high_resolution_clock::now();
        // Save original frame
        display_frame = current_frame.clone();

        // Obtain test results
        // DetectedObject class vector, used to store all detected objs
        vector<DetectedObject> objects = detector.detect(current_frame);
        auto end_time = std::chrono::high_resolution_clock::now();

        // Draw a regular detection box
        // Only used for module demonstration or debugging, not required for actual operation
        for(auto& obj : objects) {
            rectangle(display_frame, obj.box, Scalar(0, 255, 0), 2);
            // Display ID
            
            double duration_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();
            std::string text_time = cv::format("CNN Inference: %.2f ms", duration_ms);  

            putText(display_frame, to_string(obj.id), Point(obj.box.x, obj.box.y-5), 
                    FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0,255,0), 1);
            putText(display_frame, text_time, Point(obj.box.x, obj.box.y+20), 
                    FONT_HERSHEY_SIMPLEX, 0.7, Scalar(0,255,0), 2);
        }

        imshow("Demo", display_frame);

        // Single module button demonstration
        int key = waitKey(30);
        if (key == 'q') break;
        

        // [B] Update Background
        if (key == 'b') {
            detector.update_background(current_frame);
        }
        
        // [S] Save the largest object as' obj_1 '
        else if (key == 's') {
            if (objects.empty()) {
                cout << "There are no objects in the picture, unable to save." << endl;
            } else {
                // Find the object with the largest area
                int max_idx = 0; 
                float max_area = 0;
                for(int i=0; i<objects.size(); i++) {
                    if (objects[i].score > max_area) {
                        max_area = objects[i].score;
                        max_idx = i;
                    }
                }
                feat_mgr.save_feature(objects[max_idx], "obj_1");
            }
        }

        // [M] Match 'obj_1'
        else if (key == 'm') {
            auto start_time = std::chrono::high_resolution_clock::now();
            // Call the matching function, which will modify the matchname in the objects
            // Called in the main function to detect different items
            // Return the ID of the object, and subsequently pass in the attributes of this ID, 
            // such as the center point coordinates for the robotic arm to grasp and the orientation for the robotic arm to move to that orientation
            int idx = feat_mgr.match_object("obj_1", objects, 0.2f); // 
            
            auto end_time = std::chrono::high_resolution_clock::now();

            if (idx != -1) {
                // Highlight the matched object
                // For demonstration purposes
                // After detection, return the information to the main thread core or robotic arm core for processing
                Rect b = objects[idx].box;
                rectangle(display_frame, b, Scalar(0, 0, 255), 3); 

                double duration_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();
                std::string text_time = cv::format("Feature Match: %.2f ms", duration_ms); 

                putText(display_frame, "Found: obj_1", Point(b.x, b.y+b.height+20), 
                        FONT_HERSHEY_SIMPLEX, 0.8, Scalar(0,0,255), 2);

                putText(display_frame, text_time, Point(b.x, b.y + b.height - 20), 
                        FONT_HERSHEY_SIMPLEX, 0.7, Scalar(0, 255, 0), 2); 
                
                imshow("Demo", display_frame); 
                cout << ">>> Match successful, press any key to continue .." << endl;
                waitKey(0);
            } else {
                cout << ">>> Obj_1 not found" << endl;
            }
        }
    }
    
    return 0;
}