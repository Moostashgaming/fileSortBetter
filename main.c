#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <linux/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/sendfile.h>

// TODO: Files and directories with "$" in their names will break the program
// Fix is literally to change SEPARATOR to something that can't be in directory or file name, I'm just too lazy to implement it

#define SEPERATOR "$"

/**
* Reads through directory entries and passes them to moveSort to be sorted
* @param sortDir: The directory to sort
* @param skip: The directories or files to skip
* @return The current entry to be sorted, or a bell character prefixed error message
*/
char * readEntries(char * sortDir, char * skip) {
  DIR * pSortDir;
  struct dirent * dirEntry;
  struct stat statBuf;

  // Checks if the directory actually opened and outputs a message and exits the function if it didn't
  if ((pSortDir = opendir(sortDir)) == NULL)
    return "\aFailed to open the search directory.";

  chdir(sortDir);
  char * tok;
  
  // Loop through each token in skip and compare them to the dirEntry, if they match, then skip returning that dirEntry
  while (1) {   
    // If no more entries return that
    if ((dirEntry = readdir(pSortDir)) == NULL)
      return NULL;    
    
    tok = strtok(skip, SEPERATOR);
    
    if (strcmp(tok, dirEntry->d_name) == 0)
      continue;
    // Stat entry to see if it's a directory
    lstat(dirEntry->d_name, &statBuf);
  
    if (S_ISDIR(statBuf.st_mode)) {
      
    }
    return dirEntry->d_name;
  }
}

int main () {
  // The directory to sort
  char * pSortDir;
  // Whether or not to sort through subdirectories
  char sortSubDirsRes;
  // The directories and/or files to be skipped by the program
  char * pSkip;
  // Whether or not to use the config
  char useConfigRes;
  
  // The list of filetype extensions to sort in the same order as the directories they're to be sorted into
  char * pSortExtensions;
  // The list of directories into which the specified filetypes will go
  char * pExtensionSortDirs;

  // List of filename keywords to sort in the same order as the directories they're to go to
  char * pSortKeywords;
  // The list of directories into which the specified keywords will go
  char * pKeywordsSortDirs;

  struct stat * buffer;
  size_t bufSize = sizeof(char);
    
  puts("\nWelcome to fileSortBetter!\nStart by entering either the full path to the directory to be sorted or a \".\" if the directory to be sorted is the current:");
  getline(&pSortDir, &bufSize, stdin);
  
  // Whether or not to sort through subdirs
  puts("\nSort through subdirectories? (Depth Limit 3) [y/n]");
  sortSubDirsRes = fgetc(stdin);
  
  // Which files and dirs to skip
  puts("\nPlease enter the names of the files/directories you would like the program to skip over in the form \"/skipDirectory/\" and \"skipFile\" separated by a \""SEPERATOR"\" (/skipDirectory/"SEPERATOR"skipFile)");
  getline(&pSkip, &bufSize, stdin);
  
  // Whether to use the current config
  puts("\nUse current config? (fileSortBetter/config) [Y/n/(r)ead]");
  useConfigRes = fgetc(stdin);
  // Printing config if read option selected
  if (useConfigRes == 'r') {
    FILE * file = fopen("config", "r");
    char c; 
  
    while ((c = fgetc(file)) != EOF) 
      putchar(c);
  
    fclose(file);
  }
  // If the user declined to use the current config or if the config doesn't exist
  if (useConfigRes == 'n' || (stat("config", buffer) == -1)) {
    free(buffer);
    puts("\nCreating new config...");
    FILE * pConfig;
    // Exit program if open config fails somehow
    if ((pConfig = fopen("config", "w")) == NULL) {
      puts("\n\aWe're cooked, failed to open config.\nGet your shit together and then come back to me!\nExiting...");
      return 1;
    }
    // Getting filetype extensions to sort by and placing them into the config
    puts("\nPlease enter in the filetype extensions you would like to sort in the form \".extension\" followed by a \""SEPERATOR"\" (.extension"SEPERATOR"):");
    getline(&pSortExtensions, &bufSize, stdin);
    fprintf(pConfig, "char* pSortExtensions = \"'%s'\"", pSortExtensions);

    // Getting the directories into which to sort the aformentioned filetypes and placing them into the config
    puts("\nPlease enter the directories you would like these file types to be sorted to in the same order you entered the extensions and in the form \"/directory/\" (Please use the full path) followed by a \""SEPERATOR"\". (/directory/"SEPERATOR"):");
    getline(&pExtensionSortDirs, &bufSize, stdin);
    fprintf(pConfig, "\nchar* pExtensionSortDirs = \"'%s'\"", pExtensionSortDirs);

    // Getting the keywords to sort by and placing those into the config
    puts("\nPlease enter in the name keywords you would like to sort in the form \"keyword\" followed by a \""SEPERATOR"\" (keyword"SEPERATOR"):");
    getline(&pSortKeywords, &bufSize, stdin);
    fprintf(pConfig, "char* pSortExtensions = \"'%s'\"", pSortKeywords);

    // Getting the directories to into which to sort the aformentioned filetypes and placing that into the config
    puts("\nPlease enter the directories you would like matches to be sorted to in the same order you entered their keywords and in the form \"/directory/\" (Please use the full path) followed by a \""SEPERATOR"\". (/directory/"SEPERATOR"):");
    getline(&pKeywordsSortDirs, &bufSize, stdin);
    fprintf(pConfig, "\nchar* pExtensionSortDirs = \"'%s'\"", pKeywordsSortDirs);
  } 

  // Now that we have the treasure, begin sorting the directories
  puts("\nBeginning the sorting process and definitly not stealing your data :3...");

  return 1;  
}
