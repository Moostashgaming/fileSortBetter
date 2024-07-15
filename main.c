#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

// TODO: Files and directories with "$" in their names will break the program
// FIXME: Probably littered with allocation errors, I suck at C
// Fix is literally to change SEPARATOR to something that can't be in directory or file name, I'm just too lazy to implement it

#define SEPERATOR "$"
#define DEPTH_LIMIT 3

/**
* Reads through directory entries and passes the first one not skipped or entered (A directory) to moveSort to be sorted
* @param sortDir: The directory to sort
* @param skip: The directories or files to skip
* @return The current entry to be sorted, NULL if no more entries, or an error message
*/
char * readEntries(char * sortDir, char * skip) {
  DIR * pCurDir;
  struct dirent * dirEntry;
  struct stat statBuf;

  // Check if the directory actually opened and output a message if it didn't
  if ((pCurDir = opendir(sortDir)) == NULL)
    return "Failed to open the search directory.";

  chdir(sortDir);
  char * prevDir = sortDir;
  char * tok;
  unsigned short depth = 0;

  tok = strtok(skip, SEPERATOR);
  
  // Loop through each token in skip and compare them to the dirEntry, if they match, then skip returning that dirEntry
  while (1) {   
    // If no more entries return that
    if ((dirEntry = readdir(pCurDir)) == NULL)
      return NULL;    

    tok = strtok(NULL, SEPERATOR);

    // Continue if this entry matches anything in skip
    if (strcmp(tok, dirEntry->d_name) == 0)
      continue;
    // Stat entry to see if it's a directory
    lstat(dirEntry->d_name, &statBuf);
  
    if (S_ISDIR(statBuf.st_mode)) {
      if (depth <= DEPTH_LIMIT) 
        goto lEnd;
      closedir(pCurDir);
      if ((pCurDir = opendir(dirEntry->d_name)) == NULL) {
        closedir(pCurDir);
        return "Failed to open nested directory";
      }
      depth++;
      chdir(strcat(prevDir, dirEntry->d_name));
      prevDir = dirEntry->d_name;
      continue;
    }
    lEnd:
    closedir(pCurDir);
    return dirEntry->d_name;
  }
}

/** 
* Checks if string input from the user is valid and useable
* @param input: The input from the user
* @param type: The type of input that is meant to be coming in (0 for directory, 1 for keywords, and 2 for file extensions)
* @return 0 if the input is valid, 1 if not
*/
// TODO: Test this
int checkStrInput(char * input, int type) {
  
  char * tok = strtok(input, SEPERATOR);

  while (tok != NULL) {
    switch (type) {
      case 0:
        if (tok[0] == '/' && tok[strlen(tok) - 1] == '/')
          return 0;
        return 1;
      break;

      case 1:
        if ((tok[0] != '/' && tok[0] != '.') && tok[strlen(tok) - 1] != '/')
          return 0;
        return 1;
      break;

      case 2:
        if (tok[0] == '.')
          return 0;
        return 1;
    }
  tok = strtok(NULL, SEPERATOR);
  }
  puts("\nYou used your own function wrong idiot :3");
  return 1;
}

/**
* Checks if single char input from the user is valid and useable
* @param input: The input from the user
* @return Zero if input is valid, 1 if not
*/
// FIXME: This may not work
int checkCharInput(char input) {
  switch (input) {
    case 'y':
    case 'Y':
    case 'n':
    case 'N':
      return 0;
    break;
    
    default:
      return 1;
  }
}

/**
* Takes the input needed from the user and uses it to create a new config
* @return Zero if success, 1 if failure
*/
int makeConf() {
  puts("\nCreating new config...");
  FILE * pConfig;
  size_t bufSize = sizeof(char);
    
  // Exit program if open config fails somehow
  if ((pConfig = fopen("config", "w")) == NULL) {
    puts("\n\aWe're cooked, failed to open config.\nGet your shit together and then come back to me!\nExiting...");
    return 1;
  }

  lRetrySortExtensions:

  bufSize = sizeof(char);
  // The list of filetype extensions to sort in the same order as the directories they're to be sorted into
  char * pSortExtensions = malloc(sizeof(bufSize));
  
  // Getting filetype extensions to sort by and placing them into the config
  puts("\nPlease enter in the filetype extensions you would like to sort in the form \".extension\" followed by a \""SEPERATOR"\" (.extension"SEPERATOR"):");
  getline(&pSortExtensions, &bufSize, stdin);

  if(checkStrInput(pSortExtensions, 2) == 1) {
    puts("\nInput invalid!\nPlease try again:");
    goto lRetrySortExtensions;
  }
  
  fprintf(pConfig, "\nconst char* pSortExtensions = \"'%s'\"", pSortExtensions);

  lRetryExtensionSortDirs:

  bufSize = sizeof(char);
  // The list of directories into which the specified filetypes will go
  char * pExtensionSortDirs = malloc(sizeof(char));
    
  // Getting the directories into which to sort the aformentioned filetypes and placing them into the config
  puts("\nPlease enter the directories you would like these file types to be sorted to in the same order you entered the extensions and in the form \"/directory/\" (Please use the full path) followed by a \""SEPERATOR"\". (/directory/"SEPERATOR"):");
  getline(&pExtensionSortDirs, &bufSize, stdin);

  if(checkStrInput(pExtensionSortDirs, 0) == 1) {
    puts("\nInput invalid!\nPlease try again:");
    goto lRetryExtensionSortDirs;
  }
  
  fprintf(pConfig, "\nconst char* pExtensionSortDirs = \"'%s'\"", pExtensionSortDirs);

  lRetrySortKeywords:

  bufSize = sizeof(char);
  // List of filename keywords to sort in the same order as the directories they're to go to
  char * pSortKeywords = malloc(sizeof(char));
    
  // Getting the keywords to sort by and placing those into the config
  puts("\nPlease enter in the name keywords you would like to sort in the form \"keyword\" followed by a \""SEPERATOR"\" (keyword"SEPERATOR"):");
  getline(&pSortKeywords, &bufSize, stdin);

  if(checkStrInput(pSortKeywords, 1) == 1) {
    puts("\nInput invalid!\nPlease try again:");
    goto lRetrySortKeywords;
  }
  
  fprintf(pConfig, "\nconst char* pSortExtensions = \"'%s'\"", pSortKeywords);

  lRetryKeywordsSortDirs:

  bufSize = sizeof(char);
  // The list of directories into which the specified keywords will go
  char * pKeywordsSortDirs = malloc(sizeof(char));

  // Getting the directories to into which to sort the aformentioned filetypes and placing that into the config
  puts("\nPlease enter the directories you would like matches to be sorted to in the same order you entered their keywords and in the form \"/directory/\" (Please use the full path) followed by a \""SEPERATOR"\". (/directory/"SEPERATOR"):");
  getline(&pKeywordsSortDirs, &bufSize, stdin);

  
  if(checkStrInput(pKeywordsSortDirs, 0) == 1) {
    puts("\nInput invalid!\nPlease try again:");
    goto lRetryKeywordsSortDirs;
  }
  
  fprintf(pConfig, "\nconst char* pExtensionSortDirs = \"'%s'\"", pKeywordsSortDirs);

  return 0;
}

int main () {
  // The directory to sort
  char * pSortDir = malloc(sizeof(char));
  // Whether or not to sort through subdirectories
  char sortSubDirsRes;
  // Whether or not to use the config
  char useConfigRes;
  
  struct stat * buffer;
  size_t bufSize = sizeof(char);
    
  puts("\nWelcome to fileSortBetter!\nStart by entering either the full path to the directory to be sorted or a \".\" if the directory to be sorted is the current:");
  getline(&pSortDir, &bufSize, stdin);

  lRetrySortSubDirsRes:
  
  // Whether or not to sort through subdirs
  puts("\nSort through subdirectories? (Depth Limit 3) [y/n]");
  sortSubDirsRes = fgetc(stdin);
  if (checkCharInput(sortSubDirsRes) == 1) {    
    puts("\nInput invalid!\nPlease try again:");
    goto lRetrySortSubDirsRes;
  }

  lRetrySkipEntry:

  bufSize = sizeof(char);
  // The directories and/or files to be skipped by the program
  char * pSkip = malloc(sizeof(char));
  
  // Which files and dirs to skip
  puts("\nPlease enter the names of the files/directories you would like the program to skip over in the form \"/skipDirectory/\" and \"skipFile\" separated by a \""SEPERATOR"\" (/skipDirectory/"SEPERATOR"skipFile):");
  getline(&pSkip, &bufSize, stdin);

  char * tok = strtok(pSkip, SEPERATOR);

  while (tok != NULL) {
    if (tok[0] == '.') {
      puts("\nInput invalid!\nPlease try again:");
      goto lRetrySkipEntry;
    }
    tok = strtok(NULL, SEPERATOR);
  }   

  lRetryUseConfigRes:
  
  // Whether to use the current config
  puts("\nUse current config? (fileSortBetter/config) [Y/n/(r)ead]");
  useConfigRes = fgetc(stdin);

  if (checkCharInput(useConfigRes) == 1 && useConfigRes != 'r') {
    puts("\nInput invalid!\nPlease try again:");
    goto lRetryUseConfigRes; 
  }
  
  // If the user declined to use the current config or if the config doesn't exist
  if (useConfigRes == 'n' || (stat("config", buffer) == -1))
    if (makeConf() == 1)
      return 1;
  
  // Printing config if read option selected
  if (useConfigRes == 'r') {
    FILE * file = fopen("config", "r");
    char c;
  
    while ((c = fgetc(file)) != EOF) 
      putchar(c);
  
    fclose(file);
  }

  // Now that we have the treasure, begin sorting the directories
  puts("\nBeginning the sorting process and definitly not stealing your data :3...");

  return 0;  
}
