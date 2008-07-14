#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dirent.h>
#include <sys/stat.h>

#include "registry.h"
#include "utils.h"

void search_linux(const char* mountpoint) {
    DIR* dir;
    struct dirent *entry;
    struct stat st;
    char* fullpath = NULL;
    char* cwd;
    char* profile = NULL;
    int mult = 0;

    asprintf(&fullpath, "%s/home", mountpoint);
    dir = opendir(fullpath);
    if(!dir) {
        fprintf(stderr, "ma-search-users: Error.  Unable to open %s\n", 
            fullpath);
        exit(EXIT_FAILURE);
    }
    while((entry = readdir(dir)) != NULL) {
        asprintf(&profile, "%s/%s", fullpath, entry->d_name);
        if( -1 == stat(profile, &st)) {
            fprintf(stderr, "Unable to stat %s.\n", profile);
            free(profile);
        } else if(S_ISDIR(st.st_mode)) {
            free(profile);
            cwd = entry->d_name;
            if(strcmp(cwd,".") == 0 || strcmp(cwd,"..") == 0)
                continue;
            if(strcmp(cwd,"lost+found") == 0)
                continue;
            if(mult != 0)
                printf(", %s", cwd);
            else {
                printf(cwd);
                mult = 1;
            }
        }
    }
    free(fullpath);
    closedir(dir);
    puts("");
}

void search_windowsxp(const char* mountpoint) {
    DIR* dir;
    struct dirent *entry;
    struct stat st;
    char* cwd;
    char* profilesdir = NULL;
    char* systemreg = NULL;
    char* allusers = NULL;
    char* defaultuser = NULL;
    char* profile = NULL;
    int mult = 0;

    asprintf(&systemreg, "%s/WINDOWS/system32/config/software", mountpoint);
    profilesdir = findkey(systemreg, "\\Microsoft\\Windows NT\\CurrentVersion"
        "\\ProfileList\\ProfilesDirectory");
    allusers = findkey(systemreg, "\\Microsoft\\Windows NT\\CurrentVersion"
        "\\ProfileList\\AllUsersProfile");
    defaultuser = findkey(systemreg, "\\Microsoft\\Windows NT\\CurrentVersion"
        "\\ProfileList\\DefaultUserProfile");
    free(systemreg);
    if(!profilesdir) {
        fprintf(stderr, "ma-search-users: Error.  Could not find "
            "ProfilesDirectory.\n");
        exit(EXIT_FAILURE);
    }
    char* pdir = reformat_path(profilesdir);
    free(profilesdir);
    asprintf(&profilesdir, "%s/%s", mountpoint, pdir);
    free(pdir);
    fprintf(stderr, "profilesdir: %s\n", profilesdir);
    dir = opendir(profilesdir);
    if(!dir) {
        fprintf(stderr, "ma-search-users: Error.  Unable to open %s\n", 
            profilesdir);
        exit(EXIT_FAILURE);
    }
    while((entry = readdir(dir)) != NULL) {
        asprintf(&profile, "%s/%s", profilesdir, entry->d_name);
        if( -1 == stat(profile, &st)) {
            fprintf(stderr, "Unable to stat %s.\n", profile);
            free(profile);
        } else if(S_ISDIR(st.st_mode)) {
            free(profile);
            cwd = entry->d_name;
            if(strcmp(cwd,".") == 0 || strcmp(cwd,"..") == 0)
                continue;
            if(strcmp(cwd,allusers) == 0 || strcmp(cwd,defaultuser) == 0)
                continue;
            if(strcmp(cwd,"NetworkService") == 0 || strcmp(cwd,"LocalService") == 0)
                continue;
            if(mult != 0)
                printf(", %s", cwd);
            else {
                printf(cwd);
                mult = 1;
            }
        }
    }
    free(profilesdir);
    closedir(dir);
    puts("");
}

int main(int argc, char** argv) {
    char *ostype, *mountpoint;
    if(argc < 3) {
        fprintf(stderr, "%s OSTYPE MOUNTPOINT\n", argv[0]);
        return EXIT_FAILURE;
    }
    ostype = argv[1];
    mountpoint = argv[2];
    if(strcmp(ostype,"linux") == 0) search_linux(mountpoint);
    else if(strcmp(ostype,"windowsxp") == 0) search_windowsxp(mountpoint);
    else {
        fprintf(stderr, "ma-search-users: Error.  OS type '%s' is not"
            "understood.\n", ostype);
        return EXIT_FAILURE;
    }
    return 0;
}

// vim:ai:et:sts=4:tw=80:sw=4:
