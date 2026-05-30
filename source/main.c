#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <3ds.h>

// Global counters for our summary
int files_deleted = 0;
int folders_deleted = 0;

// Helper function to completely empty and delete a folder
void remove_dir_recursively(const char* path) {
    DIR* dir = opendir(path);
    if (!dir) return;

    struct dirent* ent;
    while ((ent = readdir(dir)) != NULL) {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) {
            continue;
        }

        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, ent->d_name);

        struct stat st;
        if (stat(full_path, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                remove_dir_recursively(full_path);
            } else {
                unlink(full_path);
                files_deleted++;
            }
        }
    }
    closedir(dir);
    rmdir(path); 
    folders_deleted++;
}

// Main recursive function to scan the SD card
void clean_directory(const char* path) {
    DIR* dir = opendir(path);
    if (!dir) return;

    struct dirent* ent;
    while ((ent = readdir(dir)) != NULL) {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) {
            continue;
        }

        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, ent->d_name);

        struct stat st;
        if (stat(full_path, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                if (strcmp(ent->d_name, ".fseventsd") == 0 || 
                    strcmp(ent->d_name, ".Trashes") == 0 || 
                    strcmp(ent->d_name, ".trashes") == 0 ||
                    strcmp(ent->d_name, ".Spotlight-V100") == 0 ||
                    strcmp(ent->d_name, ".TemporaryItems") == 0) {
                    
                    printf("Deleting folder: %s\n", full_path);
                    remove_dir_recursively(full_path);
                } else {
                    clean_directory(full_path);
                }
            } else {
                if (strncmp(ent->d_name, "._", 2) == 0 || 
                    strcmp(ent->d_name, ".DS_Store") == 0 || 
                    strcmp(ent->d_name, ".ds_store") == 0) {
                    
                    printf("Deleting file: %s\n", full_path);
                    unlink(full_path); 
                    files_deleted++;
                }
            }
        }
    }
    closedir(dir);
}

int main(int argc, char **argv) {
    gfxInitDefault();
    consoleInit(GFX_TOP, NULL);

    // Updated title with v1.1
    printf("\x1b[1;1H--- dotclean3ds v1.1 ---");
    printf("\x1b[2;1Hby kate");
    
    printf("\x1b[4;1HThis will recursively delete:");
    printf("\x1b[5;1H- '._*' & '.DS_Store' files");
    printf("\x1b[6;1H- '.Trashes' folders");
    printf("\x1b[7;1H- '.fseventsd' folders");
    printf("\x1b[8;1H- '.Spotlight-V100' folders");
    printf("\x1b[9;1H- '.TemporaryItems' folders");
    
    printf("\x1b[11;1HPress A to start cleaning.");
    printf("\x1b[12;1HPress START to exit.");

    bool is_done = false;

    while (aptMainLoop()) {
        hidScanInput();
        u32 kDown = hidKeysDown();

        if (kDown & KEY_START) {
            break; 
        }

        if ((kDown & KEY_A) && !is_done) {
            consoleClear();
            printf("Cleaning... Please wait.\n\n");
            
            gfxFlushBuffers();
            gfxSwapBuffers();
            gspWaitForVBlank();

            files_deleted = 0;
            folders_deleted = 0;

            clean_directory("sdmc:/");

            printf("\nDone! Your SD card is clean.\n");
            printf("Removed %d files and %d folders.\n\n", files_deleted, folders_deleted);
            printf("Press START to exit.\n");
            is_done = true;
        }

        gfxFlushBuffers();
        gfxSwapBuffers();
        gspWaitForVBlank();
    }

    gfxExit();
    return 0;
}