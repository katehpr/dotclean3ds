#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <3ds.h>

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
            }
        }
    }
    closedir(dir);
    rmdir(path); // Delete the folder now that it's empty
}

// Main recursive function to scan the SD card
void clean_directory(const char* path) {
    DIR* dir = opendir(path);
    if (!dir) return;

    struct dirent* ent;
    while ((ent = readdir(dir)) != NULL) {
        // Skip current and parent directory pointers
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) {
            continue;
        }

        // Construct the full path
        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, ent->d_name);

        struct stat st;
        if (stat(full_path, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                // Check if it's a macOS junk folder
                if (strcmp(ent->d_name, ".fseventsd") == 0 || 
                    strcmp(ent->d_name, ".Trashes") == 0 || 
                    strcmp(ent->d_name, ".trashes") == 0 ||
                    strcmp(ent->d_name, ".Spotlight-V100") == 0) {
                    
                    printf("Deleting folder: %s\n", ent->d_name);
                    remove_dir_recursively(full_path);
                } else {
                    // It's a normal folder, keep searching inside it
                    clean_directory(full_path);
                }
            } else {
                // It's a file, check if it's macOS junk
                if (strncmp(ent->d_name, "._", 2) == 0 || 
                    strcmp(ent->d_name, ".DS_Store") == 0 || 
                    strcmp(ent->d_name, ".ds_store") == 0) {
                    
                    printf("Deleting file: %s\n", ent->d_name);
                    unlink(full_path); 
                }
            }
        }
    }
    closedir(dir);
}

int main(int argc, char **argv) {
    // Initialize the graphics and console
    gfxInitDefault();
    consoleInit(GFX_TOP, NULL);

    // Draw the UI menu
    printf("\x1b[1;1H--- dotclean3ds ---");
    printf("\x1b[2;1Hby kate");
    
    printf("\x1b[4;1HThis will recursively delete:");
    printf("\x1b[5;1H- '._*' ghost files");
    printf("\x1b[6;1H- '.DS_Store' files");
    printf("\x1b[7;1H- '.Trashes' folders");
    printf("\x1b[8;1H- '.fseventsd' folders");
    printf("\x1b[9;1H- '.Spotlight-V100' folders");
    
    printf("\x1b[11;1HPress A to start cleaning.");
    printf("\x1b[12;1HPress START to exit.");

    bool is_done = false;

    // Main loop
    while (aptMainLoop()) {
        // Scan all inputs
        hidScanInput();
        u32 kDown = hidKeysDown();

        if (kDown & KEY_START) {
            break; // Break loop to exit
        }

        if ((kDown & KEY_A) && !is_done) {
            // Moved this down to row 14 so it doesn't overwrite the menu
            printf("\x1b[14;1HCleaning... Please wait.\n\n");
            
            // Force console to update before blocking operation
            gfxFlushBuffers();
            gfxSwapBuffers();
            gspWaitForVBlank();

            // Start cleaning from the root of the SD card
            clean_directory("sdmc:/");

            printf("\nDone! Your SD card is clean.\n");
            printf("Press START to exit.\n");
            is_done = true;
        }

        // Flush and swap framebuffers
        gfxFlushBuffers();
        gfxSwapBuffers();
        // Wait for VBlank
        gspWaitForVBlank();
    }

    // Exit graphics
    gfxExit();
    return 0;
}