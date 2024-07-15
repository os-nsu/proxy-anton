/*!
    \file config.c
    \brief This file contains CAT and config parser implementation
    \version 1.0
    \date 15 Jul 2024

    There are structures unions enums and functions that defines kernel of CAT -
    Configuration Access Table. CAT allows create, change, follow, unite
    in groups plain of configuration parameters. There are also structures and
    functions needed for work with configuration file .conf.
    Whole kernel configuration parameters belong to "kernel" group

    ome information about config.conf:

    There are only string in format "key=value"

    key - string that consist only of english letters and symbols '_'.

    value could be single or array (array denoted by '{', '}', values in array separated
    by commas). Value could have one of three types: long long, double or string.
    BTW: each parameter in CAT is array. The first value in array is length of the array
    
    sample.conf:
    max_size=42
    min_level=0.054
    the_best_subjects={"math", "programming"}

    IDENTIFICATION
        src/backend/config/condig.c
*/

#include "config.h"
#include <complex.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

/*
    IMPLEMENTATION OF CAT (Configuration Access Table) SYSTEM
*/

/*!
    \struct Follower
    \brief follower of parameter value

    Followers allow system automatically update variables when
    parameter will be updated. Each parameter structure has
    linked list of followers. BTW system should check ParameterType
    to resolve addr ptr type.
*/
struct Follower {
    union Value **addr;
    struct Follower *next;  
};


/*!
    \struct ParameterData
    \brief Structure that describes configuration parameter in table

    Parameter consist of it's type, count of values (because each parameter
    in CAT is array), pointer to values, linked list of followers, and meta
    info like first and second hash keys and description
*/
typedef struct ParameterData {
    struct Follower* head; ///< head of followers list
    char *group; 
    char *name; ///< unique only in group
    char *description;
    union Value *values; ///< first element in array alwais is count of elements
    enum ParameterType type;
}  Parameter;


void freeParameter(Parameter *parameter) {
    free(parameter->group);
    free(parameter->name);
    free(parameter->description);
    free(parameter->values);
    struct Follower *curFollower, *delFollower;
    curFollower = parameter->head;
    while(curFollower) {
        delFollower = curFollower;
        curFollower = curFollower->next;
        free(delFollower);
    }
    free(parameter);
}

/*!
    \brief update parameter's values
    That is only one safe interface for updating parameter.
    Don't update parameter manually (because of multy thread)
    \param[in] param Pointer to parameter
    \param[in] newCount New count of values
    \param[in] values Values of parameter
    \return 0 if success, -1 and sets errno else
*/
int updateParameter(Parameter *param, int newCount, union Value *values) {
    
    if (!param || !values){
        return -1;
    }

    if(param->values[0].lngf == newCount) {
        for(int i = 1; i <= newCount; ++i) {
            param->values[i] = values[i - 1];
        }
    } else {
        free(param->values);
        param->values = (union Value *)malloc(sizeof(union Value) * (newCount + 1));
        param->values[0].lngf = newCount;
        for(int i = 1; i <= newCount; ++i) {
            param->values[i] = values[i - 1];
        }
        /*NOTIFY FOLLOWERS*/
        struct Follower *curFollower = param->head;
        while (curFollower) {
            *curFollower->addr = param->values;
        }
    }
    return 0;
}

/*!
    \brief adds new follower to parameter
    Adds pointer in followers list and update this pointer
    \param[in] param Pointer to parameter
    \param[out] followerVar Pointer to pointer which will follow parameter
    \return 0 if success, -1 else
*/
int addFollowerToParameter(Parameter *param, union Value **followerVar) {
    if(!param || !followerVar){
        return -1;
    }
    /*SET FOLLOWERVAR VALUE*/
    *followerVar = param->values;

    /*ADD FOLLOWER IN LIST*/
    struct Follower *follower = (struct Follower *)malloc(sizeof(struct Follower));
    follower->addr = followerVar;
    follower->next = NULL;
    if(param->head == NULL){
        param->head = follower;
    } else {
        struct Follower *curFollower = param->head;
        while (curFollower->next) {
            /*CHECK MULTIFOLLOW*/
            if(curFollower->addr == followerVar) {
                return -1;
            }
            curFollower = curFollower->next;
        }
        /*CHECK MULTIFOLLOW*/
        if(curFollower->addr == followerVar) {
                return -1;
        }
        curFollower->next = follower;
    }
    return 0;
}


/*!
    \brief removes follower from parameter's list

    Change value of follower to NULL and removes it from list.
    Success if removed from list or nothing to remove.
    \param[in] param Pointer to parameter
    \param[out] followerVar Follower pointer that should be removed
    \return 0 if success, -1 and sets errno else
*/
int removeFollowerFromParameter(Parameter *param, union Value **followerVar) {
    if(!param || !followerVar){
        return -1;
    }

    *followerVar = NULL;

    /*DELETE ELEMENT FROM ONE-DIRECTION LINKED LIST*/
    if(param->head == NULL){
        return 0;
    } else if(param->head->addr == followerVar){
        struct Follower *delFollower = param->head;
        param->head = param->head->next;
        free(delFollower);
    } else {
        struct Follower *curFollower = param->head->next;
        struct Follower *prevFollower = param->head;
        while(curFollower){
            if (curFollower->addr == followerVar) {
                prevFollower->next = curFollower->next;
                free(curFollower);
                break;
            }
            curFollower = curFollower->next;
            prevFollower = prevFollower->next;
        }
    }
    return 0;
}


/*!
    \brief get description of parameter
    \param[in] param Pointer to parameter
    \return cstring - copy of parameter's description
*/
char *getParamDescr(Parameter *param) {
    char *result = (char *)malloc(sizeof(char) * strlen(param->description));
    strcpy(result, param->description);
    return result;
}


/*!
    \brief hash string
    Now algorithm is sdbm because this was used
    in real data base and worked fine
    \param[in] str value
    \return hash that is unsigned long
*/
unsigned long hashString(char* str) {
    unsigned long hash = 0;
    int c;

    while ((c = *str++)) {
        hash = c + (hash << 6) + (hash << 16) - hash;
    }
    return hash;
}

/*!
    \struct ParameterNode
    \brief Service structure for hash table in group
*/
struct ParameterNode {
    Parameter *data;
    struct ParameterNode *next;
};

/*!
    \struct  ParametersGroupData
    \brief It contains hash table of parameters and flags

    Contains hash table of parameters. In block mode you couldn't delete or add
    new parameters in table, but you are able to change parameters
*/
typedef struct ParametersGroupData {
    char *name;
    struct ParameterNode **table;
    int size;
    int curCount; ///< current count of table elements
    int isBlocked;
} GroupParam;

void freeGroup(GroupParam *group) {
    free(group->name);
    free(group->table);
    free(group);
}

/*!
    \brief finds parameter in group by name
    \param[in] group Pointer to group of parameters
    \param[in] name Name of parameter
    \return pointer to parameter if success, NULL else
*/
Parameter *findParameterInGroup(GroupParam *group, char *name) {
    long int index = hashString(name) % group->size;
    struct ParameterNode *curParamNode = group->table[index];
    while(curParamNode) {
        if(!strcmp(curParamNode->data->name, name)){
            return curParamNode->data;
        }
        curParamNode = curParamNode->next;
    }
    return NULL;
}

/*!
    \brief add new parametr in group

    It checks ability for adding new parametr and add it. All parameters 
    will be copied therefore don't forget to free arguments.

    \param[in] group Pointer to group of parameters
    \param[in] name Name of parameter
    \param[in] type Type of parmeter's values
    \param[in] values Values of parameters (each parameter is array)
    \param[in] count Count of values
    \param[out] follower Pointer to pointer which will follow parameter's value
    \param[in] description Description of parameter
    \return 0 if success, -1 and sets errno else 
*/
int addParameterToGroup(GroupParam *group, char *name, enum ParameterType type,
    union Value *values, int count, union Value **follower, char *description) {
    /*CHECK THAT DATA IS CORRECT*/
    if(!group || !name || !values || !follower || !count || !type){
        return -1;
    }

    char c;
    while((c = *name++)){
        if(c == '.'){
            return -1;
        }
    }

    /*CHECK ABILITY TO ADDING*/

    if (findParameterInGroup(group, name)) {
        return -1;
    }

    if(group->isBlocked) {
        return -1;
    }
    
    /*INIT PARAMETER*/
    Parameter *param = (Parameter *)malloc(sizeof(Parameter));
    strcpy(param->group,group->name);
    strcpy(param->name,name);
    param->type = type;

    param->values = (union Value*)malloc(sizeof(union Value*) * (count + 1));
    param->values[0].lngf = count;
    for (int i = 1; i <= count; ++i) {
        *(param->values + i) = values[i - 1];
    }

    struct Follower *head = (struct Follower*)malloc(sizeof(struct Follower));
    head->addr = follower;
    head->next = NULL;
    param->head = head;

    strcpy(param->description, description);

    /*ADD PARAMETER IN HASH TABLE*/
    struct ParameterNode *node = (struct ParameterNode *)malloc(sizeof(struct ParameterNode));
    node->data = param;
    node->next = NULL;
    
    long int index = hashString(name) % group->size;
    if(!group->table[index]){
        group->table[index] = node;
    } else {
        struct ParameterNode *curParamNode = group->table[index];
        while(curParamNode->next) {
            curParamNode = curParamNode->next;
        }
        curParamNode->next = node;
    }

    group->curCount++;

    return 0;
}

/*!
    \brief deletes parameter structure

    Checks ability and delete parameter structure. Parameter can't has folllowers
    for avoiding undefined behavior. If group hasn't setted parameter
    function will return success value.
    \param[in] group Pointer to group
    \param[in] name Name of parameter
    \return 0 if success, -1 and sets errno else
*/
int removeParameterFromGroup(GroupParam *group, char * name) {
    /*CHECK THAT ARGUMENTS ARE CORRECT*/
    if (!group || !name){
        return -1;
    }

    /*CHECK ABILITY FOR REMOVING*/
    if (group->isBlocked){
        return -1;
    }

    long int index = hashString(name) % group->size;

    if(!group->table[index]){
        /*NOTHING TO DO*/
    } else if (!strcmp(group->table[index]->data->name, name)){
        /*CHECK THAT PARAMETER HAS NO FOLLOWERS*/
        if (group->table[index]->data->head) {
            return -1;
        }
        /*REMOVE AND FREE PARAMETER'S NODE*/
        struct ParameterNode *delNode = group->table[index];
        group->table[index] = group->table[index]->next;
        freeParameter(delNode->data);
        free(delNode);

    } else {

        struct ParameterNode *curNode = group->table[index]->next;
        struct ParameterNode *prevNode = group->table[index];

        /*FIND NODE*/
        while(curNode) {
            if(!strcmp(curNode->data->name, name)) {
                /*CHECK THAT PARAMETER NAS NO FOLLOWERS*/
                if (curNode->data->head) {
                    return -1;
                }
                /*REMOVE AND FREE PARAMETER'S NODE*/
                prevNode->next = curNode->next;
                freeParameter(curNode->data);
                free(curNode);
                break;
            }
            curNode = curNode->next;
            prevNode = prevNode->next;
        }
    }
    group->curCount--;
    return 0;    
}

/*!
    \struct GroupNode
    \brief Support structure needed in GroupsTableData
*/
struct GroupNode {
    GroupParam *value;
    struct GroupNode *next;
};

/*!
    \struct GroupsTableData
    \brief table of parmeter groups

    Hash table that contains groups of configuration parameters.
*/
typedef struct GroupsTableData {
    struct GroupNode **table;
    int size;  
    int curCount; ///< current count of parmeter groups
    int isAllocated; ///< flag means that buffer table allocated and exists
} GroupsTable;



GroupsTable CAT = {NULL, 100, 0, 0}; ///< GLOBAL MAIN CONFIGURATION ACCESS TABLE



/*!
    \brief creates new group in CAT

    It checks ability to create new group and do this.
    \param[in] name Name of the group
    \param[in] size Size of group's hash table
    \param[in] blockMode Boot value of block mode
    \return pointer to new group or NULL and sets errno
    if this group already exists or other error occured
*/
GroupParam *createGroup(char *name, int size, int blockMode) {
    /*CHECK THAT ARGUMENTS ARE CORRECT*/
    if (!CAT.isAllocated) {
        return NULL;
    }

    if (!name) {
        return NULL;
    }

    /*CREATE GROUP*/
    GroupParam *group = (GroupParam *)malloc(sizeof(GroupParam));
    strcpy(group->name,name);
    group->isBlocked = blockMode;
    group->size = size;
    group->curCount = 0;
    group->table = (struct ParameterNode **)calloc(size, sizeof(struct ParameterNode*)); 

    struct GroupNode *node = (struct GroupNode *)malloc(sizeof(struct GroupNode));
    node->next = NULL;
    node->value = group;

    /*INSERT GROUP IN HASH TABLE*/
    long int index = hashString(name) % CAT.size;

    if(!CAT.table[index]) {
        CAT.table[index] = node;
    } else {
        struct GroupNode *curNode = CAT.table[index];
        while(curNode->next){
            curNode = curNode->next;
        }
        curNode->next = node;
    }
    CAT.curCount++;
    return node->value;
}

/*!
    \brief finds group in CAT by name
    \param[in] name Name of group
    \return Pointer to group if it found group, NULL and sets errno else
*/
GroupParam *findGroup(char *name) {
    /*CHECK THAT ARGUMENTS ARE CORRECT*/
    if(!CAT.isAllocated) {
        return NULL;
    }

    if (!name) {
        return NULL;
    }

    /*FIND GROUP IN LIST*/
    long int index = hashString(name) % CAT.size;
    struct GroupNode *curNode = CAT.table[index];
    while (curNode) {
        if (!strcmp(curNode->value->name, name)) {
            return curNode->value;
        }
        curNode = curNode->next;
    }
    return NULL;
}


/*!
    \brief destroys group
    Checks if arguments are correct and group is empty, it destroys group
    If group doesn't exist whe return success result
    \param[in] name Name of group
    \returns 0 if success, -1 and sets errno else
*/
int destroyGroup(char* name) {
    /*CHECK ABILITY*/
    if (!CAT.isAllocated) {
        return -1;
    }

    if (!name) {
        return -1;
    }
    /*TRY TO DELETE GROUP FROM LIST*/
    long int index = hashString(name) % CAT.size;

    if(!CAT.table[index]){
        /*NOTHING TO DO*/
    } else if (!strcmp(CAT.table[index]->value->name,name)) {
        struct GroupNode *delNode = CAT.table[index];
        /*CHECK THAT TABLE IS EMPTY*/
        if (delNode->value->curCount) {
            return -1;
        }
        /*FREE GROUP*/
        CAT.table[index] = CAT.table[index]->next;
        freeGroup(delNode->value);
        free(delNode);    
    } else {

        struct GroupNode *curNode = CAT.table[index]->next;
        struct GroupNode *prevNode = CAT.table[index];

        /*FIND NODE*/
        while (curNode) {
            if (!strcmp(curNode->value->name, name)) {
                /*CHECK THAT TABLE IS EMPTY*/
                if (curNode->value->curCount) {
                    return -1;
                }
                /*FREE GROUP*/
                prevNode->next = curNode->next;
                freeGroup(curNode->value);
                free(curNode);
                break;
            }
            curNode = curNode->next;
            prevNode = prevNode->next;
        }
    }
    CAT.curCount--;
    return 0;
}


/*!
    \brief Setter for block mode
    Then block mode is active, group's count of parameter is constant.
    It means that you can't add or remove parameters.(So you can still change it)
    \param[in] group Pointer to group
    \param[in] blockMode New state of block mode
    \return 0 if success, -1 and sets errno else
*/
int setBlockMode(GroupParam *group, int blockMode) {
    if (!group) {
        return -1;
    }

    group->isBlocked = blockMode;
    return 0;
}

/*!
    \brief get parameter from CAT 
    \param[in] group Group's name
    \param[in] name Parameter's name
    \returns pointer to parameter if success, NULL end sets errno else
*/
Parameter *getCATParam(char *group, char *name){
    if (!group || !name) {
        return NULL;
    }

    GroupParam *foundGroup = findGroup(group);
    
    if (!foundGroup) {
        return NULL;
    }

    return findParameterInGroup(foundGroup, name);
}


/*USER INTERFACE IMPLEMENTATION*/


/*!
    \brief initialize CAT
    Use this function at the beginning of session to allocate memory for CAT.
*/
int initCAT() {
    CAT.table = (struct GroupNode **)calloc(CAT.size, sizeof(struct GroupNode *));
    if (!CAT.table){
        return -1;
    }
    CAT.isAllocated = 1;
    return 0;
}


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
                       union Value **follower, char *description) {    
    if (!groupName || !name || !values || !follower) {

        return -1;
    }

    /*FIND GROUP WITH THE SAME NAME*/
    GroupParam *foundGroup = findGroup(groupName);
    GroupParam *group = NULL;

    if (!foundGroup) { ///< \todo check errno before, maybe NULL because of error 
        group = createGroup(groupName, 100, 0);
    } else {
        group = foundGroup;
    }

    if (!group) {
        return -1;
    }

    /*ADD NEW PARAMETER*/

    if (addParameterToGroup(group, name , type, values, count, follower, description)) {
        return -1;
    }
    return 0;    
}


/*!
    \brief delete configuration parameter
    Checks if parameter has no followers when delete parameter.
    If group or parameter doesn't exist when success result.
    \param[in] groupName Name of group
    \param[in]
*/
int deleteCATParameter(char *groupName, char *name) {
    if (!groupName || !name) {

        return -1;
    }

    /*FIND GROUP WITH THE SAME NAME*/
    GroupParam *foundGroup = findGroup(groupName);

    if (!foundGroup) { ///< \todo check errno
        return 0;
    }

    if (removeParameterFromGroup(foundGroup, name)) {
        return -1;
    }
    destroyGroup(groupName); // try to destroy group
    return 0;
}


/*!
    \brief sets group's block mode
    Checks if group exists then change block mode.
    In active block mode group couldn't be extended or reduced.
    \param[in] group Group's name
    \param[in] blockMode New state of block mode
    \return 0 if success, -1 and sets errno else
*/
int setGroupBlockMode(char *group, int blockMode) {
    if (!group) {
        return -1;
    }

    GroupParam *foundGroup = findGroup(group);

    if (!foundGroup) {
        return -1;
    }

    return setBlockMode(foundGroup, blockMode);
}


/*!
    \brief gets parameter's description
    \param[in] group Group's name
    \param[in] name Parameter's name
    \return cstring (description) if success , NULL and sets errno else
*/
char *getCATParamDescr(char *group, char *name) {
    Parameter *param = getCATParam(group, name);
    if (!param){
        return NULL;
    }
    return getParamDescr(param);
}


/*!
    \brief updates parameter's value
    Updates parameter's value and updates followers.
    \param[in] group Group's name
    \param[in] name Parameter's name
    \param[in] count New count of elements (each CAT parameter is array)
    \param[in] values New values of parameter
    \return 0 if success, -1 and sets errno else
*/
int updateCATParameter(char *group, char *name, int count, union Value *values) {
    if (!values || !count) {
        return -1;
    }
    Parameter *param = getCATParam(group, name);
    if (!param){
        return -1;
    }
    return updateParameter(param, count, values);
}

/*!
    \brief adds new follower to parameter
    \param[in] group Group's name
    \param[in] name Parameter's name
    \param[in] follower pointer to pointer which will follow CAT parameter
    \return 0 if success, -1 and sets errno else
*/
int addFollowerToCAT(char *group, char *name, union Value **follower) {
    Parameter *param = getCATParam(group, name);
    if (!param) {
        return -1;
    }
    return addFollowerToParameter(param, follower);
}


/*!
    \brief removes follower from parameter
    Removes follower and change it value to NULL
    \param[in] group Group's name
    \param[in] name Parameter's name
    \param[in] follower pointer to pointer which will be deleted from followers
    \return 0 if success, -1 and sets errno else
*/
int removeFollowerFromCAT(char *group, char *name, union Value **follower) {
    Parameter *param = getCATParam(group, name);
    if (!param) {
        return -1;
    }
    return removeFollowerFromParameter(param, follower);
}
