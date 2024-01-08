# FileSort

A utility configured in JSON and used to sort loose files or directories from one directory of the user's choice into another directory of the user's choice.

### Cloning & Building (and installing)
```bash
# Using a git client:
git clone https://github.com/moostashylxd/FileSort.git

# Using GitHub CLI:
gh repo clone moostashylxd/FileSort

cd FileSort
sh runfile

# The executable will be located in the build directory. (./build/out)
# To install the executable to PATH:
sh install
# This will copy /build/out to /usr/bin/filesort.
```

### Configuration
The default configuration is located at ./config/config.json.
<br>It is also below (with illegal comments):
```json
{
    /* Place directories you wish to sort in here (Exact path) */
    "searchDirectories" : [
        "/directory/"
    ],
    
    /* Place files and directories that you would like the program to leave alone and not sort here */
    "skipDirectoriesAndFiles" : [
        "file",
        "directory/"
    ],

    /* Place the filetype extension and which directory you would like those file types sorted to in here */
    "sortFileTypes" : 
    [
        /* You may add as many of these as you would like as long as they adhere to the convention displayed below */
        [
            "fileType", "directorySortTo/"
        ],

        [
            "fileType", "directorySortTo/"
        ]
    ],

    /* Place generic terms for the program to sort by and the directory to send files/directories whose names contain those terms to (Fuzzy finding not currently supported) */
    "sortGenericTerms" : 
    [
        /* You may add as many of  these as you like, as long as they adhere to the convention displayed below */
        [
            "term", "directorySortTo/"
        ],

        [
            "term", "directorySortTo/"
        ]
    ],

    /* Whether or not to sort any directories found in the search directory */
    "sortDirectories" : true,
    
    /* Whether or not to sort any files found in the search directory */
    "sortFiles" : true,

    /* Whether or not to search through sub directories in the search directory */
    "searchSubDirectories" : false,

    /* Whether or not to sort by the instructions laid out in "sortFileTypes" */
    "sortByFileTypes" : true,

    /* Whether or not to sort by the instructions laid out in "sortGenericTerms" */
    "sortByKeywords" : true
}
```


### Coming soon:
- A working program
- Choice between exact and keyword based skip-term(s) matching 

