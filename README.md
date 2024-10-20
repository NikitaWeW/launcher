Small c tool to install the git repository, configure it with cmake and build controlled by json configuration. uses nxjson to parse launch.json. Targets windows.

## Building
No specific build process requiered. Just build all *.c files:
``` shell
gcc *.c -o out/launcher --static
```
There also is a Makefile if you need it

## Json variables
| name | description |
| --- | --- |
| repo | repository url|
| branch | git branch to clone |
| executable | executable to launch after building |
| additional packages | msys packages to install (not including gcc. cmake, etc.) |
---