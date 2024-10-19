#include <windows.h>
#include <unistd.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "nxjson.h"

time_t get_modification_time(const char *filename) {
    struct stat attr;
    if (stat(filename, &attr) == 0) {
        return attr.st_mtime;
    } else {
        perror("stat");
        exit(EXIT_FAILURE);
    }
}

int setup(const char *name, const char *url, const char *branch) {
    int ok;
    DIR *dir = opendir(name);
    if(dir) { // directory exists, already cloned
        closedir(dir);
        chdir(name);
        ok = system("git pull") == 0;
        ok = system("git submodule update --remote --recursive") == 0;
        dir = opendir("build");
        if(dir) {
            closedir(dir);
            if(get_modification_time("CMakeLists.txt") > get_modification_time("build/CMakeCache.txt")) 
            { // needs to be reconfigured
                system("rmdir /s /q \"build\"");
                ok = ok && system("cmake -S . -B build") == 0;
            }
        } else if(ENOENT == errno) {
            ok = ok && system("cmake -S . -B build") == 0;
        }
    } else if(ENOENT == errno) {
        char command[500];
        sprintf(command, "git clone %s -b %s --recursive", url, branch);
        ok = system(command) == 0;
        chdir(name);
        ok = ok && system("cmake -S . -B build") == 0;
    } else {
        return 0;
    }
    return ok;
}

int build() {
    return system("cmake --build build") == 0;
}
int launch(const char *name) {
    return system(name) == 0;
}


char *read_file_as_null_terminated_string(const char *file_path) {
    FILE *file = fopen(file_path, "rb");  // Open the file in binary mode
    if (file == NULL) {
        perror("Error opening file");
        return NULL;
    }

    // Move to the end of the file to get the size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);  // Move back to the start of the file

    // Allocate memory for the file contents plus null terminator
    char *buffer = (char *)malloc(file_size + 1);
    if (buffer == NULL) {
        perror("Error allocating memory");
        fclose(file);
        return NULL;
    }

    // Read the file into the buffer
    size_t bytes_read = fread(buffer, 1, file_size, file);
    if (bytes_read != file_size) {
        perror("Error reading file");
        free(buffer);
        fclose(file);
        return NULL;
    }

    // Null terminate the string
    buffer[bytes_read] = '\0';

    fclose(file);  // Close the file
    return buffer;  // Return the buffer containing the null-terminated string
}

int main(void) {
    char *buffer = read_file_as_null_terminated_string("launch.json");
    nx_json const *json = nx_json_parse_utf8(buffer);

    if(!json) {
        perror("failed to parce json!");
        return EXIT_FAILURE;
    }

    const char *name =      nx_json_get(json, "name")->text_value;
    const char *repo =      nx_json_get(json, "repo")->text_value;
    const char *branch =    nx_json_get(json, "branch")->text_value;
    const char *executable= nx_json_get(json, "executable")->text_value;
    
    
    printf("name: %s\n", name);
    printf("repo: %s\n", repo);
    printf("branch: %s\n", branch);
    printf("executable: %s\n", executable);

    int ok = setup(name, repo, branch);
    ok = ok && build();
    ok = ok && launch(executable);
    
    system("pause");
    free(buffer);
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
