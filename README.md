Small c tool to install the git repository, configure it with cmake and build controlled by json configuration. uses nxjson to parse launch.json. Targets windows.

## usage
first, download launcher [here](github.com/NikitaWeW/launcher/releases/latest) or build it (see [building](#building)).
then, get configuration. for example:
``` json
{
    "repo": "project url",
    "branch": "main",
    "executable": "build\\main.exe",
    "additional packages": [ // optional
        "python"
    ]
}
```

paste it in any file and then open that file with launcher, you could also name this file `launch.json` in the same dir as launcher executable and double click launcher. 

done!

## Building
No specific build process requiered. Just build all *.c files:
``` shell
gcc *.c -o out/launcher --static
```
There also is a Makefile if you need it:
``` shell
# in root directory
make
```

## Json variables
| name | description |
| --- | --- |
| repo | repository url|
| branch | git branch to clone |
| executable | executable to launch after building |
| additional packages **optional** | msys packages to install (not including gcc. cmake, etc.) |
| msys path | **optional** path to msys root folder |
| custom build commands **optional** | msys shell commands to override build process |
---