#include <iostream>
#include "opencv2/opencv.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui/highgui.hpp"
#include <algorithm>
#include <map>
#include <vector>
#include <numeric>
#include <fstream>

using namespace cv;
using namespace std;
using cv::CLAHE;

RNG rng(12345);
// TODO: Only accept circle/curve if it appears in successive frames (ex: at least 5 frames in a row)
// TODO: Select ROI (region of interest) to cut out unnecessary areas of the frame
// TODO: Detect curve instead of circle (maybe degrees in bend? curvature related to 2*pi?)
// Idea: detect first frame(s) and subtract them. Then use that difference to detect the background
// Some observations: The wafer circumference contour never breaks, but the notch always does when in motion. Maybe detect that break.

int main() {
    /*std::ofstream test_data;
    test_data.open("test_data.csv");
    test_data << "Frame #,Time (seconds)\n";*/

    int frame_num = 0;                                                // Counter tracks frames elapsed (checks in increments of 10)
    float time = 0;
    int counter = 0;                                                  // Counter tracks circle detection across several frames
    bool circleFound = false;                                          // Checks if circle was detected in current frame
    bool notchFound = false;                                          // Checks if notch was found (after averaging 10 frames)

    VideoCapture cap(0);
    if (!cap.isOpened()) {
        cout << "Error opening video stream or file" << endl;
        return -1;
    }
    while (1) {
        Mat frame;
        // Capture frame-by-frame
        cap >> frame;

        //Display the resulting frame
        cv::namedWindow("input");
        cv::imshow("Frame", frame);

        // Read frame
        cv::Mat img = frame;
        cv::Mat canny;
    
        /// Convert it to gray
        cv::Mat gray;
        cv::cvtColor(img, gray, COLOR_BGR2GRAY);

        // Let's try blurring
        cv::GaussianBlur(gray, gray, Size(9, 9), 2, 2);
        //cv::namedWindow("gaussian"); cv::imshow("gaussian", gray);

        // Try CLAHE (anti-glare)
        Ptr<CLAHE> clahe = createCLAHE();
        clahe->setClipLimit(4);
        Mat dst;
        clahe->apply(gray, dst);
        namedWindow("input");
        imshow("CLAHE", dst);

        // compute canny 
        cv::Canny(gray, canny, 200, 20);
        cv::namedWindow("canny2"); cv::imshow("canny2", canny > 0);

        // Dilation & Erosion
        Mat dilated;
        dilate(canny, dilated, Mat(), Point(-1, -1), 2, 1, 1);
        namedWindow("dilated");
        imshow("Dilated", dilated);

        /// Apply the Hough Transform to find the circles
        std::vector<cv::Vec3f> circles;
        cv::HoughCircles(dilated, circles, HOUGH_GRADIENT, 2, gray.rows, 30, 60);


        // Draw the circles detected
        int rect_x = 0;
        int rect_y = 0;
        int radius = 0;
        for (size_t i = 0; i < circles.size(); i++)
        {
            Point center(cvRound(circles[i][0]), cvRound(circles[i][1]));
            radius = cvRound(circles[i][2]);

            rect_x = cvRound(circles[i][0]) - radius;
            rect_y = cvRound(circles[i][1]) - radius;

            if (radius > 60)
                continue;

            cv::circle(img, center, 3, Scalar(0, 255, 255), -1);          // Yellow center point of circle
            cv::circle(img, center, radius, Scalar(0, 0, 255), 1);        // Red outline of circle
            counter++;
            notchFound = true;
        }
        
        frame_num++;
        time = float(frame_num / 20.0);
        // if counter >= 5 frames...notch detected
        if (frame_num % 10 == 0) {
            if (counter >= 5) {
                notchFound = true;
            }
            else {
                notchFound = false;
            }
            counter = 0;
        }

        

        if (notchFound == true) {
            // Draw rect
            //Rect rect(rect_x, rect_y, radius * 2, radius); // Rectangle: center +/- radius
            //rectangle(img, rect, cv::Scalar(0, 255, 0));
            /*test_data << frame_num << "," << time << "\n";*/
            cout << "Notch found (frame: " << frame_num << ", time: " << time << "seconds)" << endl;
        }

        cv::namedWindow("output"); cv::imshow("output", img);

        /*if (frame_num == 400) {
            break;
        }*/

        // Press  ESC on keyboard to exit
        char c = (char)waitKey(25);
        if (c == 27)
            break;
    }
    /*test_data.close();*/

    cap.release();

    // Closes all the frames
    destroyAllWindows();

	return 0;
}