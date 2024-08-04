/* INCLUDES FOR THIS PROJECT */
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <cmath>
#include <limits>
#include <opencv2/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/features2d.hpp>
#include <opencv2/xfeatures2d.hpp>
#include <opencv2/xfeatures2d/nonfree.hpp>

#include "dataStructures.h"
#include "matching2D.hpp"

using namespace std;

/* MAIN PROGRAM */
int main(int argc, const char *argv[])
{

    /* INIT VARIABLES AND DATA STRUCTURES */

    // data location
    string dataPath = "../";

    // camera
    string imgBasePath = dataPath + "images/";
    string imgPrefix = "KITTI/2011_09_26/image_00/data/000000"; // left camera, color
    string imgFileType = ".png";
    int imgStartIndex = 0; // first file index to load (assumes Lidar and camera names have identical naming convention)
    int imgEndIndex = 9;   // last file index to load
    int imgFillWidth = 4;  // no. of digits which make up the file index (e.g. img-0001.png)

    // misc
    int dataBufferSize = 2;       // no. of images which are held in memory (ring buffer) at the same time
    vector<DataFrame> dataBuffer; // list of data frames which are held in memory at the same time
    bool bVis = false;            // visualize results

    /* MAIN LOOP OVER ALL IMAGES */

    //Debugging 
    std::vector<float> countsOnVehicle;
    std::vector<float> matchedOnVehicle;
    float match {0};

    std::vector<double> timeDet;
    double time_d1 {0};

    std::vector<double> timeDesc;
    double time_d2 {0};

    for (size_t imgIndex = 0; imgIndex <= imgEndIndex - imgStartIndex; imgIndex++)
    {
        /* LOAD IMAGE INTO BUFFER */

        // assemble filenames for current index
        ostringstream imgNumber;
        imgNumber << setfill('0') << setw(imgFillWidth) << imgStartIndex + imgIndex;
        string imgFullFilename = imgBasePath + imgPrefix + imgNumber.str() + imgFileType;

        // load image from file and convert to grayscale
        cv::Mat img, imgGray;
        img = cv::imread(imgFullFilename);
        cv::cvtColor(img, imgGray, cv::COLOR_BGR2GRAY);

        //// STUDENT ASSIGNMENT
        //// TASK MP.1 -> replace the following code with ring buffer of size dataBufferSize

        // push image into data frame buffer
        DataFrame frame;
        frame.cameraImg = imgGray;
        dataBuffer.push_back(frame);

        if (dataBuffer.size() > dataBufferSize)
        {
            dataBuffer.erase(dataBuffer.begin());
        }

        

        //Checking the size of buffer
        //Use for Debugging 
        // cout<<"Size of the buffer : "<<dataBuffer.size()<<endl;
        
        //// EOF STUDENT ASSIGNMENT
        //cout << "#1 : LOAD IMAGE INTO BUFFER done" << endl;

        /* DETECT IMAGE KEYPOINTS */

        // extract 2D keypoints from current image
        vector<cv::KeyPoint> keypoints; // create empty feature list for current image
        string detectorType = "SIFT"; //HARRIS, FAST, BRISK, ORB, AKAZE, SIFT
        string descriptorType = "SIFT"; // BRIEF, ORB, FREAK, AKAZE, SIFT

        //// STUDENT ASSIGNMENT
        //// TASK MP.2 -> add the following keypoint detectors in file matching2D.cpp and enable string-based selection based on detectorType
        //// -> HARRIS, FAST, BRISK, ORB, AKAZE, SIFT
        time_d1 = detKeypointsModern(keypoints,imgGray,detectorType, false);
        timeDet.push_back(time_d1);
        //// EOF STUDENT ASSIGNMENT

        //// STUDENT ASSIGNMENT
        //// TASK MP.3 -> only keep keypoints on the preceding vehicle

        // only keep keypoints on the preceding vehicle
        bool bFocusOnVehicle = true;
        vector<cv::KeyPoint> keypointsInBbox;
        cv::Rect vehicleRect(535, 180, 180, 150);
        if (bFocusOnVehicle)
        {
            for (auto& kp : keypoints){

                if (vehicleRect.contains(kp.pt)){
                    keypointsInBbox.push_back(kp);
                }
            }
            
        }

        //std::cout<<"KeyPoints on the Vehicle : "<<keypointsInBbox.size()<<std::endl;
        countsOnVehicle.push_back(keypointsInBbox.size());

        keypoints = keypointsInBbox;
        //// EOF STUDENT ASSIGNMENT

        // optional : limit number of keypoints (helpful for debugging and learning)
        bool bLimitKpts = false;
        if (bLimitKpts)
        {
            int maxKeypoints = 50;

            if (detectorType.compare("SHITOMASI") == 0)
            { // there is no response info, so keep the first 50 as they are sorted in descending quality order
                keypoints.erase(keypoints.begin() + maxKeypoints, keypoints.end());
            }
            cv::KeyPointsFilter::retainBest(keypoints, maxKeypoints);
            cout << " NOTE: Keypoints have been limited!" << endl;
        }

        // push keypoints and descriptor for current frame to end of data buffer
        (dataBuffer.end() - 1)->keypoints = keypoints;
        //cout << "#2 : DETECT KEYPOINTS done" << endl;
        /* EXTRACT KEYPOINT DESCRIPTORS */

        //// STUDENT ASSIGNMENT
        //// TASK MP.4 -> add the following descriptors in file matching2D.cpp and enable string-based selection based on descriptorType
        //// -> BRIEF, ORB, FREAK, AKAZE, SIFT

        cv::Mat descriptors;
        
        time_d2=descKeypoints((dataBuffer.end() - 1)->keypoints, (dataBuffer.end() - 1)->cameraImg, descriptors, descriptorType);
        timeDesc.push_back(time_d2);
        
        //// EOF STUDENT ASSIGNMENT

        // push descriptors for current frame to end of data buffer
        (dataBuffer.end() - 1)->descriptors = descriptors;

        //cout << "#3 : EXTRACT DESCRIPTORS done" << endl;

        if (dataBuffer.size() > 1) // wait until at least two images have been processed
        {

            /* MATCH KEYPOINT DESCRIPTORS */

            vector<cv::DMatch> matches;
            string matcherType = "MAT_BF";        // MAT_BF, MAT_FLANN
            string descriptorType = "DES_HOG"; // DES_BINARY, DES_HOG
            string selectorType = "SEL_KNN";       // SEL_NN, SEL_KNN

            //// STUDENT ASSIGNMENT
            //// TASK MP.5 -> add FLANN matching in file matching2D.cpp
            //// TASK MP.6 -> add KNN match selection and perform descriptor distance ratio filtering with t=0.8 in file matching2D.cpp

            
            match = matchDescriptors((dataBuffer.end() - 2)->keypoints, (dataBuffer.end() - 1)->keypoints,
                             (dataBuffer.end() - 2)->descriptors, (dataBuffer.end() - 1)->descriptors,
                             matches, descriptorType, matcherType, selectorType);

            matchedOnVehicle.push_back(match);
            
            //// EOF STUDENT ASSIGNMENT

            // store matches in current data frame
            (dataBuffer.end() - 1)->kptMatches = matches;

            //cout << "#4 : MATCH KEYPOINT DESCRIPTORS done" << endl;

            // visualize matches between current and previous image
            bVis = true;
            if (bVis)
            {
                cv::Mat matchImg = ((dataBuffer.end() - 1)->cameraImg).clone();
                cv::drawMatches((dataBuffer.end() - 2)->cameraImg, (dataBuffer.end() - 2)->keypoints,
                                (dataBuffer.end() - 1)->cameraImg, (dataBuffer.end() - 1)->keypoints,
                                matches, matchImg,
                                cv::Scalar::all(-1), cv::Scalar::all(-1),
                                vector<char>(), cv::DrawMatchesFlags::DRAW_RICH_KEYPOINTS);
                
                    // Resize the image to desired dimensions
                    cv::Mat resizedImg;
                    int newWidth = 1500; // New width
                    int newHeight = 400; // New height
                    cv::resize(matchImg, resizedImg, cv::Size(newWidth, newHeight));

                string windowName = "Matching keypoints between two camera images";
                cv::namedWindow(windowName, cv::WINDOW_AUTOSIZE);
                cv::imshow(windowName, resizedImg);
                //cout << "Press key to continue to next image" << endl;
                cv::waitKey(400);// wait for key to be pressed
            }
            bVis = false;
        }

    } // eof loop over all images

float avgCount {0};
std::cout<<"Key Points on Vehicle: "<<std::endl;
for (auto& i : countsOnVehicle){
    std::cout<<i<<"\t";
    avgCount += i;
}
std::cout<<std::endl<<"Avg Count :"<<avgCount/10<<std::endl;

float avg {0};
std::cout<<"Key Points MATCHED on Vehicle: "<<std::endl;
for (auto& i : matchedOnVehicle){
    std::cout<<i<<"\t";
    avg += i;
}

std::cout<<std::endl<<"Avg :"<<avg/10<<std::endl;

double avgTimeDet {0};
std::cout<<"Detection Time : "<<std::endl;
for (auto& i : timeDet){
    std::cout<<i<<"\t";
    avgTimeDet += i;
}
std::cout<<std::endl<<"Avg DET :"<<avgTimeDet/10<<std::endl;

double avgTimeDesc {0};
std::cout<<"Desc Extraction Time: "<<std::endl;
for (auto& i : timeDesc){
    std::cout<<i<<"\t";
    avgTimeDesc += i;
}
std::cout<<std::endl<<"Avg DESC:"<<avgTimeDesc/10<<std::endl;

    return 0;
}