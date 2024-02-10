#ifndef QUEUE_H
#define QUEUE_H

#include <stdio.h>
#include <inttypes.h>
#include <thread>

#define QUEUE_MAX 256

class BadgeQueue
{
    public:
        void setQueueFPS(int fps);
        int getQueueFPS();
        void addBufferToQueue(uint8_t* buf);
        void addTextToQueue(char* text);
        void countQueue();
        void pauseQueue();
        void resumeQueue();
        void clearQueue();
        void freeQueue();
    private:
        uint8_t queueBuffers[QUEUE_MAX][144] = {0};
        uint8_t textBuffer[288] = {0};
        void shiftText(int offset);
        void drawFromQueue();
};

#endif // QUEUE_H
