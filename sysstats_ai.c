#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <ctype.h>
#include <unistd.h>
#include <stdint.h>

#ifdef _WIN32
#define DIR_SEP '\\'
#else
#define DIR_SEP '/'
#endif

/* ---------- Code Prompt 1 ---------- */
static char *join_path(const char *dir, const char *name) {
    size_t len = strlen(dir);
    size_t nlen = strlen(name);
    int need_sep = (len > 0 && dir[len - 1] != DIR_SEP);
    size_t total = len + (need_sep ? 1 : 0) + nlen + 1;
    char *p = malloc(total);
    if (!p) return NULL;
    if (need_sep)
        snprintf(p, total, "%s%c%s", dir, DIR_SEP, name);
    else
        snprintf(p, total, "%s%s", dir, name);
    return p;
}
static const char *file_type_string(mode_t m) {
    if (S_ISREG(m)) return "regular file";
    if (S_ISDIR(m)) return "directory";
    if (S_ISLNK(m)) return "symlink";
    if (S_ISCHR(m)) return "char device";
    if (S_ISBLK(m)) return "block device";
    if (S_ISFIFO(m)) return "FIFO/pipe";
    if (S_ISSOCK(m)) return "socket";
    return "unknown";
}
static void human_readable(uintmax_t size, char *buf, size_t bufsz) {
    const char *units[] = {"B","KB","MB","GB","TB","PB"};
    size_t i = 0;
    double s = (double)size;
    while (s >= 1024.0 && i < 5) {
        s /= 1024.0;
        i++;
    }
    snprintf(buf, bufsz, "%.2f %s", s, units[i]);
}

/* ---------- Code Prompt 2 ---------- */
static int has_txt_extension(const char *name) {
    size_t len = strlen(name);
    if (len < 4) return 0;
    const char *ext = name + len - 4;
    return (ext[0] == '.' &&
            tolower(ext[1]) == 't' &&
            tolower(ext[2]) == 'x' &&
            tolower(ext[3]) == 't');
}

/* ---------- Code Prompt 3 ---------- */
static char *join_path_alloc(const char *dir, const char *name) {
   size_t ld = strlen(dir), ln = strlen(name);
    if (ld > SIZE_MAX - ln - 2) return NULL;
    size_t need = ld + 1 + ln + 1;
    char *p = malloc(need);
    if (!p) return NULL;
    snprintf(p, need, "%s/%s", dir, name);
    return p;
}
static char *read_symlink_target_alloc(const char *path) {
    ssize_t r;
    size_t buf = 128;
    char *t = NULL;
    while (1) {
        char *tmp = realloc(t, buf + 1);
        if (!tmp) { free(t); return NULL; }
        t = tmp;
        r = readlink(path, t, buf);
        if (r < 0) { free(t); return NULL; }
        if ((size_t)r <= buf) { t[r] = '\0'; return t; }
        buf *= 2;
    }
}

/* ========================================================= */
/* ==================== MODE 1 CODE ======================== */
/* ========================================================= */
void run_mode_1(const char *dirpath) {
    DIR *dir = opendir(dirpath);
    if (!dir) {
        fprintf(stderr, "Error: %s\n", strerror(errno));
        return;
    }
    struct dirent *entry;
    size_t cap = 0, count = 0;
    char **names = NULL;
    while ((entry = readdir(dir)) != NULL) {
        if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
            continue;
        if (count + 1 > cap) {
            size_t newcap = cap ? cap * 2 : 64;
            names = realloc(names, newcap * sizeof(char*));
            cap = newcap;
        }
        names[count++] = strdup(entry->d_name);
    }
    closedir(dir);
    qsort(names, count, sizeof(char*), (int (*)(const void*, const void*))strcmp);
    unsigned long long totalSize = 0;
    unsigned int fileCount = 0;
    for (size_t i = 0; i < count; i++) {
        char *path = join_path(dirpath, names[i]);
        struct stat st;
        stat(path, &st);
        char mod[64];
        strftime(mod, sizeof(mod), "%Y-%m-%d %H:%M:%S", localtime(&st.st_mtime));
        char hr[32];
        human_readable(st.st_size, hr, sizeof(hr));
        printf("Name: %s\n", names[i]);
        printf("Type: %s\n", file_type_string(st.st_mode));
        printf("Size: %llu (%s)\n", (unsigned long long)st.st_size, hr);
        printf("Last Modified: %s\n\n", mod);
        if (S_ISREG(st.st_mode)) {
            fileCount++;
            totalSize += st.st_size;
        }
        free(path);
        free(names[i]);
    }
    free(names);
    printf("Total regular files: %u\n", fileCount);
    printf("Total cumulative size: %llu bytes\n", totalSize);
}

/* ========================================================= */
/* ==================== MODE 2 CODE ======================== */
/* ========================================================= */
void run_mode_2(const char *dirpath) {
    DIR *dir = opendir(dirpath);
    if (!dir) return;
    struct dirent *entry;
    struct stat st;
    char path[PATH_MAX];
    unsigned long long total_txt = 0, largest_size = 0;
    unsigned int txt_count = 0;
    char *largest = NULL;
    while ((entry = readdir(dir)) != NULL) {
        snprintf(path, sizeof(path), "%s/%s", dirpath, entry->d_name);
        stat(path, &st);
        if (S_ISREG(st.st_mode)) {
            if (!largest || st.st_size > largest_size) {
                free(largest);
                largest = strdup(entry->d_name);
                largest_size = st.st_size;
            }
            if (has_txt_extension(entry->d_name)) {
                char mod[64];
                strftime(mod, sizeof(mod), "%Y-%m-%d %H:%M:%S", localtime(&st.st_mtime));
                printf("TXT File: %s\n", entry->d_name);
                printf("Size: %llu bytes\n", (unsigned long long)st.st_size);
                printf("Last Modified: %s\n\n", mod);
                txt_count++;
                total_txt += st.st_size;
            }
        }
    }
    closedir(dir);
    printf("Total .txt files: %u\n", txt_count);
    printf("Total size of .txt files: %llu bytes\n", total_txt);
    if (largest)
        printf("Largest file: %s (%llu bytes)\n", largest, largest_size);
    free(largest);
}

/* ========================================================= */
/* ==================== MODE 3 CODE ======================== */
/* ========================================================= */
void run_mode_3(const char *dirpath) {
    DIR *dir = opendir(dirpath);
    if (!dir) return;
    struct dirent *entry;
    struct stat st;
    unsigned long long totalSize = 0;
    unsigned int count = 0;
    while ((entry = readdir(dir)) != NULL) {
        if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..")) continue;
        char *path = join_path_alloc(dirpath, entry->d_name);
        lstat(path, &st);
        if (S_ISLNK(st.st_mode)) {
            char *target = read_symlink_target_alloc(path);
            printf("Symlink: %s -> %s\n", entry->d_name, target ? target : "(unknown)");
            free(target);
        } else {
            printf("File: %s (%lld bytes)\n", entry->d_name, (long long)st.st_size);
        }
        if (S_ISREG(st.st_mode)) {
            count++;
            totalSize += st.st_size;
        }
        free(path);
    }
    closedir(dir);
    printf("Total regular files: %u\n", count);
    printf("Total size: %llu bytes\n", totalSize);
}

/* ========================================================= */
/* ======================== MAIN =========================== */
/* ========================================================= */
int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <directory> <mode>\n", argv[0]);
        printf("Modes:\n");
        printf(" 1 = Full listing (Code 1)\n");
        printf(" 2 = Only .txt files (Code 2)\n");
        printf(" 3 = Symlink-aware (Code 3)\n");
        return 1;
    }
    const char *dir = argv[1];
    int mode = atoi(argv[2]);
    if (mode == 1) run_mode_1(dir);
    else if (mode == 2) run_mode_2(dir);
    else if (mode == 3) run_mode_3(dir);
    else printf("Invalid mode.\n"):
    return 0;
}
