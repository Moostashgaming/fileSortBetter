# FileSort

A utility configured in JSON and used to sort loose files from one directory of the user's choice into another directory of the user's choice.

### Cloning & Building
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

    ],
    "skipDirectoriesAndFiles" : [
        "file",
        "directory/"
    ],
    "sortFileTypes" : 
    [
        [
            "fileType", "directorySortTo"
        ],

        [
            "fileType", "directorySortTo"
        ]
    ],
    "sortGenericTerms" : 
    [
        [
            "term", "directorySortTo"
        ],

        [
            "term", "directorySortTo"
        ]
    ],
    "sortDirectories" : true,
    "searchSubDirectories" : false,
    "sortByFileType" : true,
    "sortByKeywords" : true
}
```


### Coming soon:
- A working program
- Choice between exact and keyword based skip-term(s) matching 

