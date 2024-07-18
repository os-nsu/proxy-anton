/*!
    \file parser.c
    \brief This file contains config parser implementation
    \version 1.0
    \date 15 Jul 2024

    Whole kernel configuration parameters belong to "kernel" group

    some information about config.conf:

    There are only string in format "key=value"

    key - string that consist only of english letters and symbols '_'.
    There are some command-keys (command-keys ended with '.'):
    group. - change group of parameters

    value could be single or array (array denoted by '{', '}', values in array separated
    by commas). Value could have one of three types: long long, double or string.
    
    There are 
    
    sample.conf:

    max_size=42
    min_level=0.054 #comment-part
    group. = "system" #change group to "system"
    the_best_subjects={"math", "programming"}
    #that part also commented

    IDENTIFICATION
        src/backend/config/parser.c
*/

#include "../../include/config.h"
#include "../../include/logger.h"
#include <complex.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>


/*PARSER IMPLEMENTATION*/

/*FUNCTION ADVERTISEMENTS*/
int parseLine(char *line, char **rkey, union Value **rvalues, int *rsize, enum ParameterType *rtype);
int parseConfig(char* path);


/*!
    \brief Parses 1 line from config

    Takes one line from config and parse it. line could be empty or commented.
    Parsing implemented with finite state machine. Rules for line syntax
    described in config.h description.

    \param[in] line Line of config that will be parsed
    \param[out] rkey Pointer to result key
    \param[out] rvalues Pointer to result array of values
    \param[out] rsize Pointer to result size of array
    \param[out] rtype Pointer to result type
    \return 0 if took key and value, -1 in error case, 2 in comment case, 3 if line is empty 
*/
int parseLine(char *line, char **rkey, union Value **rvalues, int *rsize, enum ParameterType *rtype) {
    char *curSymPtr = line;
    
    typedef enum ParsingStates {
        comment = -3,
        error = -2,
        finish = -1,
        start = 0,
        key,
        spacesAfterKey,
        spacesBeforeValues,
        valueDigit,
        valueDouble,
        valueString,
        valueStringSlash,
        valueArray,
        nextString,
        nextStringSlash,
        spacesAfterArraysString,
        spacesBeforeArraysString,
        nextDigit,
        spacesAfterArraysLong,
        spacesBeforeArraysLong,
        nextExpectedLong,
        nextDouble,
        spacesAfterArraysDouble,
        spacesBeforeArraysDouble,
        nextExpectedDouble,
        nextExactlyDouble
    } States;

    
    States curState = start;

    int beginKeyPos, endKeyPos;
    char *keyName = NULL;
    int beginCurValPos, endCurValPos;
    enum ParameterType type = 0;
    int cntSlash = 0;
    union Value *values = calloc(1, sizeof(union Value));
    int size = 1;
    values[0].lngf = 1;
    int countValues = 0;
    int curPos = 0;
    int breakLoop = 0;
    /*USES FINITE STATE MACHINE TO PARSE LINE*/
    while (*curSymPtr) {
        char sym = *curSymPtr;
        switch (curState) {
        case comment: {
            breakLoop = 3;
            break;
        }
        case error: {
            breakLoop = 2;
            break;
        }
        case finish: {
            breakLoop = 1;
            break;
        }
        case start: {
            if ((sym < 'A' && sym != ' ' && sym != '#') || (sym > 'Z' && sym < 'a' && sym != '_') || sym > 'z') {
                curState = error;
            } else if (sym == '#') {
                curState = comment;
            } else if ((sym >= 'A' && sym <= 'Z') || sym == '_' || (sym >= 'a' && sym <= 'z')) {
                beginKeyPos = curPos;
                curState = key;
            }
            break;
        }
        case key: {
            if ((sym < 'A' && sym != ' ' && sym != '.' && sym != '=') || (sym > 'Z' && sym < 'a' && sym != '_') || sym > 'z') {
                curState = error;
            } else if (sym == '.') {
                endKeyPos = curPos + 1;
                keyName = (char *)calloc((endKeyPos - beginKeyPos + 1), sizeof(char));
                for (int i = beginKeyPos; i < endKeyPos; ++i) {
                    keyName[i-beginKeyPos] = line[i];
                }
            } else if (sym == ' ') {
                endKeyPos = curPos;
                keyName = (char *)calloc((endKeyPos - beginKeyPos + 1), sizeof(char));
                for (int i = beginKeyPos; i < endKeyPos; ++i) {
                    keyName[i-beginKeyPos] = line[i];
                }
                curState = spacesAfterKey;
            } else if (sym == '=') {
                endKeyPos = curPos;
                keyName = (char *)calloc((endKeyPos - beginKeyPos + 1), sizeof(char));
                for (int i = beginKeyPos; i < endKeyPos; ++i) {
                    keyName[i-beginKeyPos] = line[i];
                }
                curState = spacesBeforeValues;
            }
            break;
        }
        case spacesAfterKey: {
            if (sym != ' ' && sym != '=') {
                curState = error;
            } else if (sym == '=') {
                curState = spacesBeforeValues;
            }
            break;
        }
        case spacesBeforeValues: {
            if (!((sym >= '0' && sym <= '9') || sym == '"' || sym == '{' || sym == ' ')) {
                curState = error;
            } else if (sym != ' ') {
                if (sym >= '0' && sym <= '9') {
                    beginCurValPos = curPos;
                    curState = valueDigit;
                } else if (sym == '"') {
                    beginCurValPos = curPos + 1;
                    cntSlash = 0;
                    curState = valueString;
                } else {
                    curState = valueArray;
                }
            }
            break;
        }
        case valueDigit: {
            if (!((sym >= '0' && sym <= '9') || sym == '.' || sym == ' ')) {
                curState = error;
            } else if (sym == '.') {
                if (*(curSymPtr + 1) < '0' || *(curSymPtr+1) > '9') {
                    curState = error;
                } else {
                    curState = valueDouble;
                }
            } else {
                endCurValPos = curPos;
                type = T_LONG;
                /*DELETE LEFT NULLS*/
                while (line[beginCurValPos] == '0' && beginCurValPos < endCurValPos - 1){
                    beginCurValPos++;
                }
                values[countValues].lngf = strtoll(line + beginCurValPos, NULL, 10);
                countValues++;
                curState = finish;
            }
            break;
        }
        case valueDouble: {
            if (!((sym >= '0' && sym <= '9') || sym == ' ')) {
                curState = error;
            } else if (sym == ' ') {
                type = T_DOUBLE;
                endCurValPos = curPos;
                values[countValues].dblf = strtod(line + beginCurValPos, NULL);
                countValues++;
                curState = finish;
            }
            break;
        }
        case valueString: {
            if (sym == '"') {
                type = T_STRING;
                endCurValPos = curPos;
                char *stringValue = (char *)calloc((endCurValPos - beginCurValPos + 1 - cntSlash), sizeof(char));
                /*COPY STRING WITHOUT SLASHES*/
                int cntSym = 0;
                for (int i = 0; i < endCurValPos - beginCurValPos; ++i) {
                    if (line[beginCurValPos + i] != '\\') {
                        stringValue[cntSym] = line[beginCurValPos + i];
                    } else {
                        stringValue[cntSym] = line[beginCurValPos + ++i]; // get next symbol after '\'
                    }
                    cntSym++;
                }
                values[countValues].strf = stringValue;
                countValues++;
                curState = finish;
            } else if (sym == '\\') {
                curState = valueStringSlash;
            }
            break;
        }
        case valueStringSlash: {
            cntSlash++;
            curState = valueString;
            break;
        }
        case valueArray: {
            if (sym != ' ' && sym != '\"' && !(sym >= '0' && sym <= '9')){
                curState = error;
            } else if (sym >= '0' && sym <= '9') {
                beginCurValPos = curPos;
                curState = nextDigit;
            } else if (sym == '\"'){
                beginCurValPos = curPos + 1;
                cntSlash = 0;
                curState = nextString;
            }
            break;
        }
        case nextString: {
            if (sym == '"') {
                type = T_STRING;
                endCurValPos = curPos;
                char *stringValue = (char *)calloc((endCurValPos - beginCurValPos + 1 - cntSlash), sizeof(char));
                /*COPY STRING WITHOUT SLASHES*/
                int cntSym = 0;
                for (int i = 0; i < endCurValPos - beginCurValPos; ++i) {
                    if (line[beginCurValPos + i] != '\\') {
                        stringValue[cntSym] = line[beginCurValPos + i];
                    } else {
                        stringValue[cntSym] = line[beginCurValPos + ++i]; // get next symbol after '\'
                    }
                    cntSym++;
                }
                /*CHECK CAPACITY OF VALUES ARRAY*/
                if (size <= countValues ) {
                    values = (union Value *)realloc(values, sizeof(union Value) * (size * 2 + 1));
                    size *= 2;
                }
                values[countValues].strf = stringValue;
                countValues++;
                curState = spacesAfterArraysString;
            } else if (sym == '\\') {
                curState = nextStringSlash;
            }
            break;
        }
        case nextStringSlash: {
            cntSlash++;
            curState = nextString;
            break;
        }
        case spacesAfterArraysString: {
            if(sym != ' ' && sym != ',' && sym != '}') {
                curState = error;
            } else if (sym == ',') {
                curState = spacesBeforeArraysString;
            } else if (sym == '}') {
                curState = finish;
            }
            break;
        }
        case spacesBeforeArraysString: {
            if(sym != ' ' && sym != '\"') {
                curState = error;
            } else if (sym == '\"') {
                beginCurValPos = curPos + 1;
                cntSlash = 0;
                curState = nextString;
            }
            break;
        }
        case nextDigit: {
            if (!(sym >= '0' && sym <= '9') && sym != '.' && sym != ' ' && sym != '}' && sym != ',') {
                curState = error;
            } else if (sym == '.') {
                if (*(curSymPtr + 1) < '0' || *(curSymPtr+1) > '9') {
                    curState = error;
                } else {
                    curState = nextDouble;
                }
            } else if (sym == ' ' || sym == '}' || sym == ',') {
                type = T_LONG;
                endCurValPos = curPos;
                /*DELETE LEFT NULLS*/
                while (line[beginCurValPos] == '0' && beginCurValPos < endCurValPos - 1){
                    beginCurValPos++;
                }
                /*CHECK CAPACITY OF VALUES ARRAY*/
                if (size <= countValues ) {
                    values = (union Value *)realloc(values, sizeof(union Value) * (size * 2));
                    size *= 2;
                }
                values[countValues].lngf = strtoll(line + beginCurValPos, NULL, 10);
                countValues++;
                if (sym == ' ') {
                    curState = spacesAfterArraysLong;
                } else if (sym == '}') {
                    curState = finish;
                } else if (sym == ',') {
                    curState = spacesBeforeArraysLong;
                }
            } 
            break;
        }
        case spacesAfterArraysLong: {
            if (sym != ' ' && sym != ',' && sym != '}') {
                curState = error;
            } else if (sym == ',') {
                curState = spacesBeforeArraysLong;
            } else if (sym == '}') {
                curState = finish;
            }
            break;
        }
        case spacesBeforeArraysLong: {
            if (sym != ' ' && !(sym >= '0' && sym <= '9')) {
                curState = error;
            } else if (sym >= '0' && sym <= '9') {
                beginCurValPos = curPos;
                curState = nextExpectedLong;
            }
            break;
        }
        case nextExpectedLong: {
            if (!(sym >= '0' && sym <= '9') && sym != ' ' && sym != '}' && sym != ',') {
                curState = error;
            } else if (sym == ' ' || sym == '}' || sym == ',') {
                endCurValPos = curPos;
                /*DELETE LEFT NULLS*/
                while (line[beginCurValPos] == '0' && beginCurValPos < endCurValPos - 1){
                    beginCurValPos++;
                }
                /*CHECK CAPACITY OF VALUES ARRAY*/
                if (size <= countValues ) {
                    values = (union Value *)realloc(values, sizeof(union Value) * (size * 2));
                    size *= 2;
                }
                values[countValues].lngf = strtoll(line + beginCurValPos, NULL, 10);
                countValues++;
                if (sym == ' ') {
                    curState = spacesAfterArraysLong;
                } else if (sym == ','){
                    curState = spacesBeforeArraysLong;
                } else {
                    curState = finish;
                }
            } 
            break;
        }
        case nextDouble: {
            if (!((sym >= '0' && sym <= '9') || sym == ' '  || sym == '}' || sym == ',')) {
                curState = error;
            } else if (sym == ' ' || sym == '}' || sym == ',') {
                type = T_DOUBLE;
                endCurValPos = curPos;
                /*CHECK CAPACITY OF VALUES ARRAY*/
                if (size <= countValues ) {
                    values = (union Value *)realloc(values, sizeof(union Value) * (size * 2));
                    size *= 2;
                }
                values[countValues].dblf = strtod(line + beginCurValPos, NULL);
                countValues++;
                if (sym == ' '){
                    curState = spacesAfterArraysDouble;
                } else if (sym == ',') {
                    curState = spacesBeforeArraysDouble;
                } else if (sym == '}') {
                    curState = finish;
                }
            }
            break;
        }
        case spacesAfterArraysDouble: {
            if (sym != ' ' && sym != ',' && sym != '}') {
                curState = error;
            } else if (sym == ',') {
                curState = spacesBeforeArraysDouble;
            } else if (sym == '}') {
                curState = finish;
            }
            break;
        }
        case spacesBeforeArraysDouble: {
            if (sym != ' ' && !(sym >= '0' && sym <= '9')) {
                curState = error;
            } else if (sym >= '0' && sym <= '9') {
                beginCurValPos = curPos;
                curState = nextExpectedDouble;
            }
            break;
        }
        case nextExpectedDouble: {
            if (!(sym >= '0' && sym <= '9') && sym != '.') {
                curState = error;
            } else if (sym == '.') {
                if (*(curSymPtr + 1) < '0' || *(curSymPtr+1) > '9') {
                    curState = error;
                } else {
                    curState = nextExactlyDouble;
                }
            }
            break;
        }
        case nextExactlyDouble: {
            if (!((sym >= '0' && sym <= '9') || sym == ' '  || sym == '}' || sym == ',')) {
                curState = error;
            } else if (sym == ' ' || sym == '}' || sym == ',') {
                endCurValPos = curPos;
                /*CHECK CAPACITY OF VALUES ARRAY*/
                if (size <= countValues ) {
                    values = (union Value *)realloc(values, sizeof(union Value) * (size * 2));
                    size *= 2;
                }
                values[countValues].dblf = strtod(line + beginCurValPos, NULL);
                countValues++;
                if (sym == ' '){
                    curState = spacesAfterArraysDouble;
                } else if (sym == ',') {
                    curState = spacesBeforeArraysDouble;
                } else if (sym == '}') {
                    curState = finish;
                }
            }
            break;
        }
        default:
            curState = error;
            break;
        }

        if (breakLoop) {
            break;
        }
        curPos++;
        curSymPtr++;
    }
    //CHECK IF BROKE AND DIDN'T WROTE VALUE
    if(curState == valueDigit) {
        endCurValPos = curPos;
        /*DELETE LEFT NULLS*/
        while (line[beginCurValPos] == '0' && beginCurValPos < endCurValPos - 1){
            beginCurValPos++;
        }
        values[countValues].lngf = strtoll(line + beginCurValPos, NULL, 10);
        countValues++;
        curState = finish;
        breakLoop = 1;
    } else if (curState == valueDouble) {
        endCurValPos = curPos;
        values[countValues].dblf = strtod(line + beginCurValPos, NULL);
        countValues++;
        curState = finish;
        breakLoop = 1;
    } else if (curState == start) { //CHECK THAT LINE IS EMPTY
        breakLoop = -1;
    } else if (curState == finish) {
        breakLoop = 1;
    } else if (curState == error) {
        breakLoop = 2;
    } else if (curState == comment) {
        breakLoop = 3;
    }

    //RETURN RESULTS

    if (breakLoop == 1) {
        *rkey = keyName;
        *rvalues = values;
        *rsize = countValues;
        *rtype = type;
        return 0;
    } else if (breakLoop == 3) {
        *rkey = NULL;
        return 2;
    } else if (breakLoop == -1) {
        *rkey = NULL;
        return 3;
    }

    /*PROCESS ERROR CASE*/
    if (keyName) {
        free(keyName);
    }
    if (type == T_STRING) {
        for (int i = 0; i < countValues; ++i) {
            free(values[i].strf);
        }
    }
    free(values);
    *rkey = NULL;
    return -1;
}

/*!
    \brief Parses config file
    
    Rules for config file described in config.h description. If logger is active,
    parser will write in log.
    \param[in] path Path to configuration file
    \return 0 if success, -1 and sets errno else
*/
int parseConfig(char* path) {
    if (!path) {
        return -1;
    }
    
    char *line = NULL;
    size_t len = 0;

    FILE *config = fopen(path, "r");
    
    if (!config) {
        return -1;
    }

    char *curGroup = (char*)calloc(7, sizeof(char));
    strcpy(curGroup,"kernel"); //"kernel" - default group name in config
    int check = 0; // check result of getline
    int cntLine = 0;
    while (!feof(config) && (check = getline(&line, &len, config))) {
        cntLine++;
        int size = 0;
        union Value *values = NULL;
        char *key = NULL;
        enum ParameterType type = 0;
        int parseResult = 0;
        if((parseResult = parseLine(line, &key, &values, &size, &type))) { 
            if(parseResult == -1 ) {
                logReport(LOG_ERROR, "Config file error", "In line %d", "Read syntax in config.h", cntLine);
                free(line);
                return -1;       
            }
            /*ELSE WE SKIP COMMENT OR EMPTY STRING*/
            free(line);
            continue;
        }
        free(line);
        /*CHECK IF COMMAND*/
        if(!strcmp(key,"group.")) {
            if(type != T_STRING || size != 1){
                return -1;
            }
            free(curGroup);
            curGroup = values[0].strf;
            free(key);
            continue;
        }
        /*TRY TO ADD NEW PARAMETER*/
        int createRes = 0;
        if((createRes = createCATParameter(curGroup, key, type, size, values, NULL, NULL))) {
            return -1;
        }
    }

    free(curGroup);
    return 0;
}

