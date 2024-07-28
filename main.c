#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>

#if defined(_WIN32) || defined(_WIN64) || defined(__WINDOWS__)

#include <direct.h>
#define getcwd _getcwd

#elif defined(__linux__)

#include <sys/stat.h>
#include <unistd.h>

#endif

// Fuck MacOS support

// TODO: Files and directories with "$" in their names will break the program
// FIXME: Probably littered with allocation errors, I suck at C
// Fix is literally to change SEPARATOR to something that can't be in directory or file name, I'm just too lazy to implement it

// Goto comments are subject to drift, expect an error of around 7 lines

#define SEPERATOR "$"
#define DEPTH_LIMIT 3

size_t sortDirMemSize;
size_t skipMemSize;

/**
* Reads through directory entries and passes the first one not skipped or entered (A directory) to moveSort to be sorted output must be freed by user
* @param sortDir: The directory to sort
* @param skip: The directories or files to skip
* @return A list of entries to be sorted (Should be freed by user) or an error message
*/
char * readEntries(char * sortDir, char * skip) {
  DIR * pCurDir;
  struct dirent * dirEntry;
  struct stat statBuf;
    
  // Check if the directory actually opened and output a message if it didn't
  if ((pCurDir = opendir(sortDir)) == NULL)
    return "!: Failed to open the search directory.";

  chdir(sortDir);
  char * prevDir = sortDir;
  char * tok;
  unsigned short depth = 0;

  char * pSkipCp = skip;
  char * pDirEntries;
  char * buf;
  char * catBuf;

  size_t dirEntriesSize;
  
  tok = strtok(skip, SEPERATOR);
  
  while (1) {   
    // If no more entries return our entries
    if ((dirEntry = readdir(pCurDir)) == NULL) {
      // Allocate buffer for strcat
      // FIXME: May cause memory leak?
      // TODO: I think this has to be freed at some point
      catBuf = malloc(strlen(getcwd(buf, 0)) + sizeof(char) + dirEntriesSize);
      
      catBuf = getcwd(buf, 0);
      free(buf);
      catBuf = strcat(strcat(catBuf, SEPERATOR), pDirEntries);
      free(pDirEntries);
      // TODO: Remove debugging puts
      puts(catBuf);
      
      return catBuf;    
    }
    
    // Loop through each token in skip and compare them to the dirEntry, if they match, then skip returning that dirEntry
    while (1) {
      // Continue if this entry matches anything in skip
      if (strcmp(tok, dirEntry->d_name) == 0) {
        skip = pSkipCp;
        tok = strtok(skip, SEPERATOR);
        // Goes to line 99 and continues the outer loop
        goto lContinueOuter;
      }

      tok = strtok(NULL, SEPERATOR);
      
      if (tok == NULL)
        break;
    }  
   
    // Stat entry to see if it's a directory
    lstat(dirEntry->d_name, &statBuf);

    // If the entry is a directory and the depth limit is exceeded, return that entry
    if (S_ISDIR(statBuf.st_mode)) {
      if (depth >= DEPTH_LIMIT)
        // Goes to line 96 
        goto lAddEntry;
      
      closedir(pCurDir);
      
      if ((pCurDir = opendir(dirEntry->d_name)) == NULL) {
        closedir(pCurDir);
        puts("!: Failed to open nested directory");
        continue;
      }

      
      depth++;

      // Add together the previous dir and the dirEntry to cd into new dir
      prevDir[sortDirMemSize - 2] = 0;

      // FIXME: May cause buffer overflow (Not sure if I need to allocate space for the null terminator)
      catBuf = malloc(sizeof(char) + dirEntry->d_reclen);
      catBuf = "/\0";
      
      chdir(strcat(catBuf, dirEntry->d_name));
      
      // TODO: Remove debugging puts
      printf("D: Directory we just changed into: %s", strcat(catBuf, dirEntry->d_name));

      free(catBuf);
      
      prevDir = dirEntry->d_name;
      continue;
    }
    
    lAddEntry:
    // FIXME: I have no idea if this actually works, help
    if (pDirEntries == NULL) {
      pDirEntries = malloc(strlen(dirEntry->d_name));
      pDirEntries = dirEntry->d_name;
    }  else {
      pDirEntries = realloc(pDirEntries, (2 * strlen(dirEntry->d_name)) + sizeof(char));
      
      strcat(pDirEntries, SEPERATOR);
      strcat(pDirEntries, dirEntry->d_name);
    }

    continue;
    
    lContinueOuter:
    continue;
  }
}

/**
* Checks if string input from the user is valid and useable
* @param input: The input from the user
* @param type: The type of input that is meant to be coming in (0 for directory, 1 for keywords, and 2 for file extensions)
* @return 0 if the input is valid, 1 if not
*/
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
  puts("!: You used your own function wrong idiot :3");
  return 1;
}

/**
* Checks if single char input from the user is valid and useable
* @param input: The input from the user
* @return Zero if input is valid, 1 if not
*/
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
  puts(":: Creating new config...");
  FILE * pConfig;
  size_t sizeBuf = sizeof(char);
    
  // Exit program if open config fails somehow
  if ((pConfig = fopen("config", "w")) == NULL) {
    puts("!! \aWe're cooked, failed to open config.\n!! Get your shit together and then come back to me!\n:: Exiting...");
    
    return 1;
  }

  // The list of filetype extensions to sort in the same order as the directories they're to be sorted into
  char * pSortExtensions = malloc(sizeof(char));

  lRetrySortExtensions:

  // Collect random newlines
  fgetc(stdin);
  
  // Getting filetype extensions to sort by and placing them into the config
  puts("\nPlease enter in the filetype extensions you would like to sort in the form \".extension\" followed by a \""SEPERATOR"\" (.extension"SEPERATOR"):");
  getline(&pSortExtensions, &sizeBuf, stdin);

  if (checkStrInput(pSortExtensions, 2) == 1) {
    puts("!: Input invalid!\n!: Please try again:");

    // Goes to line 178
    goto lRetrySortExtensions;
  }
  
  fprintf(pConfig, "\nconst char* pSortExtensions = \"'%s'\"", pSortExtensions);

  lRetryExtensionSortDirs:

  sizeBuf = sizeof(char);
  // The list of directories into which the specified filetypes will go
  char * pExtensionSortDirs = malloc(sizeof(char));
    
  // Getting the directories into which to sort the aformentioned filetypes and placing them into the config
  puts("\nPlease enter the directories you would like these file types to be sorted to in the same order you entered the extensions and in the form \"/directory/\" (Please use the full path) followed by a \""SEPERATOR"\". (/directory/"SEPERATOR"):");
  getline(&pExtensionSortDirs, &sizeBuf, stdin);

  if(checkStrInput(pExtensionSortDirs, 0) == 1) {
    puts("!: Input invalid!\n!: Please try again:");
    // Goes to line 196
    goto lRetryExtensionSortDirs;
  }
  
  fprintf(pConfig, "\nconst char* pExtensionSortDirs = \"'%s'\"", pExtensionSortDirs);

  lRetrySortKeywords:

  sizeBuf = sizeof(char);
  // List of filename keywords to sort in the same order as the directories they're to go to
  char * pSortKeywords = malloc(sizeof(char));
    
  // Getting the keywords to sort by and placing those into the config
  puts("\nPlease enter in the name keywords you would like to sort in the form \"keyword\" followed by a \""SEPERATOR"\" (keyword"SEPERATOR"):");
  getline(&pSortKeywords, &sizeBuf, stdin);

  if(checkStrInput(pSortKeywords, 1) == 1) {
    puts("!: Input invalid!\n!: Please try again:");
    // Goes to line 214
    goto lRetrySortKeywords;
  }
  
  fprintf(pConfig, "\nconst char* pSortExtensions = \"'%s'\"", pSortKeywords);

  lRetryKeywordsSortDirs:

  sizeBuf = sizeof(char);
  // The list of directories into which the specified keywords will go
  char * pKeywordsSortDirs = malloc(sizeof(char));

  // Getting the directories to into which to sort the aformentioned filetypes and placing that into the config
  puts("\nPlease enter the directories you would like matches to be sorted to in the same order you entered their keywords and in the form \"/directory/\" (Please use the full path) followed by a \""SEPERATOR"\". (/directory/"SEPERATOR"):");
  getline(&pKeywordsSortDirs, &sizeBuf, stdin);
  
  
  if(checkStrInput(pKeywordsSortDirs, 0) == 1) {
    puts("!: Input invalid!\n!: Please try again:");
    // Goes to line 232
    goto lRetryKeywordsSortDirs;
  }
  
  fprintf(pConfig, "\nconst char* pExtensionSortDirs = \"'%s'\"", pKeywordsSortDirs);

  return 0;
}

/**
* Takes in the path to a file/directory to be sorted, sorts it, and then moves the entry to the correct location
*/

int moveSort(char * sortEntries) {
  
}

int main () {
  // The directory to sort
  char * pSortDir = malloc(sizeof(char));
  // Whether or not to sort through subdirectories
  char sortSubDirsRes;
  // Whether or not to use the config
  char useConfigRes;
  
  size_t bufSize = sizeof(char);
  char * buf;
  
  puts("\nWelcome to fileSortBetter!\nStart by entering either the full path to the directory to be sorted or Enter if the directory to be sorted is the current:");
  getline(&pSortDir, &bufSize, stdin);

  sortDirMemSize = bufSize;

  if (strcmp(pSortDir, "\n") == 0) {
    pSortDir = realloc(pSortDir, strlen(getcwd(buf, 0)));
    pSortDir = getcwd(buf, 0);
    puts(getcwd(buf, 0));
  }
  
  lRetrySortSubDirsRes:
  
  // Whether or not to sort through subdirs
  puts("\nSort through subdirectories? (Depth Limit 3) [y/n]");
  sortSubDirsRes = fgetc(stdin);
  if (checkCharInput(sortSubDirsRes) == 1) {    
    puts("!: Input invalid!\n!: Please try again:");
    // Goes to line 275
    goto lRetrySortSubDirsRes;
  }

  fgetc(stdin);
  
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
      puts("!: Input invalid!\n!: Please try again:");
      // Goes to line 288
      goto lRetrySkipEntry;
    }
    tok = strtok(NULL, SEPERATOR);
  }   

  skipMemSize = bufSize;

  lRetryUseConfigRes:
  
  // Whether to use the current config
  puts("\nUse current config? (fileSortBetter/config) [Y/n/(r)ead]");
  useConfigRes = fgetc(stdin);
  
  if (checkCharInput(useConfigRes) == 1 && !(useConfigRes == 'r' || useConfigRes == 'R')) {
    puts("!: Input invalid!\n!: Please try again:");
    // Goes to line 309
    goto lRetryUseConfigRes; 
  }

  struct stat buffer;
    
  if (useConfigRes == 'y' || useConfigRes == 'Y')
    // Goes to line 331
    goto lSkipDeclineCheck;
  
  // If the user declined to use the current config or if the config doesn't exist
  if (useConfigRes == 'n' || useConfigRes == 'N')
    // Goes to line 346
    goto lMakeConf;

  lSkipDeclineCheck:

  if (stat("config", &buffer) == -1) {
    puts("!: Config does not exist.");
    goto lMakeConf;
  }    
  
  if (buffer.st_size <= 1) {
    puts("!: Config exists but is empty.");
    // Goes to line 346
    goto lMakeConf;   
  }

  // Goes to line 354
  goto lPassedChecks;
  
  lMakeConf:
  if (makeConf() == 1) {
    puts("!! Failed making the config, we're boned.\n!! Bye Bye :3");
    return 1;
  }

  lPassedChecks:
    
  // Printing config if read option selected
  if (useConfigRes == 'r') {
    FILE * file = fopen("config", "r");
    char c;
  
    while ((c = fgetc(file)) != EOF) 
      putchar(c);
  
    fclose(file);
  }

  // Now that we have the treasure, begin sorting the directories
  puts("\n:: Beginning the sorting process and definitly not stealing your data :3...");

  char * pCwd;

  // FIXME: This is a test, it will not be final
  if (strcmp(pSortDir, "\n") == 0) {
    getcwd(pCwd, 0);
    puts(readEntries(pCwd, pSkip));
  } else {
    puts(readEntries(pSortDir, pSkip));
  }
  
  return 0;  
}
