#ifndef WEBCAM_H
#define WEBCAM_H

#include <stdio.h>
#include <inttypes.h>
#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/imgproc.hpp>
#include <QImage>

class Webcam
{
private:
    size_t width;
    size_t height;
    bool opened = false;
    cv::Mat frame;
    cv::Mat grayed;
    cv::Mat resized;
    cv::VideoCapture capture;
public:
    int open(int devID);
    int close();
    void setScale(size_t width, size_t height);
    bool isOpened();
    //uint8_t* data();
    QImage getFrame();
};

#endif // WEBCAM_H
