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
#include <sys/sendfile.h>

typedef unsigned char uint8_t;

/// NOTE: 
/// Define all JSON objects expected (in the configuration file) in their respective groups here.
/// Make sure to line them up in order with conf_Settings_bool_t. MAKE SURE TO.
/// If a new type needs to be implemented, update conf_LoadSettingsFromJSON(cJSON *parsed) and struct conf_Settings
/// JSON_ROOTOBJECTS_ARRAYS_ARRAYS_STRING_TWO is an array of two-string size arrays. So good.
char JSON_ROOTOBJECTS_BOOL[] = "sortDirectories sortFiles searchSubDirectories sortByFileTypes sortByKeywords";
char JSON_ROOTOBJECTS_ARRAYS_STRING[] = "searchDirectories skipDirectoriesAndFiles";
char JSON_ROOTOBJECTS_ARRAYS_ARRAYS_STRING_TWO[] = "sortFileTypes sortGenericTerms";

// Really hacky, should work just fine, but just in case. (gcc attrib)
union conf_Settings_bool_t {
    __attribute__((__packed__))
    struct  {
        uint8_t sortDirectories : 1;
        uint8_t sortFiles : 1;
        uint8_t searchSubDirectories : 1;
        uint8_t sortByFileTypes : 1;
        uint8_t sortByKeywords : 1;
    };
    // Should have the same size as the amount of elements in the above struct.
    uint8_t indx[5];
} conf_Settings_bool_t;

union conf_Settings_array_sizes_t {
    __attribute__((__packed__))
    struct  {
        uint8_t searchDirectories;
        uint8_t skipDirectoriesAndFiles;
    };
    // Should have the same size as the amount of elements in the above struct.
    uint8_t indx[2];
} conf_Settings_array_sizes_t;

/// Used to index through conf_Settings.array_string[][][].
enum conf_Settings_array_string_indx_t {
    conf_Settings_array_string_indx_searchDirectories = 0,
    conf_Settings_array_string_indx_skipDirectoriesAndFiles = 1
} conf_Settings_array_string_indx_t;

struct conf_Settings_t {
    union conf_Settings_bool_t bool;
    char*** array_string;
    union conf_Settings_array_sizes_t array_sizes;
} conf_Settings_t;

/// The underscore variable is the value of the bit.
struct ubit_t {
    unsigned int _ : 1;
} ubit_t;

/// Stores the user defined settings after they are fully parsed from JSON. conf_Settings.indx[i] can be used to access elements by number.
struct conf_Settings_t conf_Settings;

/// Stores the loaded and parsed configuration JSON object.
struct cJSON *conf_Parsed;

/// Stores the entries in the user-specified search directory.
char** dir_SearchEntries;
/// Stores if the corresponding index in dir_SearchEntries is a directory or not (if dir_SearchEntries_d[i]._ == 1 or 0).
struct ubit_t* dir_SearchEntries_d;
unsigned short dir_SearchEntries_c;

/**
 * Sets internal variables based off the contents of the supplied JSON object.
 * @param parsed: The parsed JSON string from the configuration.
 * @return NULL if the operation has succeeded, otherwise an error message (which will need to be freed).
*/
char* conf_LoadSettingsFromJSON(cJSON *parsed) {
    // Check if the supplied object contains data.
    if (parsed == NULL)
        return "\aSupplied JSON object is NULL. Is something wrong with the configuration?";
    cJSON *j_tmp1, *j_tmp2;
    char *j_objn1, *j_objn2;
    unsigned char i, p, c;

/// Displayed to the user in event of a parsing error, following the JSON object name.
#define FEEDBACK_JSON_OBJERROR_SUFFIX_NULL         " is NULL (doesn't exist?), but a value is expected."
#define FEEDBACK_JSON_OBJERROR_SUFFIX_BOOL         " is supposed to be boolean, but it is not."
#define FEEDBACK_JSON_OBJERROR_SUFFIX_STRING       " is supposed to be a string, but it is not."
#define FEEDBACK_JSON_OBJERROR_SUFFIX_ARRAY        " is supposed to be an array, but it is not."
#define FEEDBACK_JSON_OBJERROR_SUFFIX_ARRAY_STRING " contains an item which did not load properly. Are all items of string type?"

    // Parse BOOL-type objects from the JSON and set them in the conf_Settings union
    // Make the first split/strtok call
    j_objn1 = strtok(JSON_ROOTOBJECTS_BOOL, " ");
    // Setup our index variable
    i = 0;

    // Begin the iteration loop
    l_conf_LoadSettingsFromJSON__parseBoolObjects_a:
    // Check if we have reached the end of our expected objects list. If so, leave the loop
    if (j_objn1 == NULL)
        goto l_conf_LoadSettingsFromJSON__parseBoolObjects_b;
    // Get the object from the parsed JSON from one of the names defined in JSON_ROOTOBJECTS_BOOL
    j_tmp1 = cJSON_GetObjectItem(parsed, j_objn1);
    // Error-check the type of the value
    if (!cJSON_IsBool(j_tmp1)) {
        size_t err_s = strlen(j_objn1) + 1;
        char* err = malloc((err_s * sizeof(char)) + sizeof(FEEDBACK_JSON_OBJERROR_SUFFIX_BOOL));
        memcpy(err, j_objn1, err_s * sizeof(char));
        return strcat(err, FEEDBACK_JSON_OBJERROR_SUFFIX_BOOL);
    }
    // Store the true/false status of the value
    conf_Settings.bool.indx[i++] = cJSON_IsTrue(j_tmp1);
    // Make another split/strtok call (this time NULL to continue) and loop
    j_objn1 = strtok(NULL, " ");
    goto l_conf_LoadSettingsFromJSON__parseBoolObjects_a;
    l_conf_LoadSettingsFromJSON__parseBoolObjects_b:

    // Really the same steps as before, but we're working with an array of strings.
    i = 0;
    j_objn1 = strtok(JSON_ROOTOBJECTS_ARRAYS_STRING, " ");
    // Allocate a buffer in the array_string
    conf_Settings.array_string = malloc(sizeof(char[1][1][1]));
    l_conf_LoadSettingsFromJSON__parseStringArrayObjects_a:

    if (j_objn1 == NULL)
        goto l_conf_LoadSettingsFromJSON__parseStringArrayObjects_b;

    j_tmp1 = cJSON_GetObjectItem(parsed, j_objn1);
    
    if (!cJSON_IsArray(j_tmp1)) {
        // Free the one byte in the array string
        free(conf_Settings.array_string);
        size_t err_s = strlen(j_objn1) + 1;
        char* err = malloc((err_s * sizeof(char)) + sizeof(FEEDBACK_JSON_OBJERROR_SUFFIX_ARRAY));
        memcpy(err, j_objn1, err_s * sizeof(char));
        return strcat(err, FEEDBACK_JSON_OBJERROR_SUFFIX_ARRAY);
    }
    
    // Store the array size
    conf_Settings.array_sizes.indx[i] = cJSON_GetArraySize(j_tmp1);
    p = 0;

    // Start the internal array loop
    l_conf_LoadSettingsFromJSON__parseStringArrayObjectsSpecific_a:

    // Break out if we run out of elements.
    if (p == conf_Settings.array_sizes.indx[i])
        goto l_conf_LoadSettingsFromJSON__parseStringArrayObjectsSpecific_b;

    // Index into the JSON object array
    j_tmp2 = cJSON_GetArrayItem(j_tmp1, p);

    // Error-check the resulting value
    if (j_tmp2 == NULL || !cJSON_IsString(j_tmp2)) {
        // Free the one byte in the array string
        free(conf_Settings.array_string);
        // Since j_objn2 is NULL, we don't have the string, and we simply say that one of the objects in the array failed to load.
        size_t err_s = strlen(j_objn1) + 1;
        char* err = malloc((err_s * sizeof(char)) + sizeof(FEEDBACK_JSON_OBJERROR_SUFFIX_ARRAY_STRING));
        memcpy(err, j_objn1, err_s * sizeof(char));
        return strcat(err, FEEDBACK_JSON_OBJERROR_SUFFIX_ARRAY_STRING);
    }
    j_objn2 = cJSON_GetStringValue(j_tmp2);
    c = (strlen(j_objn2) + 1) * sizeof(char);

    conf_Settings.array_string = realloc(conf_Settings.array_string, sizeof(char[i + 1][p + 1][c]));
    memcpy(conf_Settings.array_string[i][p], j_objn2, c);
    //printf("\nITERATION: %d, %d, %s\n", i, p, conf_Settings.array_string[i][p]);
    p++;
    goto l_conf_LoadSettingsFromJSON__parseStringArrayObjectsSpecific_a;
    l_conf_LoadSettingsFromJSON__parseStringArrayObjectsSpecific_b:

    i++;
    j_objn1 = strtok(NULL, " ");
    goto l_conf_LoadSettingsFromJSON__parseStringArrayObjects_a;
    l_conf_LoadSettingsFromJSON__parseStringArrayObjects_b:
    printf("\nFINAL: %d, %d, %s\n", i, p, conf_Settings.array_string[1][0]);
    // Return success status
    return NULL;
}

/**
 * Reads the configuration file pointed to by configuration_file.
 * @param configuration_file: The path to the configuration file.
 * @return A pointer to the resulting string. If an error has occured, the first character of the return string will not be a '{', and it will instead contain the corresponding error message.
*/
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
 * @return NULL if the operation succeeded, or a bell-character prefixed error message.
*/
char* dir_GetEntries(char *searchDir, char** skip, unsigned short int skip_size) {
    DIR *pSearchDir;
    struct dirent* dirEntry;
    struct stat statBuf;   

    printf("\n\nIN THE FUNCTION:\nData: %s\n\n", conf_Settings.array_string[conf_Settings_array_string_indx_skipDirectoriesAndFiles][0]);
    
    // Checks if the directory actually opened and outputs a message and exits the function if it didn't
    if ((pSearchDir = opendir(searchDir)) == NULL)
        return "\aFailed to open the search directory.";

    // FIXME: What? Where did it go?
    printf("\n\nIN THE FUNCTION:\nData: %s\n\n", conf_Settings.array_string[conf_Settings_array_string_indx_skipDirectoriesAndFiles][0]);

    // Changes the current directory to the one to be searched
    chdir(searchDir);
    // Allocate initial byte
    dir_SearchEntries = malloc(1);
    dir_SearchEntries_d = malloc(1);
    unsigned int s_iter = 0, s_mem = 1;
    unsigned short s_loop;
    // Loops through the directory until the entire directory has been read (readdir returns a pointer to the next directory entry)
    while ((dirEntry = readdir(pSearchDir)) != NULL) {
        // Stats the directory name pointed to by dirEntry->d_name and puts the information in the buffer (statBuff)
        lstat(dirEntry->d_name, &statBuf);
        for (s_loop = 0; s_loop < skip_size; s_loop++)
            if (strcmp(skip[s_loop], dirEntry->d_name) == 0)
                goto l_dir_GetEntries__omitSkipEntities_a;

        goto l_dir_GetEntries__omitSkipEntities_b;

        l_dir_GetEntries__omitSkipEntities_a:
        continue;
        
        l_dir_GetEntries__omitSkipEntities_b:

        // Checks if file pointed to by statBuf is a directory
        if(S_ISDIR(statBuf.st_mode)) {
            // Checks if the directory entry is the current directory or the parent of the current directory
            if (strcmp(".", dirEntry->d_name) == 0 || strcmp("..", dirEntry->d_name) == 0)
                continue;
            dir_SearchEntries_d[s_iter]._ = 1;
        } else
            dir_SearchEntries_d[s_iter]._ = 0;
        // 256 here is the size of dirent.d_name
        dir_SearchEntries = realloc(dir_SearchEntries, s_mem += (256 * sizeof(char)));
        // TODO: We allocate a little more memory then we need.
        dir_SearchEntries_d = realloc(dir_SearchEntries_d, s_iter + 2);
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

// TODO Change accordingly once parsing config arrays is added
/**
 * Sorts and moves the directory entries to their specified locations
 * @param sortEntries The entries in the search directory that are to be sorted
 * @param dirMoveTo The directories that the entries are to be moved to
 * @param sortKeywords
 * @param sortFileTypes
 * 
*/
void dir_sortEntries(
    char** sortEntries, 
    char* dirMoveTo, 
    char* sortKeywords, 
    char* sortFileTypes 
) {
    unsigned short iter;

    for (iter = 0; iter < dir_SearchEntries_c; iter++) {
        // If the config says to sort both directories and files
        if (conf_Settings.bool.sortDirectories && conf_Settings.bool.sortFiles)
           goto l_checkSortMethod;
           
        // If the config says to sort only directories 
        if (conf_Settings.bool.sortDirectories && !conf_Settings.bool.sortFiles)
            if (dir_SearchEntries_d[iter]._ == 1)
                goto l_checkSortMethod;
            else
                continue;

        // If the config says to sort only files
        if (!conf_Settings.bool.sortDirectories && conf_Settings.bool.sortFiles)
            if (dir_SearchEntries_d[iter]._ == 0)
                goto l_checkSortMethod;
            else
                continue;

        break;
        
        l_checkSortMethod:
        // If the config says to sort by both keywords and filetypes
        if (conf_Settings.bool.sortByKeywords && conf_Settings.bool.sortByFileTypes) {
            if (strcmp(sortEntries[iter], sortKeywords) == 0) {
                // Move the Entry to the directory specified
               
             //   if (rename(sortEntries[iter],  ) == -1)
                    
            }

            if (strcmp(sortEntries[iter], sortFileTypes) == 0) {
                // Move the Entry to the directory specified
            }
        }

        // If the config says to sort by only keywords
        if (conf_Settings.bool.sortByKeywords && !conf_Settings.bool.sortByFileTypes)
            if (strcmp(sortEntries[iter], sortKeywords) == 0) {
                // Move the Entry to the directory specified
            }                
    

        // If the config says to sort by only filetypes
        if (!conf_Settings.bool.sortByKeywords && conf_Settings.bool.sortByFileTypes)
            if (strcmp(sortEntries[iter], sortFileTypes) == 0) {
                // Move the Entry to the directory specified
            }            

        break;
    }
}

int main() {
#define FEEDBACK_OPENINGCONFIG "Opening configuration... "
#define FEEDBACK_PARSINGCONFIG "Parsing configuration... "
#define FEEDBACK_SEARCHING     "Searching in the directory... "
#define FEEDBACK_SUCCESS "success!"
#define FEEDBACK_FAILURE "failed!\n"

/// Path to the configuration file (relative to execution location).
#define CONFIG_PATH "config/config.json"

    fputs(FEEDBACK_OPENINGCONFIG, stdout);
    char* stat = conf_FileReadString(CONFIG_PATH);
    // If reading failed, o[0] != '{'
    if (stat[0] != '{') {
        // Print the error and exit
        fputs(FEEDBACK_FAILURE, stdout);
        puts(stat);
        return 1;
    } 
    puts(FEEDBACK_SUCCESS);
    fputs(FEEDBACK_PARSINGCONFIG, stdout);
    conf_Parsed = cJSON_Parse(stat);
    free(stat);
    stat = conf_LoadSettingsFromJSON(conf_Parsed);
    // If loading failed, o != NULL
    if (stat != NULL) {
        fputs(FEEDBACK_FAILURE, stdout);
        puts(stat);
        free(stat);
        return 1;
    }
    puts(FEEDBACK_SUCCESS);   
    fputs(FEEDBACK_SEARCHING, stdout);
    printf("\n\nOUTSIDE OF THE FUNCTION:\nSize: %d\nData: %s\n\n", conf_Settings.array_sizes.skipDirectoriesAndFiles, conf_Settings.array_string[conf_Settings_array_string_indx_skipDirectoriesAndFiles][0]); 
    char *skipd[] = {
        "Hello"
    };
    stat = dir_GetEntries(".", skipd, 1);//conf_Settings.array_string[conf_Settings_array_string_indx_skipDirectoriesAndFiles], conf_Settings.array_sizes.skipDirectoriesAndFiles);
    // If searching failed, o != NULL
    if (stat != NULL) {
        puts(FEEDBACK_FAILURE);
        puts(stat);
        return 1;
    }  
    puts(FEEDBACK_SUCCESS);

    puts(dir_SearchEntries[0]);

    // DEBUG: Print the search entries.
    // for (int z = 0; z < dir_SearchEntries_c; z++)
    //     printf("%s <%d>\n", dir_SearchEntries[z], dir_SearchEntries_d[z]._);


    // Check the filetype of those files and sort according to that
    // Check the filenames and sort according to that
    free(conf_Parsed);
    free(conf_Settings.array_string);
    free(dir_SearchEntries);
    free(dir_SearchEntries_d);
    return 0;
}