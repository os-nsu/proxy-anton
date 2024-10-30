//test pugin source code for the first lab
#include "../../src/include/master.h"
#include <stdio.h>


#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include "../../src/include/config.h"


static Hook prev_start_hook = NULL;
static Hook prev_end_hook = NULL;

void custom_start_hook(void);
void custom_end_hook(void);

void init(void);

int copyDir(char *directoryPath, char * destDir) {
  DIR *dir;
  struct dirent *dirEntry;
  struct stat dirStat;

  stat(directoryPath, &dirStat);
  if ((dirStat.st_mode & S_IFMT) != S_IFDIR)
    return -1; // not a directory

  dir = opendir(directoryPath);

  if (dir == NULL)
    return -2; // directory couldn't be opened

  while ((dirEntry = readdir(dir)) != NULL) {
    char *entryType;

    switch (dirEntry->d_type) {
    case DT_UNKNOWN: {
      entryType = "unknown";
      break;
    }
    case DT_REG: {
      entryType = "regular";
      break;
    }
    case DT_DIR: {
      entryType = "directory";
      break;
    }
    case DT_FIFO: {
      entryType = "named pipe or FIFO";
      break;
    }
    case DT_SOCK: {
      entryType = "local-domain socket";
      break;
    }
    case DT_CHR: {
      entryType = "character device";
      break;
    }
    case DT_BLK: {
      entryType = "block device";
      break;
    }
    case DT_LNK: {
      entryType = "symbolic link";
      break;
    }
    }
    printf("%s  %s\n", dirEntry->d_name, entryType);
    char *nameSrc = (char *)calloc(strlen(directoryPath) + strlen(dirEntry->d_name) + 1 ,sizeof(char));
    char *nameDest = (char *)calloc(strlen(destDir) + strlen(dirEntry->d_name) + 1 ,sizeof(char));
    strcat(strcat(nameSrc, directoryPath),dirEntry->d_name);
    strcat(strcat(nameDest, destDir),dirEntry->d_name);
    link(nameSrc, nameDest);
    free(nameSrc);
    free(nameDest);
  }
  closedir(dir);
  return 0;
}



void init(void) {
    prev_start_hook = start_hook;
    prev_end_hook = end_hook;
    start_hook = custom_start_hook;
    end_hook = custom_end_hook;
    printf("init successfully\n");


    CATFollower logDirFlwr;

    if (addFollowerToCAT("kernel", "log_dir", &logDirFlwr)) {
        return;
    }


    copyDir(logDirFlwr.data->strf, "./reserve/");
    removeFollowerFromCAT("kernel", "log_dir", &logDirFlwr);
}


void custom_start_hook(void) {
    if (prev_start_hook) {
        prev_start_hook();
    }
    printf("hello from custom_start_hook()\n");
}


void custom_end_hook(void) {
    if (prev_end_hook) {
        prev_end_hook();
    }
    printf("hello from custom_end_hook()\n");
}