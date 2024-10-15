
/*TEST PART TEST PART TEST PART TEST PART TEST PART TEST PART TEST PART TEST PART TEST PART TEST PART TEST PART TEST PART TEST PART TEST PART TEST PART */

//All tests were passed
/*

int main() {
    Parameter * testParameter = (Parameter *)calloc(1, sizeof(Parameter));
    testParameter->description = (char *)calloc(15, sizeof(char));
    strcpy(testParameter->description, "a nice parameter");
    testParameter->group = (char *)malloc(sizeof(char) * 7);
    testParameter->name = (char *)malloc(sizeof(char) * 7);
    strcpy(testParameter->group, "group1");
    strcpy(testParameter->name, "name12");
    testParameter->head = NULL;
    testParameter->type = T_DOUBLE;
    union Value *values = (union Value *)calloc(2, sizeof(union Value));
    values[0].lngf = 1;
    values[1].dblf = 42.1;
    testParameter->values = values;
    union Value *newValues = (union Value *)calloc(2, sizeof(union Value));
    newValues[0].dblf = 51.0;
    newValues[1].dblf = 56.5;
    
    printf("start updateParameter test\n");
    updateParameter(testParameter, 2, newValues);
    printf("end updateParameter test\n");
    free(newValues);
    printf("%lf %lf\n", testParameter->values[1].dblf, testParameter->values[2].dblf);
    printf("end updateParameter test\n");

    printf("start addFollowerToParameter test\n");
    CATFollower follower;
    addFollowerToParameter(testParameter,&follower);
    printf("size = %lld\n", *follower.size);
    for(int i = 0; i < *follower.size; ++i) {
        printf("see: %lf\n", follower.data[i].dblf);
    }
    printf("end FollowerToParameter test\n");

    printf("\nstart removeFollowerFromParameter test\n");
    printf("before removing follower list head is %p\n", testParameter->head);
    removeFollowerFromParameter(testParameter, &follower);
    printf("after removing follower list head is %p\n", testParameter->head);
    printf("and info in follower: size = %p, data = %p\n", follower.size, follower.data);
    printf("end removeFollowerFromParameter test\n");

    printf("\nstart getParameterDescr test\n");
    printf("description: %s\n",getParamDescr(testParameter));
    printf("end getParamDescr test\n");

    printf("\nstart hashString test\n");
    printf("took hash from description: %lu\n",hashString(testParameter->description));
    printf("end hashString test\n");

    //GROUPS
    
    printf("\nstart initCAT test\n");
    printf("result of init CAT: %d\n",initCAT());
    printf("end initCAT test\n");


    printf("\nstart createGroup test\n");
    GroupParam *group = createGroup("mygroup", 100, 0);
    printf("result pointer %p\n", group);
    printf("\nend createGroup test\n");

    printf("\nstart addParameterToGroup test\n");
    union Value sample = (union Value)"Barnaul";
    CATFollower followerGr = {NULL, NULL};
    char *nameS = (char *)calloc(8, sizeof(char));
    strcpy(nameS,"city");
    addParameterToGroup(group, nameS, T_STRING, &sample, 1, &followerGr, "description sample");
    printf("view from followerGr: size = %lld, value = %s\n",*followerGr.size, followerGr.data[0].strf);
    printf("end addParameterToGroup test\n");

    printf("\nstart findParameterInGroup test\n");
    Parameter *searchRes = findParameterInGroup(group, "city");
    Parameter *searchIncorrect = findParameterInGroup(group, "nothing");
    printf("Found first result: group - %s, name - %s, size - %lld, description - %s\n", searchRes->group, searchRes->name, searchRes->values[0].lngf, searchRes->description);
    printf("And incorrect result: pointer - %p\n", searchIncorrect);
    printf("\nend findParameterInGroup test\n");


    printf("\nstart findGroup test\n");
    GroupParam * searchGroupRes = findGroup("mygroup");
    GroupParam * searchGroupIncorrectRes = findGroup("nogroup");
    printf("find group name %s\n and cur count %d\n", searchGroupRes->name, searchGroupRes->curCount);
    printf("result pointer for incorrect query %p\n", searchGroupIncorrectRes);
    printf("end findGroup test\n");

    printf("\nstart setBlockMode test\n");
    printf("count elements before block: %d\n", group->curCount);
    setBlockMode(group, 1);
    union Value testv = (union Value)42LL;
    addParameterToGroup(group, "test", T_LONG, &testv, 1, NULL, NULL);
    printf("count elements with block after trying add element: %d\n", group->curCount);
    removeParameterFromGroup(group, "city");
    setBlockMode(group, 0);
    printf("count elements with block after trying delete element: %d\n", group->curCount);
    printf("end setBlockMode test\n");

    printf("\nstart removeParameterFromGroup test\n");
    printf("count of parameters before remove = %d\n",group->curCount );
    removeFollowerFromParameter(searchRes, &followerGr);
    removeParameterFromGroup(group, "city");
    Parameter *searchDeleted = findParameterInGroup(group, "city");
    printf("result of searching after removing %p\n",searchDeleted );
    printf("count of parameters after remove = %d\n",group->curCount );
    printf("end removeParameterFromGroup test\n");

    printf("\nstart destroyGroup test\n");
    destroyGroup("mygroup");
    destroyGroup("111");
    GroupParam *searchDelGroup = findGroup("myGroup");
    printf("result of group searching after destroy: %p\n", searchDelGroup);
    printf("end destroyGroup test\n");

    //TEST API

    printf("\nstart API complex test\n");
    CATFollower followerC;
    char *math = (char *)calloc(5, sizeof(char));
    strcpy(math,"math");
    char *phys = (char *)calloc(8, sizeof(char));
    strcpy(phys,"physics");
    char *os = (char *)calloc(3, sizeof(char));
    strcpy(os,"os");
    union Value *valuesSample = (union Value *)malloc(sizeof(union Value) * 3);
    valuesSample[0].strf = math;
    valuesSample[1].strf = phys;
    valuesSample[2].strf = os;
    createCATParameter("school", "subjects", T_STRING, 3, valuesSample, &followerC, "perfect subjects in school" );
    printf("follower view after creating:\n");
    for(int i = 0; i < *followerC.size; ++i){
        printf("element %d: %s\n", i, followerC.data[i].strf);
    }
    printf("description of new parameter: %s\n",getCATParamDescr("school", "subjects"));
    union Value oop = (union Value)"oop";
    updateCATParameter("school", "subjects", 1, &oop);
    printf("follower view after updating:\n");
    for(int i = 0; i < *followerC.size; ++i){
        printf("element %d: %s\n", i, followerC.data[i].strf);
    }

    setGroupBlockMode("school", 1);
    printf("turn on block mode\n");
    union Value mark = (union Value)5LL;
    printf(" result of trying add parameter %d\n", createCATParameter("school","marks", T_LONG, 1, &mark, NULL, NULL));
    printf("result of trying to delete element from group: %d\n", deleteCATParameter("school", "subjects"));
    printf("turn off block mode - %d\n",setGroupBlockMode("school", 0));
    CATFollower secFollower;
    addFollowerToCAT("school", "subjects", &secFollower);
    printf("add follower: follower 2 view : size - %lld, data - %s\n", *secFollower.size, secFollower.data[0].strf);
    printf("result of trying 2 to delete element from group: %d\n", deleteCATParameter("school", "subjects"));
    printf("results of delete 1: %d and delete 2: %d\n",removeFollowerFromCAT("school", "subjects", &followerC), removeFollowerFromCAT("school", "subjects", &secFollower));
    printf("followerC: %p\nsecFollower: %p\n",&followerC, &secFollower);
    printf("result of trying 3 to delete element from group: %d\n", deleteCATParameter("school", "subjects"));
    printf("end API complex test\n");
    

    printf("\nstart freeParameter test\n");
    freeParameter(testParameter);
    printf("end freeParameter test\n");
    return 0;
}
*/