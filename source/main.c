#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <3ds.h>

#define MAX_PATH 1024

static int files_deleted = 0;
static int folders_deleted = 0;
static int scanned_items = 0;

/* ---------------- PATH ---------------- */

static void build_path(char *out, size_t size, const char *base, const char *name) {
    size_t base_len = strlen(base);
    size_t name_len = strlen(name);

    if (base_len + name_len + 2 >= size) {
        strncpy(out, base, size - 1);
        out[size - 1] = '\0';
        return;
    }

    if (base_len > 0 && base[base_len - 1] == '/') {
        snprintf(out, size, "%s%s", base, name);
    } else {
        snprintf(out, size, "%s/%s", base, name);
    }
}

/* ---------------- FILTERS ---------------- */

static int is_junk_file(const char *name) {
    return (strncmp(name, "._", 2) == 0 ||
            strcmp(name, ".DS_Store") == 0 ||
            strcmp(name, ".ds_store") == 0);
}

static int is_junk_dir(const char *name) {
    return (strcmp(name, ".fseventsd") == 0 ||
            strcmp(name, ".Trashes") == 0 ||
            strcmp(name, ".trashes") == 0 ||
            strcmp(name, ".Spotlight-V100") == 0 ||
            strcmp(name, ".TemporaryItems") == 0);
}

/* ---------------- RECURSIVE DELETE ---------------- */

static void remove_dir_recursively(const char *path) {
    DIR *dir = opendir(path);
    if (!dir)
        return;

    struct dirent *ent;

    while ((ent = readdir(dir)) != NULL) {

        if (strcmp(ent->d_name, ".") == 0 ||
            strcmp(ent->d_name, "..") == 0)
            continue;

        char full[MAX_PATH];
        build_path(full, sizeof(full), path, ent->d_name);

        struct stat st;
        if (stat(full, &st) != 0)
            continue;

        if (S_ISDIR(st.st_mode)) {
            remove_dir_recursively(full);
        } else {
            if (unlink(full) == 0) {
                files_deleted++;
                printf("Deleted file: %s\n", full);
            }
        }
    }

    closedir(dir);

    if (rmdir(path) == 0) {
        folders_deleted++;
        printf("Deleted folder: %s\n", path);
    } else {
        printf("rmdir failed (%d): %s\n", errno, path);
    }
}

/* ---------------- STACK ---------------- */

typedef struct {
    char path[MAX_PATH];
} DirStack;

static DirStack stack[1024];
static int stack_top = 0;

static void push(const char *path) {
    if (stack_top >= 1024)
        return;

    strncpy(stack[stack_top].path, path, MAX_PATH - 1);
    stack[stack_top].path[MAX_PATH - 1] = '\0';
    stack_top++;
}

static void scan_and_delete(const char *root) {
    push(root);

    while (stack_top > 0) {

        char current[MAX_PATH];
        strcpy(current, stack[--stack_top].path);

        DIR *dir = opendir(current);
        if (!dir)
            continue;

        struct dirent *ent;

        while ((ent = readdir(dir)) != NULL) {

            if (strcmp(ent->d_name, ".") == 0 ||
                strcmp(ent->d_name, "..") == 0)
                continue;

            scanned_items++;

            char full[MAX_PATH];
            build_path(full, sizeof(full), current, ent->d_name);

            int is_dir = (ent->d_type == DT_DIR);

            if (ent->d_type != DT_DIR && ent->d_type != DT_REG) {
                struct stat st;
                if (stat(full, &st) == 0)
                    is_dir = S_ISDIR(st.st_mode);
                else
                    continue;
            }

            if (is_dir) {

                if (is_junk_dir(ent->d_name)) {

                    printf("Removing junk folder: %s\n", full);
                    remove_dir_recursively(full);

                } else {

                    push(full);

                }

            } else {

                if (is_junk_file(ent->d_name)) {

                    if (unlink(full) == 0) {
                        files_deleted++;
                        printf("Deleted file: %s\n", full);
                    } else {
                        printf("unlink failed (%d): %s\n", errno, full);
                    }
                }
            }
        }

        closedir(dir);
    }
}

/* ---------------- MAIN ---------------- */

int main() {
    gfxInitDefault();
    consoleInit(GFX_TOP, NULL);

    printf("--- dotclean3ds v1.3 by kate ---\n");
    printf("A = START CLEANING\n");
    printf("START = EXIT\n");

    bool running = false;

    while (aptMainLoop()) {

        hidScanInput();
        u32 kDown = hidKeysDown();

        if (kDown & KEY_START)
            break;

        if ((kDown & KEY_A) && !running) {

            running = true;

            consoleClear();

            printf("Scanning + deleting sdmc:/ ...\n\n");

            files_deleted = 0;
            folders_deleted = 0;
            scanned_items = 0;
            stack_top = 0;

            scan_and_delete("sdmc:/");

            printf("\nDONE\n");
            printf("Scanned: %d\n", scanned_items);
            printf("Files deleted: %d\n", files_deleted);
            printf("Folders deleted: %d\n", folders_deleted);

            printf("\nPress START to exit\n");
        }

        gfxFlushBuffers();
        gfxSwapBuffers();
        gspWaitForVBlank();
    }

    gfxExit();
    return 0;
}
