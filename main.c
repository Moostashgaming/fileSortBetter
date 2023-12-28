#include "cjson/cJSON.h"
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <linux/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

#define TRUE 1
#define FALSE 0

typedef unsigned char uint8_t;

/// NOTE: Define all JSON objects expected (in the configuration file) in their respective groups here.
///       Make sure to line them up in order with conf_Settings_t. MAKE SURE TO.
///       If a new type needs to be implemented, update conf_LoadSettingsFromJSON(cJSON *parsed) and struct conf_Settings
#define JSON_ROOTOBJECTS_BOOL   "searchSubDirs sortDirs sortByFiletype sortByKeywords"
#define JSON_ROOTOBJECTS_ARRAYS "sortFiletypes sortTerms"

// Really hacky, should work just fine, but just in case. (gcc attrib)
__attribute__((__packed__))
union conf_Settings_t {
    struct  {
        uint8_t searchSubDirs : 1;
        uint8_t sortDirs : 1;
        uint8_t sortByFiletype : 1;
        uint8_t sortByKeywords : 1;
    };
    uint8_t indx[4];
} conf_Settings_t;

/// Stores the user defined settings after they are fully parsed from JSON. conf_Settings.indx[i] can be used to access elements by number.
union conf_Settings_t conf_Settings;

/// Stores the loaded and parsed configuration JSON object.
struct cJSON *conf_Parsed;

/// Stores the entries in the user-specified search directory.
char** dir_SearchEntries;
unsigned short dir_SearchEntries_c;

/// Sets internal variables based off the contents of the supplied JSON object.
/// Returns NULL if succeeded, an error message otherwise.
char* conf_LoadSettingsFromJSON(cJSON *parsed) {
    // Check if the supplied object contains data.
    if (parsed == NULL)
        return "\aSupplied JSON object is NULL. (Internal error, conf_LoadSettingsFromJSON)";

    cJSON *j_tmp;
    char *j_objn;
    unsigned char i = 0;

/// Displayed to the user in event of a parsing error, following the JSON object name.
#define FEEDBACK_JSON_OBJERROR_SUFFIX_NULL " is NULL (doesn't exist?), but a value is expected."
#define FEEDBACK_JSON_OBJERROR_SUFFIX_BOOL " is supposed to be boolean, but it is not."

    // Parse BOOL-type objects from the JSON and set them in the conf_Settings union

    // Make the first split/strtok call
    j_objn = strtok(JSON_ROOTOBJECTS_BOOL, " ");
    // Begin the iteration loop
    l_conf_LoadSettingsFromJSON__parseBoolObjects_a:
    // Check if we have reached the end of our expected objects list. If so, leave the loop
    if (j_objn == NULL)
        goto l_conf_LoadSettingsFromJSON__parseBoolObjects_b;
    // Get the object from the parsed JSON from one of the names defined in JSON_ROOTOBJECTS_BOOL
    j_tmp = cJSON_GetObjectItem(parsed, j_objn);
    // Error-check the type of the value
    if (!cJSON_IsBool(j_tmp))
        return strcat(j_objn, FEEDBACK_JSON_OBJERROR_SUFFIX_BOOL);
    // Store the status of the value
    conf_Settings.indx[i++] = cJSON_IsTrue(j_tmp);
    // Make another split/strtok call (this time NULL to continue) and loop
    j_objn = strtok(NULL, " ");
    goto l_conf_LoadSettingsFromJSON__parseBoolObjects_a;

    l_conf_LoadSettingsFromJSON__parseBoolObjects_b:
    
    // TODO: Parse array objects.

    // Return success status
    return NULL;
}

/// Reads the entire configuration file and returns a pointer to the resulting string.
/// If an error has occured, the first character will not be '{'.
char *conf_FileReadString(char* configuration_file) {
#define FEEDBACK_CONFIGFILE_OPENERR "\aCould not open the configuration file.\nCheck the location of your configuration file, and if you have permissions to it."
#define FEEDBACK_CONFIGFILE_EMPTY "\aThe configuration file was empty."
    // Open file for reading
    FILE *f_file;
    f_file = fopen(configuration_file, "r");
    if (f_file == NULL) {
        return FEEDBACK_CONFIGFILE_OPENERR;
    }

    // Variables used to dynamically allocate space for characters read
    char *f_final, f_char;
    unsigned short f_allocsize = 10, f_buffersize = 0;

    // Allocate beginning space and read JSON string from file
    f_final = malloc(f_allocsize * sizeof(char));
    while ((f_char = getc(f_file)) != -1) {
        f_final[f_buffersize] = f_char;
        // If we run out of buffer space, allocate more
        if (++f_buffersize + 1 > f_allocsize) {
            f_allocsize += 10;
            f_final = realloc(f_final, f_allocsize * sizeof(char));
        }
    }
    // Close the file
    fclose(f_file);
    // Check if the buffer is empty, which would mean the file is empty
    if (f_buffersize == 0) {
        return FEEDBACK_CONFIGFILE_EMPTY;
    }
    // Null termination for safety
    f_final[++f_buffersize] = 0;
    return f_final;
}

/**
 * Gets all entries in searchDir 
 * @param searchDir: The directory to search in
 * @param skip: Any files or directories to skip over when searching 
 * @return: A string with all files and directories found in the current directory
*/
char* dir_GetEntries(char *searchDir, char* skip) {
    DIR *pSearchDir;
    struct dirent* dirEntry;
    struct stat statBuf;    

    // Checks if the directory actually opened and outputs a message and exits the function if it didn't
    if ((pSearchDir = opendir(searchDir)) == NULL) {
        puts("Failed to open directory");
        return;
    }

    // Changes the current directory to the one to be searched
    chdir(searchDir);
    // Allocate initial byte
    dir_SearchEntries = malloc(1);
    unsigned int s_iter = 0, s_mem = 1;
    // Loops through the directory until the entire directory has been read (readdir returns a pointer to the next directory entry)
    while ((dirEntry = readdir(pSearchDir)) != NULL) {
        // Stats the directory name pointed to by dirEntry->d_name and puts the information in the buffer (statBuff)
        lstat(dirEntry->d_name, &statBuf);
        
        // Checks if file pointed to by statBuf is a directory
        if(S_ISDIR(statBuf.st_mode)) {
            // Checks if the directory entry is the current directory or the parent of the current directory
            if (strcmp(".", dirEntry->d_name) == 0 || strcmp("..", dirEntry->d_name) == 0)
            continue;
        }
        
        dir_SearchEntries = realloc(dir_SearchEntries, s_mem += ((strlen(dirEntry->d_name) + 1) * sizeof(char)));
        dir_SearchEntries[s_iter++] = dirEntry->d_name;
        // printf("%*s%s/\n", 5, "", strstr(dirEntry->d_name, skip));
    }
    dir_SearchEntries_c = s_iter;
    // Reopens the CWD
    int cwd = open(".", O_RDONLY);
    // Changes directory to the CWD
    fchdir(cwd);
    // Close the searchDir 
    closedir(pSearchDir);
    return NULL;
}


int main() {
#define FEEDBACK_PREAMBLE "+\nOpening Config... "
#define FEEDBACK_SUCCESS "success!"
#define FEEDBACK_FAILURE "failed!\n"
/// Path to the configuration file (relative to execution location).
#define CONFIG_PATH "config/config.json"
    fputs(FEEDBACK_PREAMBLE, stdout);
    
    char* stat = conf_FileReadString(CONFIG_PATH);
    // If reading failed, o[0] != '{'
    if (stat[0] != '{') {
        // Print the error and exit
        puts(strcat(FEEDBACK_FAILURE, stat));
        return 1;
    } 
    puts(FEEDBACK_SUCCESS);
    conf_Parsed = cJSON_Parse(stat);
    free(stat);
    stat = conf_LoadSettingsFromJSON(conf_Parsed);
    // If loading failed, o != NULL
    if (stat != NULL) {
        puts(strcat(FEEDBACK_FAILURE, stat));
        return 1;
    }

    // DEBUG: Print the settings table in full.
    for (int z = 0; z < 4; z++) {
        printf("boolsym %d\n", conf_Settings.indx[z]);
    }

    stat = dir_GetEntries(".", "out");
    // If searching failed, o != NULL
    if (stat != NULL) {
        puts(strcat(FEEDBACK_FAILURE, stat));
        return 1;
    }  
    
    // DEBUG: Print the search entries.
    for (int z = 0; z < dir_SearchEntries_c; z++) {
        printf("searche %s\n", dir_SearchEntries[z]);
    }

    // Check the filetype of those files and sort according to that
    // Check the filenames and sort according to that
    free(conf_Parsed);
    return 0;
}