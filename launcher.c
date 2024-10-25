#include <windows.h>
#include <unistd.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "nxjson.h"

// buffers
char cuwd[1024];
char unix_path[1024];
char windows_path[1024];

char msys_dir[1024];

char *convert_to_unix_path(const char *windows_path) {
    if (windows_path[1] == ':') {
        // Convert "C:\path" to "/c/path"
        unix_path[0] = '/';
        unix_path[1] = tolower(windows_path[0]);  // Convert drive letter to lowercase
        unix_path[2] = '\0';  // Ensure string ends here
        strcat(unix_path, &windows_path[2]);  // Append the rest of the path (skip "C:")
    } else {
        strcpy(unix_path, windows_path);  // If not a drive path, just copy the string
    }

    // Replace backslashes with forward slashes
    for (int i = 0; unix_path[i] != '\0'; i++) {
        if (unix_path[i] == '\\') {
            unix_path[i] = '/';
        }
    }

    return unix_path;
}
char *cwd() {
    getcwd(cuwd, sizeof(cuwd));
    return cuwd;
}
char *ucwd() {
    getcwd(cuwd, sizeof(cuwd));
    return convert_to_unix_path(cuwd);
}
time_t get_modification_time(const char *filename) {
    struct stat attr;
    if (stat(filename, &attr) == 0) {
        return attr.st_mtime;
    } else {
        perror("stat");
        exit(EXIT_FAILURE);
    }
}
int exists(const char *name) {
  struct stat buffer;
  return stat(name, &buffer) == 0;
}
int msys(const char *cmd) {
    char command[1024];
    snprintf(command, sizeof(command), "%s/usr/bin/bash.exe -lc \"export PATH=/mingw64/bin:$PATH; cd %s; %s\"", msys_dir, ucwd(), cmd);
    return system(command);
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
int is_package_installed(const char *package) {
    char command[256];
    snprintf(command, sizeof(command), 
             "pacman -Q \"%s\" > /dev/null 2>&1", 
             package);
    int result = msys(command);
    return result == 0;  // Returns 1 if installed, 0 if not
}
void install_package(const char *package) {
    if (is_package_installed(package)) {
        printf("%s is already installed.\n", package);
        return;
    }

    printf("Installing %s...\n", package);
    char command[256];
    snprintf(command, sizeof(command), 
             "%s\\usr\\bin\\pacman -S --needed --noconfirm %s", 
             msys_dir,
             package);

    int result = system(command);
    if (result != 0) {
        fprintf(stderr, "Failed to install %s\n", package);
        exit(EXIT_FAILURE);
    }
    printf("%s installed successfully.\n", package);
}
void add_to_path(const char *var) {
    char currentPath[4096];
    GetEnvironmentVariable("PATH", currentPath, sizeof(currentPath));

    char newPath[4096];
    snprintf(newPath, sizeof(newPath), "%s%s", var, currentPath);

    SetEnvironmentVariable("PATH", newPath);
}

int setup_repository(const char *url, const char *branch) {
    int ok;
    if(exists("repository")) { // directory exists, already cloned
        chdir("repository");
        ok = msys("git pull -j 4 --autostash") == 0;
        ok = msys("git submodule update --remote --recursive -j 4") == 0;
    } else if(ENOENT == errno) {
        char command[1024];
        sprintf(command, "git clone %s repository -b %s --recursive", url, branch);
        ok = msys(command) == 0;
        chdir("repository");
    } else {
        return 0;
    }
    return ok;
}
int setup_wcmake() {
    int ok = 1;
    if(exists("build")) {
        if(get_modification_time("CMakeLists.txt") > get_modification_time("build/CMakeCache.txt")) 
        { // needs to be reconfigured
            system("rmdir /s /q \"build\"");
            ok = ok && msys("cmake -S . -B build") == 0;
        }
    } else if(ENOENT == errno) {
        ok = ok && msys("cmake -S . -B build") == 0;
    }
    return ok;
}
int build_wcmake() {
    return msys("cmake --build build") == 0;
}
int launch(const char *name) {
    return system(name) == 0;
}

int main(int argc, char **argv) {
    char const *json_file;
    if(argc == 1) {
        json_file = "../launch.json";
    } else {
        json_file = argv[1];
    }

    mkdir("launcher");
    chdir("launcher");

    if(!exists(json_file)) {
        perror("launch file not found!\n");
        system("pause");
        return EXIT_FAILURE;
    }

    char *buffer = read_file_as_null_terminated_string(json_file);
    nx_json const *json = nx_json_parse_utf8(buffer);

    if(!json) {
        perror("failed to parce json!\n");
        system("pause");
        return EXIT_FAILURE;
    }

    nx_json const* repo_in         = nx_json_get(json, "repo");
    nx_json const* branch_in       = nx_json_get(json, "branch");
    nx_json const* executable_in   = nx_json_get(json, "executable");
    nx_json const* msys_dir_in     = nx_json_get(json, "msys path");
    nx_json const* packages        = nx_json_get(json, "additional packages");
    nx_json const* custom_commands = nx_json_get(json, "custom build commands");

    if(!repo_in) {
        perror("repo is requiered!\n");
        system("pause");
        return EXIT_FAILURE;
    }
    if(!branch_in) {
        perror("branch is requiered!\n");
        system("pause");
        return EXIT_FAILURE;
    }
    if(!executable_in) {
        perror("executable is requiered!\n");
        system("pause");
        return EXIT_FAILURE;
    }

    char const* repo       = repo_in       -> text_value;
    char const* branch     = branch_in     -> text_value;
    char const* executable = executable_in -> text_value;

    printf("repo: %s on branch %s\n", repo, branch);

    if(msys_dir_in) {
        snprintf(msys_dir, sizeof(msys_dir), msys_dir_in->text_value); 
    } else {
        snprintf(msys_dir, sizeof(msys_dir), "C:\\msys64"); 
    }
    
    if(!exists(msys_dir)) {
        printf("downloading msys installer...\n");
        system("curl https://repo.msys2.org/distrib/msys2-x86_64-latest.exe -o msys2-x86_64-latest.exe");
        printf("installing msys...\n");
        char command[1024];
        snprintf(command, sizeof(command), ".\\msys2-x86_64-latest.exe in --confirm-command --accept-messages --root %s", msys_dir);
        system(command);
    }
    char msys_path[1024];
    snprintf(msys_path, sizeof(msys_path), "%s\\mingw64\\bin;%s\\usr\\bin;%s;", msys_dir, msys_dir, msys_dir);
    add_to_path(msys_path);

    if(!exists("repository/build")) {
        for(int i = 0; i < packages->children.length; ++i) {
            install_package(nx_json_item(packages, i)->text_value);
        }
        install_package("git");
        if(!custom_commands) {
            install_package("mingw-w64-x86_64-gcc");
            install_package("mingw-w64-x86_64-cmake");
            install_package("mingw-w64-x86_64-ninja");
        }
    }

    int ok = setup_repository(repo, branch);
    if(!custom_commands) {
        ok = ok && setup_wcmake();
        ok = ok && build_wcmake();
    }
    else {
        for(int i = 0; i < custom_commands->children.length; ++i) {
            msys(nx_json_item(custom_commands, i)->text_value);
        }
    }
    
    ok = ok && launch(executable);
    nx_json_free(json);
    free(buffer);
    system("pause");
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
