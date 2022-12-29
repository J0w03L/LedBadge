#include "log.h"

QTextBrowser* logTextBrowser;
void registerLogTextBrowser(QTextBrowser* _)
{
    logTextBrowser = _;
    return;
}

/*
 * This is just a quick and dirty way to write output to logTextBrowser.
*/
void logf(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    char logText[2048];
    vsprintf(logText, fmt, args);
    logTextBrowser->append(logText);
    va_end(args);
    return;
}

/*
 * This just does the same as logf, but outputs to both stdout and logTextBrowser.
*/
void plogf(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    char logText[2048];
    vsprintf(logText, fmt, args);
    printf("%s\n", logText);
    logTextBrowser->append(logText);
    va_end(args);
    return;
}

void clog()
{
    logTextBrowser->clear();
}
