#ifndef LOG_H
#define LOG_H

#include <stdarg.h>
#include <stdio.h>
#include <QTextBrowser>

void logf(QTextBrowser* logTextBrowser, const char* fmt, ...);
void pllogf(QTextBrowser* logTextBrowser, const char* fmt, ...);
void plogf(QTextBrowser* logTextBrowser, const char* fmt, ...);
void clog(QTextBrowser* logTextBrowser);

#endif // LOG_H
