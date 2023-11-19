#include <opencv2/core/utility.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/tracking.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>
#include <stdio.h>
#include <vector>
#include "yolo-fastestv2.h"

#include <sys/socket.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>


#include "easywsclient.hpp"
// #include "easywsclient.cpp" // <-- include only if you don't want compile separately

yoloFastestv2 yoloF2;

const char* class_names[] = {
    "background", "person", "bicycle",
    "car", "motorbike", "aeroplane", "bus", "train", "truck",
    "boat", "traffic light", "fire hydrant", "stop sign",
    "parking meter", "bench", "bird", "cat", "dog", "horse",
    "sheep", "cow", "elephant", "bear", "zebra", "giraffe",
    "backpack", "umbrella", "handbag", "tie", "suitcase",
    "frisbee", "skis", "snowboard", "sports ball", "kite",
    "baseball bat", "baseball glove", "skateboard", "surfboard",
    "tennis racket", "bottle", "wine glass", "cup", "fork",
    "knife", "spoon", "bowl", "banana", "apple", "sandwich",
    "orange", "broccoli", "carrot", "hot dog", "pizza", "donut",
    "cake", "chair", "sofa", "pottedplant", "bed", "diningtable",
    "toilet", "tvmonitor", "laptop", "mouse", "remote", "keyboard",
    "cell phone", "microwave", "oven", "toaster", "sink",
    "refrigerator", "book", "clock", "vase", "scissors",
    "teddy bear", "hair drier", "toothbrush"
};

class Blob {
public:
    // member variables ///////////////////////////////////////////////////////////////////////////
    std::vector<cv::Point> centerPositions;

    // double dblCurrentDiagonalSize;
    // double dblCurrentAspectRatio;

    bool blnCurrentMatchFoundOrNewBlob;

    bool blnStillBeingTracked;

    int intNumOfConsecutiveFramesWithoutAMatch;

    cv::Point predictedNextPosition;
    
    // function prototypes ////////////////////////////////////////////////////////////////////////
    Blob(cv::Point centerPoint);
    void predictNextPosition(void);

};

Blob::Blob(cv::Point centerPoint) {

    cv::Point currentCenter;

    currentCenter.x = centerPoint.x;
    currentCenter.y = centerPoint.y;

    centerPositions.push_back(currentCenter);

    blnStillBeingTracked = true;
    blnCurrentMatchFoundOrNewBlob = true;

    intNumOfConsecutiveFramesWithoutAMatch = 0;
}

void Blob::predictNextPosition(void) {

    int numPositions = (int)centerPositions.size();

    if (numPositions == 1) {

        predictedNextPosition.x = centerPositions.back().x;
        predictedNextPosition.y = centerPositions.back().y;

    }
    else if (numPositions == 2) {

        int deltaX = centerPositions[1].x - centerPositions[0].x;
        int deltaY = centerPositions[1].y - centerPositions[0].y;

        predictedNextPosition.x = centerPositions.back().x + deltaX;
        predictedNextPosition.y = centerPositions.back().y + deltaY;

    }
    else if (numPositions == 3) {

        int sumOfXChanges = ((centerPositions[2].x - centerPositions[1].x) * 2) +
            ((centerPositions[1].x - centerPositions[0].x) * 1);

        int deltaX = (int)std::round((float)sumOfXChanges / 3.0);

        int sumOfYChanges = ((centerPositions[2].y - centerPositions[1].y) * 2) +
            ((centerPositions[1].y - centerPositions[0].y) * 1);

        int deltaY = (int)std::round((float)sumOfYChanges / 3.0);

        predictedNextPosition.x = centerPositions.back().x + deltaX;
        predictedNextPosition.y = centerPositions.back().y + deltaY;

    }
    else if (numPositions == 4) {

        int sumOfXChanges = ((centerPositions[3].x - centerPositions[2].x) * 3) +
            ((centerPositions[2].x - centerPositions[1].x) * 2) +
            ((centerPositions[1].x - centerPositions[0].x) * 1);

        int deltaX = (int)std::round((float)sumOfXChanges / 6.0);

        int sumOfYChanges = ((centerPositions[3].y - centerPositions[2].y) * 3) +
            ((centerPositions[2].y - centerPositions[1].y) * 2) +
            ((centerPositions[1].y - centerPositions[0].y) * 1);

        int deltaY = (int)std::round((float)sumOfYChanges / 6.0);

        predictedNextPosition.x = centerPositions.back().x + deltaX;
        predictedNextPosition.y = centerPositions.back().y + deltaY;

    }
    else if (numPositions >= 5) {

        int sumOfXChanges = ((centerPositions[numPositions - 1].x - centerPositions[numPositions - 2].x) * 4) +
            ((centerPositions[numPositions - 2].x - centerPositions[numPositions - 3].x) * 3) +
            ((centerPositions[numPositions - 3].x - centerPositions[numPositions - 4].x) * 2) +
            ((centerPositions[numPositions - 4].x - centerPositions[numPositions - 5].x) * 1);

        int deltaX = (int)std::round((float)sumOfXChanges / 10.0);

        int sumOfYChanges = ((centerPositions[numPositions - 1].y - centerPositions[numPositions - 2].y) * 4) +
            ((centerPositions[numPositions - 2].y - centerPositions[numPositions - 3].y) * 3) +
            ((centerPositions[numPositions - 3].y - centerPositions[numPositions - 4].y) * 2) +
            ((centerPositions[numPositions - 4].y - centerPositions[numPositions - 5].y) * 1);

        int deltaY = (int)std::round((float)sumOfYChanges / 10.0);

        predictedNextPosition.x = centerPositions.back().x + deltaX;
        predictedNextPosition.y = centerPositions.back().y + deltaY;

    }
    else {
        // should never get here
    }
}

void matchCurrentFrameBlobsToExistingBlobs(std::vector<Blob> &existingBlobs, std::vector<Blob> &currentFrameBlobs);
void addBlobToExistingBlobs(Blob &currentFrameBlob, std::vector<Blob> &existingBlobs, int &intIndex);
void addNewBlob(Blob &currentFrameBlob, std::vector<Blob> &existingBlobs);
double distanceBetweenPoints(cv::Point point1, cv::Point point2);
int people_in = 0, people_out = 0;
std::vector<Blob> currentFrameBlobs;

static void draw_objects(cv::Mat& cvImg, const std::vector<TargetBox>& boxes)
{
    int person_index = std::find(class_names, class_names + sizeof(class_names) / sizeof(class_names[0]), "person") - class_names;
    int numOfPeople = 0;
    for(size_t i = 0; i < boxes.size(); i++) {
        if(boxes[i].cate + 1 == person_index && boxes[i].score >= 0.65){
            char text[256];
            sprintf(text, "%s %.1f%%", class_names[boxes[i].cate+1], boxes[i].score * 100);

            int baseLine = 0;
            cv::Size label_size = cv::getTextSize(text, cv::FONT_HERSHEY_SIMPLEX, 0.5, 1, &baseLine);

            int x = boxes[i].x1;
            int y = boxes[i].y1 - label_size.height - baseLine;
            if (y < 0) y = 0;
            if (x + label_size.width > cvImg.cols) x = cvImg.cols - label_size.width;

            cv::rectangle(cvImg, cv::Rect(cv::Point(x, y), cv::Size(label_size.width, label_size.height + baseLine)),
                          cv::Scalar(255, 255, 255), -1);

            cv::putText(cvImg, text, cv::Point(x, y + label_size.height),
                        cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0));

            cv::rectangle (cvImg, cv::Point(boxes[i].x1, boxes[i].y1),
                           cv::Point(boxes[i].x2, boxes[i].y2), cv::Scalar(255,0,0));
            cv::circle( cvImg, cv::Point((boxes[i].x1+boxes[i].x2)/2, (boxes[i].y1+boxes[i].y2)/2), 2, cv::Scalar( 0, 0, 255 ));
            Blob possibleBlob(cv::Point((boxes[i].x1+boxes[i].x2)/2, (boxes[i].y1+boxes[i].y2)/2));
            currentFrameBlobs.push_back(possibleBlob);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void matchCurrentFrameBlobsToExistingBlobs(std::vector<Blob> &existingBlobs, std::vector<Blob> &currentFrameBlobs) {

    for (auto &existingBlob : existingBlobs) {
        existingBlob.blnCurrentMatchFoundOrNewBlob = false;
        existingBlob.predictNextPosition();
    }

    for (auto &currentFrameBlob : currentFrameBlobs) {

        int intIndexOfLeastDistance = 0;
        double dblLeastDistance = 100000.0;

        for (unsigned int i = 0; i < existingBlobs.size(); i++) {
            if (existingBlobs[i].blnStillBeingTracked == true) {
                double dblDistance = distanceBetweenPoints(currentFrameBlob.centerPositions.back(), existingBlobs[i].predictedNextPosition);

                if (dblDistance < dblLeastDistance) {
                    dblLeastDistance = dblDistance;
                    intIndexOfLeastDistance = i;
                }
            }
        }

        if (dblLeastDistance < 45) {
            cv::Point newestCenterPositionCurrent = currentFrameBlob.centerPositions.back();
            std::vector<cv::Point> centerPositions = existingBlobs[intIndexOfLeastDistance].centerPositions;
            cv::Point newestCenterPositionExisting(centerPositions.back().x, centerPositions.back().y);
            // std::cout << "NewestCenterPositionCurrent: " << newestCenterPositionCurrent.x << " " << newestCenterPositionExisting.x << intIndexOfLeastDistance <<"\n";
            if ((newestCenterPositionCurrent.x > 352/2) && (newestCenterPositionExisting.x < 352/2))
                people_in++;
            if ((newestCenterPositionCurrent.x < 352/2) && (newestCenterPositionExisting.x > 352/2))
                people_out++;
            addBlobToExistingBlobs(currentFrameBlob, existingBlobs, intIndexOfLeastDistance);
        }
        else {
            addNewBlob(currentFrameBlob, existingBlobs);
        }

    }

    for (auto &existingBlob : existingBlobs) {

        if (existingBlob.blnCurrentMatchFoundOrNewBlob == false) {
            existingBlob.intNumOfConsecutiveFramesWithoutAMatch++;
        }

        if (existingBlob.intNumOfConsecutiveFramesWithoutAMatch >= 5) {
            existingBlob.blnStillBeingTracked = false;
        }

    }

}

///////////////////////////////////////////////////////////////////////////////////////////////////
void addBlobToExistingBlobs(Blob &currentFrameBlob, std::vector<Blob> &existingBlobs, int &intIndex) {

    existingBlobs[intIndex].centerPositions.push_back(currentFrameBlob.centerPositions.back());

    // existingBlobs[intIndex].dblCurrentDiagonalSize = currentFrameBlob.dblCurrentDiagonalSize;
    // existingBlobs[intIndex].dblCurrentAspectRatio = currentFrameBlob.dblCurrentAspectRatio;

    existingBlobs[intIndex].blnStillBeingTracked = true;
    existingBlobs[intIndex].blnCurrentMatchFoundOrNewBlob = true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void addNewBlob(Blob &currentFrameBlob, std::vector<Blob> &existingBlobs) {

    currentFrameBlob.blnCurrentMatchFoundOrNewBlob = true;

    existingBlobs.push_back(currentFrameBlob);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
double distanceBetweenPoints(cv::Point point1, cv::Point point2) {
    
    int intX = abs(point1.x - point2.x);
    int intY = abs(point1.y - point2.y);

    return(sqrt(pow(intX, 2) + pow(intY, 2)));
}

///////////////////////////////////////////////////////////////////////////////////////////////////

int main(int argc, char** argv){
    int socket_desc;
    struct sockaddr_in server;
    bool blnFirstFrame = true;

    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_desc == -1) {
        printf("Could not create socket\n");
        return 1;
    }

    server.sin_addr.s_addr = inet_addr("192.168.68.115"); 
    server.sin_family = AF_INET;
    server.sin_port = htons(4000); 

    if (connect(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("Connect failed. Error");
        return 1;
    }
    

    float f;
    float FPS[16];
    int i,Fcnt=0;
    cv::Mat frame;
    //some timing
    std::chrono::steady_clock::time_point Tbegin, Tend;

    for(i=0;i<16;i++) FPS[i]=0.0;

    yoloF2.init(false); //we have no GPU

    yoloF2.loadModel("yolo-fastestv2-opt.param","yolo-fastestv2-opt.bin");

    cv::VideoCapture cap(0);
    if (!cap.isOpened()) {
        std::cerr << "ERROR: Unable to open the camera" << std::endl;
        return 0;
    }

    std::cout << "Start grabbing, press ESC on Live window to terminate" << std::endl;

    std::vector<Blob> blobs;
    int frameCounter=0;
	while(1){
//        frame=cv::imread("000139.jpg");  //need to refresh frame before dnn class detection
        cap >> frame;
        if (frame.empty()) {
            std::cerr << "ERROR: Unable to grab from the camera" << std::endl;
            break;
        }

        Tbegin = std::chrono::steady_clock::now();
        // #################################    
        // downscale the image using new  width and height
        int down_width = 352;
        int down_height = 352;
        cv::Mat resized_down;
        //resize down
        resize(frame, resized_down, cv::Size(down_width, down_height), cv::INTER_LINEAR);
        // #################################
        std::vector<TargetBox> boxes;
        yoloF2.detection(resized_down, boxes);
        draw_objects(resized_down, boxes);
        Tend = std::chrono::steady_clock::now();

        if (blnFirstFrame == true) {
            for (auto &currentFrameBlob : currentFrameBlobs) {
                blobs.push_back(currentFrameBlob);
            }
        }
        else {
            matchCurrentFrameBlobsToExistingBlobs(blobs, currentFrameBlobs);
        }
        currentFrameBlobs.clear();
        blnFirstFrame = false;

        cv::line(resized_down, cv::Point(352/2, 0), cv::Point(352/2, 352), cv::Scalar(0, 0, 255), 2);

        //calculate frame rate
        f = std::chrono::duration_cast <std::chrono::milliseconds> (Tend - Tbegin).count();
        if(f>0.0) FPS[((Fcnt++)&0x0F)]=1000.0/f;
        for(f=0.0, i=0;i<16;i++)f+=FPS[i]; 
        putText(resized_down, cv::format("FPS %0.2f", f/16),cv::Point(10,20),cv::FONT_HERSHEY_SIMPLEX,0.6, cv::Scalar(0, 0, 255));
        putText(resized_down, cv::format("People In: %d", people_in),cv::Point(5,345),cv::FONT_HERSHEY_SIMPLEX,0.6, cv::Scalar(255, 0, 0));
        putText(resized_down, cv::format("People Out: %d", people_out),cv::Point(200,345),cv::FONT_HERSHEY_SIMPLEX,0.6, cv::Scalar(255, 0, 0));

        //show outputstd::cerr << "ERROR: Unable to grab from the camera" << std::endl;
        // cv::imshow("Counting System",frame);
        char filename[100]; // Assuming a maximum file name length of 100 characters
        sprintf(filename, "/home/raspberrypi/People_Counting_System/Frames/f%d.jpg", frameCounter);

        cv::imwrite(filename, resized_down);
        char buffer[1024];
        sprintf(buffer, "People In: %d, People Out: %d", people_in, people_out);

        if (send(socket_desc, buffer, strlen(buffer), 0) < 0) {
            perror("Send failed");
            return 1;
        }
        char esc = cv::waitKey(5);
        if(esc == 27) break;
	}
	// close(socket_desc);
    return 0;
}
