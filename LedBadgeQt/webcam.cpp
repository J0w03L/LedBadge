#include "webcam.h"

int Webcam::open(int devID)
{
    if (this->opened) this->close();

    this->capture.open(devID, cv::CAP_ANY);
    if (!this->capture.isOpened()) return -1;

    this->opened = true;
    return 0;
}

int Webcam::close()
{
    if (!this->opened) return -1;

    this->frame.release();
    this->frame = NULL;

    this->grayed.release();
    this->grayed = NULL;

    this->resized.release();
    this->resized = NULL;

    this->capture.release();

    this->opened = false;
    return 0;
}

void Webcam::setScale(size_t width, size_t height)
{
    this->width = width;
    this->height = height;
}

bool Webcam::isOpened()
{
    return this->opened;
}

QImage Webcam::getFrame()
{
    QImage ret = QImage(this->width, this->height, QImage::Format_Grayscale8);

    // Read image data from camera.
    this->capture.read(this->frame);

    // Convert image data to grayscale.
    cv::cvtColor(this->frame, this->grayed, cv::COLOR_RGB2GRAY);

    // Resize image data.
    cv::resize(this->grayed, this->resized, cv::Size(this->width, this->height), 0, 0, cv::INTER_NEAREST);

    // Copy image data into our QImage.
    memcpy(ret.bits(), this->resized.data, this->width * this->height);

    return ret;
}
