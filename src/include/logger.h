/*!
    \file logger.h
    \brief Logger library interface
    \version 1.0
    \date 13 Jul 2024
    
    These functions provide methods for oppenening logging session
    and log into choosed file

    IDENTIFICATION
        src/include/logger.h
*/

#ifndef LOGGER_H
#define LOGGER_H

/*!
    \enum LogLevel
    \brief Contains logging levels

    Log levels are informational only.  They do not affect program flow.
*/
enum LogLevel {
    LOG_DEBUG = 1, ///< low level messages
    LOG_INFO, ///< traces and meta information (could be substitution for stderr)
    LOG_WARNING, ///< messages that describe possibly incorrect behavior with saving invariant
    LOG_ERROR ///< messages about system errors
};

/*!
    \enum LogPart
    \brief Contains types of log parts

    Log part are informational only.  They do not affect program flow.
*/
enum LogPart {
    LOG_PRIMARY = 0, ///< Use one-string message
    LOG_DETAIL, ///< There you can describe circumstances
    LOG_HINT ///< Hint (not guaranteed correct) about how to fix the problem
};


/*!
    It intializes logger data (path to current log file). So you could init logger only when no one session is openned
    \param[in] path Path to log file
    \return 0 if success, -1 and sets errno if error
*/
int initLogger(char* path);

/*!
    It opens file connection with log file.
    \return 0 if success, -1 and sets errno if error
*/
int openLogSession();

/*!
    It closes file connection with log file.
    Use this function before next (but not first) logger initialization.
    \return 0 if success, -1 and sets errno if error
*/
int closeLogSession();

/*!
    It closes file connection with log file and free other meta data about session.
    \return 0 if success, -1 and sets errno if error
*/
int destructLogger();

/// @brief Checks if now session is openned
/// @return 1 if session is openned, 0 else
int checkLogSession();

/*!
    It log message(format string with variadic list of arguments) into log file.
    It requires openned session.
    \param[in] lvl Level of sended log
    \param[in] part Part of the log (needed for custom log reports)
    \param[in] format Message
    \param[in] other variadic list of arguments for message string
    \return 0 if success, -1 and sets errno if error
*/
int logMsg(enum LogLevel lvl, enum LogPart part, char *format, ...);

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
int logReport(enum LogLevel lvl, char *primary, char *detail, char *hint, ...);

#endif    // LOGGER_H