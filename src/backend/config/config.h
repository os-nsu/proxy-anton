 /*!
    \file config.h
    \brief This file contains CAT and config parser interface
    \version 1.0
    \date 15 Jul 2024

    There are structures unions enums and function's definitions that 
    describes user interface to create, change, follow, unite and separate
    groups of CAT (Configuration Access Table) parameters. There are also
    interface's functions needed for work with config file .conf

    Some information about config.conf:

    There are only string in format "key=value"

    key - string that consist only of english letters and symbols '_'.

    value could be single or array (array denoted by '{', '}', values in array separated
    by commas). Value could have one of three types: long long, double or string.
    
    sample.conf:
    max_size=42
    min_level=0.054
    the_best_subjects={"math", "programming"}

    IDENTIFICATION
        src/backend/config/condig.h
*/


/*
    DEFENITION  TYPES  AND FUNCTIONS OF CAT (Configuration Access Table) SYSTEM
*/

/*!
    \union Value
    \brief unified format of appearence in CAT

    Each parameter could have one o three types: long long, double or char *\n
    Use this union in work with CAT.
*/
union Value{
    long long int lngf; 
    double dblf; 
    char *strf; 
};

/*!
    \enum ParameterType
    \brief enum that definetype of configuration parameter

    Each configuration parameter is array of (long long / double / string) elements.
*/
enum ParameterType{
    T_LONG = 1, 
    T_DOUBLE, 
    T_STRING 
};


/*!
    \brief initialize CAT
    Use this function at the beginning of session to allocate memory for CAT.
*/
int initCAT();


/*!
    \brief Creates new configuration parameter
    Creates new configuration parameter in set group or create new group
    \param[in] groupName Name of parameter's group
    \param[in] name Name of parameter
    \param[in] type Parater's type
    \param[in] count Count of parameter's values (each parameter is array)
    \param[in] values Boot values of parameter
    \param[out] follower Pointer to pointer which will follows parameter
    \param[in] description Parameter's description
    \return 0 if success, -1 and sets errno else
*/
int createCATParameter(char *groupName, char *name, enum ParameterType type, int count, union Value *values,
                       union Value **follower, char *description);


/*!
    \brief delete configuration parameter
    Checks if parameter has no followers when delete parameter.
    If group or parameter doesn't exist when success result.
    \param[in] groupName Name of group
    \param[in]
*/
int deleteCATParameter(char *groupName, char *name);


/*!
    \brief sets group's block mode
    Checks if group exists then change block mode.
    In active block mode group couldn't be extended or reduced.
    \param[in] group Group's name
    \param[in] blockMode New state of block mode
    \return 0 if success, -1 and sets errno else
*/
int setGroupBlockMode(char *group, int blockMode);


/*!
    \brief gets parameter's description
    \param[in] group Group's name
    \param[in] name Parameter's name
    \return cstring (description) if success , NULL and sets errno else
*/
char *getCATParamDescr(char *group, char *name);


/*!
    \brief updates parameter's value
    Updates parameter's value and updates followers.
    \param[in] group Group's name
    \param[in] name Parameter's name
    \param[in] count New count of elements (each CAT parameter is array)
    \param[in] values New values of parameter
    \return 0 if success, -1 and sets errno else
*/
int updateCATParameter(char *group, char *name, int count, union Value *values);



/*!
    \brief adds new follower to parameter
    \param[in] group Group's name
    \param[in] name Parameter's name
    \param[in] follower pointer to pointer which will follow CAT parameter
    \return 0 if success, -1 and sets errno else
*/
int addFollowerToCAT(char *group, char *name, union Value **follower);


/*!
    \brief removes follower from parameter
    Removes follower and change it value to NULL
    \param[in] group Group's name
    \param[in] name Parameter's name
    \param[in] follower pointer to pointer which will be deleted from followers
    \return 0 if success, -1 and sets errno else
*/
int removeFollowerFromCAT(char *group, char *name, union Value **follower);