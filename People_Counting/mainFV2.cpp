// Tencent is pleased to support the open source community by making ncnn available.
//
// Copyright (C) 2020 THL A29 Limited, a Tencent company. All rights reserved.
//
// Licensed under the BSD 3-Clause License (the "License"); you may not use this file except
// in compliance with the License. You may obtain a copy of the License at
//
// https://opensource.org/licenses/BSD-3-Clause
//
// Unless required by applicable law or agreed to in writing, software distributed
// under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
// CONDITIONS OF ANY KIND, either express or implied. See the License for the
// specific language governing permissions and limitations under the License.

//modified 12-31-2021 Q-engineering

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

int people_in = 0, people_out = 0;

static void draw_objects(cv::Mat& cvImg, const std::vector<TargetBox>& boxes)
{
    int person_index = std::find(class_names, class_names + sizeof(class_names) / sizeof(class_names[0]), "person") - class_names;
    for(size_t i = 0; i < boxes.size(); i++) {
//        std::cout<<boxes[i].x1<<" "<<boxes[i].y1<<" "<<boxes[i].x2<<" "<<boxes[i].y2
//                 <<" "<<boxes[i].score<<" "<<boxes[i].cate<<std::endl;
        if(boxes[i].cate + 1 == person_index && boxes[i].score >= 0.5){
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
            people_in++;
            people_out+=2;
        }
    }
}

int main(int argc, char** argv)
{
    int socket_desc;
    struct sockaddr_in server;

    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_desc == -1) {
        printf("Could not create socket\n");
        return 1;
    }

    server.sin_addr.s_addr = inet_addr("192.168.68.115"); // Replace with the actual server IP address
    server.sin_family = AF_INET;
    server.sin_port = htons(5006); // You can choose a port number

    if (connect(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("Connect failed. Error");
        return 1;
    }
    // using easywsclient::WebSocket;
    // WebSocket::pointer ws = WebSocket::from_url("ws://192.168.68.115:5005/");
    // assert(ws);

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
	while(1){
//        frame=cv::imread("000139.jpg");  //need to refresh frame before dnn class detection
        cap >> frame;
        if (frame.empty()) {
            std::cerr << "ERROR: Unable to grab from the camera" << std::endl;
            break;
        }

        Tbegin = std::chrono::steady_clock::now();

        std::vector<TargetBox> boxes;
        yoloF2.detection(frame, boxes);
        draw_objects(frame, boxes);
        Tend = std::chrono::steady_clock::now();

        //calculate frame rate
        f = std::chrono::duration_cast <std::chrono::milliseconds> (Tend - Tbegin).count();
        if(f>0.0) FPS[((Fcnt++)&0x0F)]=1000.0/f;
        for(f=0.0, i=0;i<16;i++){ f+=FPS[i]; }
        putText(frame, cv::format("FPS %0.2f", f/16),cv::Point(10,20),cv::FONT_HERSHEY_SIMPLEX,0.6, cv::Scalar(0, 0, 255));

        //show outputstd::cerr << "ERROR: Unable to grab from the camera" << std::endl;
        cv::imshow("Counting System",frame);

        char buffer[1024];
        sprintf(buffer, "People In: %d, People Out: %d", people_in, people_out);

        if (send(socket_desc, buffer, strlen(buffer), 0) < 0) {
            perror("Send failed");
            return 1;
        }
        //cv::imwrite("test.jpg",frame);
        // ws->send("hello");
        //std::cout << "OK";
        char esc = cv::waitKey(5);
        if(esc == 27) break;
	}
	close(socket_desc);
    return 0;
}
