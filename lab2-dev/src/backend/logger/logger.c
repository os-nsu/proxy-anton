/*!
    \file logger.c
    \brief Logger library source code
    \version 1.0
    \date 24 Oct 2024
    
    These functions provide methods for oppenening logging session
    and log into choosed file (path is set in LoggerData struct by initLogger function)

    IDENTIFICATION
        src/backend/logger/logger.c
*/
#include "../../include/logger.h"
#include "../../include/config.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>

/*STRUCT ADVERTISEMENTS*/
struct LoggerData;

/*FUNCTION ADVERTISEMENTS*/
void incrementFileNum(void);
char *mkLogPath(const char *mainDir, const char *name);
int initLogger(void);
int openLogSession(void);
int closeLogSession(void);
int destructLogger(void);
int checkLogSession(void);
int elog(enum LogLevel lvl, char *format, ...);
int logMsg(enum LogLevel lvl, enum LogPart part, char *format, ...);
int logReport(enum LogLevel lvl, char *primary, char *detail, char *hint, ...);

/*!
    \struct LoggerData
    \brief struct that describes info needed for logging session
*/
struct LoggerData {
    unsigned int fileNum;
    FILE *session;
    int isSessionOpen; 
};


struct LoggerData mainLogger = {0,NULL, 0}; ///< Main and only one logger



void incrementFileNum(void) {
    mainLogger.fileNum++;
}

/*!
    \brief make loggerPath using mainDir
    \param[in] mainDir path to main proxy directory
    \return loggerPath if success, NULL else
*/
char *mkLogPath(const char *mainDir, const char *name) {
    printf("hey\n"); //delete
    if (!mainDir) {
        return NULL;
    }
    
    /*DEFINE LOG FILE NAME*/
    char *logFileName;
    char *numberBuff = (char *)calloc(1024, sizeof(char));
    sprintf(numberBuff,"%d",mainLogger.fileNum);
    const char *c = mainDir;
    while (*c) {
        c++;
    }
    
    if (name == NULL) {
        if (*(--c) == '/') {
            logFileName = (char *)calloc(strlen(numberBuff) + strlen("proxy") + strlen(".log") + 1, sizeof(char));
            logFileName = strcat(strcat(strcat(logFileName, "proxy"),numberBuff),".log");
            printf("lfn is %s\n",logFileName); //delete
        } else {
            logFileName = (char *)calloc(strlen(numberBuff) + strlen("/proxy") + strlen(".log") + 1, sizeof(char));
            logFileName = strcat(strcat(strcat(logFileName, "/proxy"),numberBuff),".log");
            printf("lfp is %s\n",logFileName); //delete
        }
    } else {
        logFileName = (char *)name;
        printf("lfd is %s\n",logFileName); //delete
    }
    
    
    char *loggerPath = (char *)calloc(strlen(mainDir) + strlen(logFileName) + 1, sizeof(char));
    strcat(strcat(loggerPath, mainDir), logFileName);
    free(numberBuff);
    if (name == NULL) {
        free(logFileName);
    }
    return loggerPath;
}

int makeDir(char *directoryPath) { return mkdir(directoryPath, 0777); }



/*!
    It intializes logger data (path to current log file). So you could init logger only when no one session is openned
    \return 0 if success, -1  and sets errno else
*/
int initLogger(void){

    CATFollower logsDirFollower;
    
    if (addFollowerToCAT("kernel", "log_dir", &logsDirFollower)) {
        const char *logsDir = "./logs";
        createCATParameter("kernel", "log_dir", T_STRING, 1, (union Value *)(&logsDir), &logsDirFollower, "path to directory with log files");
    } else if (!logsDirFollower.data->strf) {
        const char *logsDir = "./logs";
        updateCATParameter("kernel", "log_dir", 1, (union Value *)(&logsDir));
    }
    printf("%s\n",logsDirFollower.data->strf);
    if(mainLogger.isSessionOpen){
        return -1;
    }
    
    if (access(logsDirFollower.data->strf, 00) != 0 && makeDir(logsDirFollower.data->strf)) {
        removeFollowerFromCAT("kernel", "log_dir", &logsDirFollower);    
        return -1;
    }

    removeFollowerFromCAT("kernel", "log_dir", &logsDirFollower);
    return 0;
}

/*!
    It opens file connection with log file.
    \return 0 if success, -1  and sets errno else
*/
int openLogSession(void) {

    CATFollower logsDirFollower;
    if (addFollowerToCAT("kernel", "log_dir", &logsDirFollower)) {
        return -1;
    } else if (!logsDirFollower.data->strf) {
        removeFollowerFromCAT("kernel", "log_dir", &logsDirFollower);
        return -1;
    }
    

    char *logFilePath = mkLogPath(logsDirFollower.data->strf, NULL);
    printf("lfpp: %s\n", logFilePath); //delete
    if (logFilePath == NULL) {
        removeFollowerFromCAT("kernel", "log_dir", &logsDirFollower);
        return -1;
    } else if (mainLogger.isSessionOpen == 1 || mainLogger.session != NULL){
        free(logFilePath);
        removeFollowerFromCAT("kernel", "log_dir", &logsDirFollower);
        return -1;
    }
    
    mainLogger.session = fopen(logFilePath, "a");

    if(mainLogger.session == NULL) {
        removeFollowerFromCAT("kernel", "log_dir", &logsDirFollower);
        free(logFilePath);
        return -1;
    }

    mainLogger.isSessionOpen = 1;
    free(logFilePath);
    removeFollowerFromCAT("kernel", "log_dir", &logsDirFollower);
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


    return 0;
}



int checkLogSession(void){
    return mainLogger.isSessionOpen;
}


long getFileLength() {
    if(!mainLogger.isSessionOpen) {
        errno = EACCES;
        return -1;
    }
    
    long curPos = ftell(mainLogger.session);

    if (fseek(mainLogger.session, 0L, SEEK_END) != 0) {
        perror("Error seeking to end of file");
        fclose(mainLogger.session);
        return -1;
    }

    long size = ftell(mainLogger.session);
    if (size == -1) {
        perror("Error getting file size");
        fclose(mainLogger.session);
        return -1;
    }

    if (fseek(mainLogger.session, curPos, SEEK_SET) != 0) {
        perror("Error seeking to end of file");
        fclose(mainLogger.session);
        return -1;
    }

    return size;
}


int separateFile(long begin) {
    closeLogSession();


    FILE *secFile = NULL, *tempFile = NULL,*file = NULL;
    char *lastLogFilePath = NULL, *newLogFilePath = NULL, *tmpLogFilePath = NULL;

    CATFollower logsDirFollower;
    if (addFollowerToCAT("kernel", "log_dir", &logsDirFollower)) {
        return -1;
    } else if (!logsDirFollower.data->strf) {
        removeFollowerFromCAT("kernel", "log_dir", &logsDirFollower);
        return -1;
    }
    lastLogFilePath = mkLogPath(logsDirFollower.data->strf, NULL);
    incrementFileNum();
    newLogFilePath = mkLogPath(logsDirFollower.data->strf, NULL);
    tmpLogFilePath = mkLogPath(logsDirFollower.data->strf, "tmpLog.log");

    

    file = fopen(lastLogFilePath, "r+b");
    if (file == NULL) {
        perror("Error opening file");
        goto errorLabel;
    }

    if (fseek(file, begin, SEEK_SET) != 0) {
        perror("Error seeking to start position");
        fclose(file);
        file = NULL;
        goto errorLabel;
    }

    tempFile = fopen(tmpLogFilePath, "wb");
    if (tempFile == NULL) {
        perror("Error creating temporary file");
        goto errorLabel;
    }

    char buffer[1024];
    long firstFileSize = begin;
    size_t bytesRead;
    while (firstFileSize > 0 && (bytesRead = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        firstFileSize -= bytesRead;
        if (firstFileSize >= 0) {
            fwrite(buffer, 1, bytesRead, tempFile);
        } else {
            fwrite(buffer, 1, bytesRead + firstFileSize, tempFile);
        }
    }
    fclose(tempFile);
    tempFile = NULL;

    if (fseek(file, begin, SEEK_SET) != 0) {
        perror("Error seeking to start position");
        goto errorLabel;
    }

    secFile = fopen(newLogFilePath, "wb");
    if (tempFile == NULL) {
        perror("Error creating temporary file");
        goto errorLabel;
    }

    while ((bytesRead = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        fwrite(buffer, 1, bytesRead, secFile);
    }
    fclose(secFile);
    secFile = NULL;
    fclose(file);
    file = NULL;

    if (rename(tmpLogFilePath, lastLogFilePath) != 0) {
        perror("Error renaming temporary file");
        goto errorLabel;
    }


    free(lastLogFilePath);
    free(newLogFilePath);
    free(tmpLogFilePath);
    removeFollowerFromCAT("kernel", "log_dir", &logsDirFollower);
    return 0;
errorLabel:
    fclose(file);
    fclose(secFile);
    fclose(tempFile);
    free(lastLogFilePath);
    free(newLogFilePath);
    free(tmpLogFilePath);
    removeFollowerFromCAT("kernel", "log_dir", &logsDirFollower);
    return -1;

}

/*!
    used in OOS labs
    It log message(format string with variadic list of arguments) into log file.
    It requires openned session.
    \param[in] lvl Level of sended log
    \param[in] format Message
    \param[in] other variadic list of arguments for message string
    \return 0 if success, -1 and sets errno
*/
int elog(enum LogLevel lvl, char *format, ...) {
    
    if (!mainLogger.isSessionOpen){
        openLogSession();
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
    case LOG_FATAL:
        msgLvl = "FATAL: ";
        break;
    default:
    errno = EBADMSG;
        return -1;
    }

    long beginMsg = ftell(mainLogger.session);
    time_t mytime = time(NULL);
    struct tm *now = localtime(&mytime);
    fprintf(mainLogger.session, "%d.%d.%d %d:%d:%d %s file:%s string:%d |",now->tm_mday, now->tm_mon + 1, now->tm_year + 1900, now->tm_hour, now->tm_min, now->tm_sec, msgLvl,__FILE__,__LINE__);
    va_list ptr;
    va_start(ptr, format);
    vfprintf(mainLogger.session, format, ptr);
    va_end(ptr);
    fprintf(mainLogger.session,"\n");
    fflush(mainLogger.session);


    
    long fileLength = getFileLength();
    printf("len is %ld\n", fileLength); //delete
    CATFollower fileLimitFlwr;
    if (!addFollowerToCAT("kernel", "log_file_size_limit", &fileLimitFlwr)) {
        printf("limit is%lld\n",fileLimitFlwr.data->lngf); //delete
        if (fileLimitFlwr.data->lngf < fileLength && 2*fileLimitFlwr.data->lngf >= fileLength ){
            closeLogSession();
            separateFile(beginMsg);
            openLogSession();
        } else if (fileLimitFlwr.data->lngf == fileLength) {
            closeLogSession();
            incrementFileNum();
            openLogSession();
        } else if (2*fileLimitFlwr.data->lngf < fileLength) {
            closeLogSession();
            removeFollowerFromCAT("kernel", "log_file_size_limit", &fileLimitFlwr);
            return -1;
        }
    }
    removeFollowerFromCAT("kernel", "log_file_size_limit", &fileLimitFlwr);

    return 0;
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
        openLogSession();
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
    case LOG_FATAL:
        msgLvl = "FATAL: ";
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
    case LOG_FATAL:
        msgLvl = "FATAL: ";
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
