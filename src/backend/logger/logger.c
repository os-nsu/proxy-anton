/*!
    \file logger.c
    \brief Logger library source code
    \version 1.0
    \date 13 Jul 2024
    
    These functions provide methods for oppenening logging session
    and log into choosed file (path is set in LoggerData struct by initLogger function)

    IDENTIFICATION
        src/backend/logger/logger.c
*/
#include "../../include/logger.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>

/*STRUCT ADVERTISEMENTS*/
struct LoggerData;

/*FUNCTION ADVERTISEMENTS*/
int initLogger(char* path);
int openLogSession(void);
int closeLogSession(void);
int destructLogger(void);
int checkLogSession(void);
int logMsg(enum LogLevel lvl, enum LogPart part, char *format, ...);
int logReport(enum LogLevel lvl, char *primary, char *detail, char *hint, ...);

/*!
    \struct LoggerData
    \brief struct that describes info needed for logging session
*/
struct LoggerData {
    char *logFilePath;
    FILE *session;
    int isSessionOpen; 
};


struct LoggerData mainLogger = {NULL, NULL, 0}; ///< Main and only one logger


/*!
    It intializes logger data (path to current log file). So you could init logger only when no one session is openned
    \param[in] path Path to log file
    \return 0 if success, -1  and sets errno else
*/
int initLogger(char* path){

    if (path == NULL){
        return -1;
    }
    if(mainLogger.isSessionOpen){
        return -1;
    }

    mainLogger.logFilePath = (char*)calloc(strlen(path) + 1, sizeof(char));

    strcpy(mainLogger.logFilePath, path);

    return 0;
}

/*!
    It opens file connection with log file.
    \return 0 if success, -1  and sets errno else
*/
int openLogSession(void) {
    
    if (mainLogger.logFilePath == NULL) {
        return -1;
    } else if (mainLogger.isSessionOpen == 1 || mainLogger.session != NULL){
        return -1;
    }

    mainLogger.session = fopen(mainLogger.logFilePath, "a");

    if(mainLogger.session == NULL) {
        return -1;
    }

    mainLogger.isSessionOpen = 1;

    return 0;
}

/*!
    It closes file connection with log file.
    Use this function before next (but not first) logger initialization.
    \return 0 if success, -1  and sets errno else
*/
int closeLogSession(void) {
    
    if (mainLogger.session == NULL) {
        errno = EACCES;
        return -1;
    }

    if (fclose(mainLogger.session) == EOF) {
        errno = EBADF;
        return -1;
    }

    mainLogger.session = NULL;
    mainLogger.isSessionOpen = 0;
    return 0;
}

/*!
    It closes file connection with log file and free other meta data about session.
    \return 0 if success, -1 and sets errno else
*/
int destructLogger(void) {

    if (mainLogger.session != NULL && !mainLogger.isSessionOpen){
        errno = EACCES;
        return -1;
    }

    if(closeLogSession() != 0){
        return -1;
    }

    if (mainLogger.logFilePath != NULL) {
        free(mainLogger.logFilePath);
    }

    return 0;
}



int checkLogSession(void){
    return mainLogger.isSessionOpen;
}


/*!
    It log message(format string with variadic list of arguments) into log file.
    It requires openned session.
    \param[in] lvl Level of sended log
    \param[in] part Part of the log (needed for custom log reports)
    \param[in] format Message
    \param[in] other variadic list of arguments for message string
    \return 0 if success, -1 and sets errno
*/
int logMsg(enum LogLevel lvl, enum LogPart part, char *format, ...) {
    
    if (!mainLogger.isSessionOpen){
        errno = EACCES;
        return -1;
    }

    const char * msgLvl;

    switch (lvl)
    {
    case LOG_DEBUG:
        msgLvl = "DEBUG: ";
        break;
    case LOG_INFO:
        msgLvl = "INFO: ";
        break;
    case LOG_WARNING:
        msgLvl = "WARNING: ";
        break;
    case LOG_ERROR:
        msgLvl = "ERROR: ";
        break;
    default:
    errno = EBADMSG;
        return -1;
    }

    const char *msgPart;

    switch (part)
    {
    case LOG_PRIMARY:
        msgPart = "PRIMARY:    ";
        break;
    case LOG_DETAIL:
        msgPart = "DETAIL:    ";
        break;
    case LOG_HINT:
        msgPart = "HINT:    ";
        break;
    default:
        errno = EBADMSG;
        return -1;
    }
    fprintf(mainLogger.session, "%s%s", msgLvl, msgPart);
    va_list ptr;
    va_start(ptr, format);
    vfprintf(mainLogger.session, format, ptr);
    va_end(ptr);
    fprintf(mainLogger.session,"\n");
    fflush(mainLogger.session);
    return 0;
}

/*!
    It log report into log file.
    It requires openned session.
    \param[in] lvl Level of sended log
    \param[in] primary primary part of message (set NULL for skip)
    \param[in] detail detail part of message (set NULL for skip)
    \param[in] hint hint part of message (set NULL for skip)
    \param[in] other variadic list of arguments for message strings Use order like arguments ((primary, detail, hint))
    \return 0 if success, -1 and sets errno if error
*/
int logReport(enum LogLevel lvl, char *primary, char *detail, char *hint, ...) {
    if (!mainLogger.isSessionOpen){
        errno = EACCES;
        return -1;
    }
    
    const char * msgLvl;

    switch (lvl)
    {
    case LOG_DEBUG:
        msgLvl = "DEBUG: ";
        break;
    case LOG_INFO:
        msgLvl = "INFO: ";
        break;
    case LOG_WARNING:
        msgLvl = "WARNING: ";
        break;
    case LOG_ERROR:
        msgLvl = "ERROR: ";
        break;
    default:
    errno = EBADMSG;
        return -1;
    }
    
    const char *primaryLvl = "";
    const char *primaryLbl = "";
    const char *firstLF = "";

    if (primary != NULL){
        primaryLvl = msgLvl;
        primaryLbl = "PRIMARY:    ";
        firstLF = "\n";
    } else {
        primary = "";
    }

    const char *detailLvl = "";
    const char *detailLbl = "";
    const char *secondLF = "";

    if (detail != NULL) {
        detailLvl = msgLvl;
        detailLbl =  "DETAIL:    ";
        secondLF = "\n";
    } else {
        detail = "";
    }

    const char *hintLvl = "";
    const char *hintLbl = "";
    const char *thirdLF = "";

    if (hint != NULL) {
        hintLvl = msgLvl;
        hintLbl = "HINT:    ";
        thirdLF = "\n";
    } else {
        hint = "";
    }

    ///< Allocate and fill report string 
    
    int reportLen = strlen(primaryLvl) + strlen(primaryLbl) + strlen(primary) + strlen(firstLF)\
        + strlen(detailLvl) + strlen(detailLbl) + strlen(detail) + strlen(secondLF)\
        + strlen(hintLvl) + strlen(hintLbl) + strlen(hint) + strlen(thirdLF) + 1;

    char *report = (char *)calloc(reportLen, sizeof(char));
    strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat\
        (report, primaryLvl), primaryLbl),primary),firstLF),\
        detailLvl), detailLbl), detail), secondLF),\
        hintLvl), hintLbl), hint), thirdLF);
    
    va_list ptr;
    va_start(ptr, hint);

    vfprintf(mainLogger.session, report, ptr);

    va_end(ptr);
    fflush(mainLogger.session);
    free(report);
    return 0;
}
