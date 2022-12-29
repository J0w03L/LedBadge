#ifndef LOG_H
#define LOG_H

#include <stdarg.h>
#include <stdio.h>
#include <QTextBrowser>

void registerLogTextBrowser(QTextBrowser* _);
void logf(const char* fmt, ...);
void pllogf(const char* fmt, ...);
void plogf(const char* fmt, ...);
void clog();

#endif // LOG_H
