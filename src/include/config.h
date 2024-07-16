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
    min_level=0.054 #comment-part
    group. = "system" #change group to "system"
    the_best_subjects={"math", "programming"}
    #that part also commented

    IDENTIFICATION
        src/include/config.h
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
enum ParameterType {
    T_LONG = 1, 
    T_DOUBLE, 
    T_STRING 
};

/*!
    \struct CATFollowerData
    \brief high-level representation for pointer to array of enum Value

    Parameter's array of values has size at first position. This struct
    solve this problem: it has 2 pointers: to size and to data.
*/
typedef struct CATFollowerData {
    long long int *size;
    union Value *data;
} CATFollower;


/*!
    \brief initialize CAT
    Use this function at the beginning of session to allocate memory for CAT.
*/
int initCAT();


/*!
    \brief Creates new configuration parameter
    Creates new configuration parameter in set group or create new group
    All parameters will be copied therefore don't forget to free arguments.
    \param[in] groupName Name of parameter's group
    \param[in] name Name of parameter
    \param[in] type Parater's type
    \param[in] count Count of parameter's values (each parameter is array)
    \param[in] values Boot values of parameter
    \param[out] follower Pointer to CATFollower which will follows parameter
    \param[in] description Parameter's description
    \return 0 if success, -1 and sets errno else
*/
int createCATParameter(const char *groupName, const char *name, enum ParameterType type, int count, const union Value *values,
                       CATFollower *follower, const char *description);


/*!
    \brief delete configuration parameter
    Checks if parameter has no followers when delete parameter.
    If group or parameter doesn't exist when success result.
    \param[in] groupName Name of group
    \param[in] name Parameter's name
*/
int deleteCATParameter(const char *groupName, const char *name);


/*!
    \brief sets group's block mode
    Checks if group exists then change block mode.
    In active block mode group couldn't be extended or reduced.
    \param[in] group Group's name
    \param[in] blockMode New state of block mode
    \return 0 if success, -1 and sets errno else
*/
int setGroupBlockMode(const char *group, int blockMode);


/*!
    \brief gets parameter's description
    \param[in] group Group's name
    \param[in] name Parameter's name
    \return cstring (description) if success , NULL and sets errno else
*/
char *getCATParamDescr(const char *group, const char *name);


/*!
    \brief updates parameter's value
    Updates parameter's value and updates followers.
    \param[in] group Group's name
    \param[in] name Parameter's name
    \param[in] count New count of elements (each CAT parameter is array)
    \param[in] values New values of parameter
    \return 0 if success, -1 and sets errno else
*/
int updateCATParameter(const char *group, const char *name, int count, const union Value *values);



/*!
    \brief adds new follower to parameter
    \param[in] group Group's name
    \param[in] name Parameter's name
    \param[in] follower pointer to CATFollower which will follow CAT parameter
    \return 0 if success, -1 and sets errno else
*/
int addFollowerToCAT(const char *group, const char *name, CATFollower *follower);


/*!
    \brief removes follower from parameter
    Removes follower and change it value to NULL
    \param[in] group Group's name
    \param[in] name Parameter's name
    \param[in] follower pointer to CATFolower which will be deleted from followers
    \return 0 if success, -1 and sets errno else
*/
int removeFollowerFromCAT(const char *group, const char *name, CATFollower *follower);