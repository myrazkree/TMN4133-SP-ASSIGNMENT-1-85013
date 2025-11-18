#include <stdio.h>             // stdio.h: input/output functions (printf, fprintf, snprintf, etc.)
#include <stdlib.h>            // stdlib.h: memory allocation, exit, atoi, free, realloc, malloc, etc.
#include <dirent.h>            // dirent.h: directory traversal (DIR, struct dirent, opendir, readdir, closedir)
#include <sys/stat.h>          // sys/stat.h: file status (stat, lstat, struct stat, file mode macros)
#include <time.h>              // time.h: time formatting (strftime, localtime)
#include <string.h>            // string.h: string utilities (strlen, strcmp, strcpy, strdup, strerror)
#include <errno.h>             // errno.h: errno variable and error strings
#include <limits.h>            // limits.h: PATH_MAX and other limits
#include <ctype.h>             // ctype.h: character classification and conversion (tolower)
#include <unistd.h>            // unistd.h: POSIX APIs (readlink, access, etc.)
#include <stdint.h>            // stdint.h: fixed-width integer types (uintmax_t)

/* portability: define directory separator depending on platform */
#ifdef _WIN32
#define DIR_SEP '\\'            // on Windows use backslash
#else
#define DIR_SEP '/'             // on POSIX use forward slash
#endif

/* ---------- Code Prompt 1 ---------- */
static char *join_path(const char *dir, const char *name) { // allocate and return concatenation of dir and name with separator
    size_t len = strlen(dir);                     // length of directory string
    size_t nlen = strlen(name);                   // length of filename
    int need_sep = (len > 0 && dir[len - 1] != DIR_SEP); // determine if we need to insert a separator
    size_t total = len + (need_sep ? 1 : 0) + nlen + 1;  // total buffer size including null terminator
    char *p = malloc(total);                      // allocate buffer
    if (!p) return NULL;                          // return NULL on allocation failure
    if (need_sep)
        snprintf(p, total, "%s%c%s", dir, DIR_SEP, name); // write "dir/ name"
    else
        snprintf(p, total, "%s%s", dir, name);            // write "dirname" if separator not needed
    return p;                                     // return allocated path (caller must free)
}
static const char *file_type_string(mode_t m) {  // return human-readable string for file type from st_mode
    if (S_ISREG(m)) return "regular file";       // regular file
    if (S_ISDIR(m)) return "directory";          // directory
    if (S_ISLNK(m)) return "symlink";            // symbolic link
    if (S_ISCHR(m)) return "char device";        // character device
    if (S_ISBLK(m)) return "block device";       // block device
    if (S_ISFIFO(m)) return "FIFO/pipe";         // FIFO or pipe
    if (S_ISSOCK(m)) return "socket";            // socket
    return "unknown";                             // unknown type
}
static void human_readable(uintmax_t size, char *buf, size_t bufsz) { // convert size to human-readable string like "1.23 MB"
    const char *units[] = {"B","KB","MB","GB","TB","PB"}; // units array
    size_t i = 0;                                // unit index
    double s = (double)size;                     // use double for division
    while (s >= 1024.0 && i < 5) {               // scale down while >= 1024 and unit available
        s /= 1024.0;
        i++;
    }
    snprintf(buf, bufsz, "%.2f %s", s, units[i]); // write formatted value into buffer
}

/* ---------- Code Prompt 2 ---------- */
static int has_txt_extension(const char *name) { // check if filename ends with ".txt" (case-insensitive)
    size_t len = strlen(name);                   // get name length
    if (len < 4) return 0;                       // must be at least 4 chars for ".txt"
    const char *ext = name + len - 4;            // pointer to extension start
    return (ext[0] == '.' &&                     // check dot (not lowercased but exact match)
            tolower(ext[1]) == 't' &&            // case-insensitive 't'
            tolower(ext[2]) == 'x' &&            // case-insensitive 'x'
            tolower(ext[3]) == 't');             // case-insensitive 't'
}

/* ---------- Code Prompt 3 ---------- */
static char *join_path_alloc(const char *dir, const char *name) { // alternate join that always inserts '/' and checks overflow
   size_t ld = strlen(dir), ln = strlen(name);    // lengths of dir and name
    if (ld > SIZE_MAX - ln - 2) return NULL;      // overflow guard for size_t arithmetic
    size_t need = ld + 1 + ln + 1;                // compute needed buffer: dir + '/' + name + '\0'
    char *p = malloc(need);                       // allocate buffer
    if (!p) return NULL;                          // fail on allocation error
    snprintf(p, need, "%s/%s", dir, name);        // write to buffer with a forward slash
    return p;                                     // return allocated path
}
static char *read_symlink_target_alloc(const char *path) { // read symlink target into dynamically sized buffer
    ssize_t r;
    size_t buf = 128;                             // initial buffer size
    char *t = NULL;                               // pointer to hold buffer
    while (1) {
        char *tmp = realloc(t, buf + 1);          // (re)allocate buf+1 bytes
        if (!tmp) { free(t); return NULL; }      // fail if realloc fails
        t = tmp;
        r = readlink(path, t, buf);               // readlink does not null-terminate
        if (r < 0) { free(t); return NULL; }     // error reading symlink
        if ((size_t)r <= buf) { t[r] = '\0'; return t; } // if fit, null-terminate and return
        buf *= 2;                                 // double buffer and retry if not enough
    }
}

/* ========================================================= */
/* ==================== MODE 1 CODE ======================== */
/* ========================================================= */
void run_mode_1(const char *dirpath) {            // mode 1: full listing with sorted names and metadata
    DIR *dir = opendir(dirpath);                  // open directory
    if (!dir) {
        fprintf(stderr, "Error: %s\n", strerror(errno)); // print error on failure to open
        return;                                   // return early
    }
    struct dirent *entry;                          // pointer for directory entries
    size_t cap = 0, count = 0;                     // capacity and count for dynamic name list
    char **names = NULL;                           // dynamic array to store names
    while ((entry = readdir(dir)) != NULL) {      // iterate directory entries
        if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
            continue;                              // skip "." and ".."
        if (count + 1 > cap) {                     // ensure capacity for another element
            size_t newcap = cap ? cap * 2 : 64;    // grow strategy: double or start at 64
            names = realloc(names, newcap * sizeof(char*)); // reallocate names array
            cap = newcap;                          // update capacity (note: no NULL check on realloc)
        }
        names[count++] = strdup(entry->d_name);    // duplicate and store entry name
    }
    closedir(dir);                                 // close directory handle
    qsort(names, count, sizeof(char*), (int (*)(const void*, const void*))strcmp); // sort names lexicographically
    unsigned long long totalSize = 0;              // accumulator for total size of regular files
    unsigned int fileCount = 0;                    // count of regular files
    for (size_t i = 0; i < count; i++) {           // iterate sorted names
        char *path = join_path(dirpath, names[i]); // build full path for file
        struct stat st;                            // stat struct to hold file metadata
        stat(path, &st);                           // get file metadata (no error checking)
        char mod[64];                              // buffer for formatted modification time
        strftime(mod, sizeof(mod), "%Y-%m-%d %H:%M:%S", localtime(&st.st_mtime)); // format modtime (no null check)
        char hr[32];                               // buffer for human-readable size
        human_readable(st.st_size, hr, sizeof(hr)); // convert size to human-readable form
        printf("Name: %s\n", names[i]);            // print name
        printf("Type: %s\n", file_type_string(st.st_mode)); // print type string
        printf("Size: %llu (%s)\n", (unsigned long long)st.st_size, hr); // print raw and human size
        printf("Last Modified: %s\n\n", mod);      // print modification time
        if (S_ISREG(st.st_mode)) {                 // if regular file
            fileCount++;                           // increment regular file count
            totalSize += st.st_size;               // add to total size
        }
        free(path);                                // free allocated path
        free(names[i]);                            // free duplicated name
    }
    free(names);                                   // free names array
    printf("Total regular files: %u\n", fileCount); // print totals
    printf("Total cumulative size: %llu bytes\n", totalSize); // print cumulative size
}

/* ========================================================= */
/* ==================== MODE 2 CODE ======================== */
/* ========================================================= */
void run_mode_2(const char *dirpath) {            // mode 2: list only .txt files and report largest file overall
    DIR *dir = opendir(dirpath);                  // open directory
    if (!dir) return;                              // return if cannot open (no error message)
    struct dirent *entry;                          // dirent pointer
    struct stat st;                                // stat struct
    char path[PATH_MAX];                           // stack buffer for path building
    unsigned long long total_txt = 0, largest_size = 0; // accumulators for txt total and largest size
    unsigned int txt_count = 0;                    // count of .txt files
    char *largest = NULL;                          // store name of largest file (allocated)
    while ((entry = readdir(dir)) != NULL) {      // iterate entries
        snprintf(path, sizeof(path), "%s/%s", dirpath, entry->d_name); // build path (no overflow check)
        stat(path, &st);                           // stat file (no error check)
        if (S_ISREG(st.st_mode)) {                 // only consider regular files for largest and txt checks
            if (!largest || st.st_size > largest_size) { // if first or larger than current largest
                free(largest);                      // free previous largest name
                largest = strdup(entry->d_name);    // store new largest file name
                largest_size = st.st_size;         // update largest size
            }
            if (has_txt_extension(entry->d_name)) { // if filename has .txt extension
                char mod[64];                      // buffer for mod time
                strftime(mod, sizeof(mod), "%Y-%m-%d %H:%M:%S", localtime(&st.st_mtime)); // format time
                printf("TXT File: %s\n", entry->d_name); // print txt filename
                printf("Size: %llu bytes\n", (unsigned long long)st.st_size); // print size
                printf("Last Modified: %s\n\n", mod); // print modification time
                txt_count++;                       // increment txt count
                total_txt += st.st_size;           // add to txt total
            }
        }
    }
    closedir(dir);                                 // close directory
    printf("Total .txt files: %u\n", txt_count);   // print .txt count
    printf("Total size of .txt files: %llu bytes\n", total_txt); // print .txt cumulative size
    if (largest)
        printf("Largest file: %s (%llu bytes)\n", largest, largest_size); // print largest file info if any
    free(largest);                                 // free allocated largest name
}

/* ========================================================= */
/* ==================== MODE 3 CODE ======================== */
/* ========================================================= */
void run_mode_3(const char *dirpath) {            // mode 3: symlink-aware listing
    DIR *dir = opendir(dirpath);                  // open directory
    if (!dir) return;                              // return if cannot open (no error message)
    struct dirent *entry;                          // dirent pointer
    struct stat st;                                // stat structure
    unsigned long long totalSize = 0;              // accumulator for total size of regular files
    unsigned int count = 0;                        // count of regular files
    while ((entry = readdir(dir)) != NULL) {      // iterate directory entries
        if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..")) continue; // skip dot entries
        char *path = join_path_alloc(dirpath, entry->d_name); // allocate joined path
        lstat(path, &st);                          // lstat to avoid following symlinks (no error check)
        if (S_ISLNK(st.st_mode)) {                 // if entry is a symlink
            char *target = read_symlink_target_alloc(path); // read symlink target
            printf("Symlink: %s -> %s\n", entry->d_name, target ? target : "(unknown)"); // print link and target
            free(target);                          // free target buffer
        } else {
            printf("File: %s (%lld bytes)\n", entry->d_name, (long long)st.st_size); // print file name and size
        }
        if (S_ISREG(st.st_mode)) {                 // if regular file
            count++;                               // increment count
            totalSize += st.st_size;               // add to total size
        }
        free(path);                                // free allocated path
    }
    closedir(dir);                                 // close directory
    printf("Total regular files: %u\n", count);    // print total regular files
    printf("Total size: %llu bytes\n", totalSize); // print total size
}

/* ========================================================= */
/* ======================== MAIN =========================== */
/* ========================================================= */
int main(int argc, char *argv[]) {                 // main: dispatch based on mode argument
    if (argc != 3) {                               // expect exactly two arguments: directory and mode
        printf("Usage: %s <directory> <mode>\n", argv[0]); // usage message
        printf("Modes:\n");                        // print available modes
        printf(" 1 = Full listing (Code 1)\n");     // mode 1 description
        printf(" 2 = Only .txt files (Code 2)\n"); // mode 2 description
        printf(" 3 = Symlink-aware (Code 3)\n");   // mode 3 description
        return 1;                                  // exit with error code
    }
    const char *dir = argv[1];                     // directory argument
    int mode = atoi(argv[2]);                      // convert mode argument to integer
    if (mode == 1) run_mode_1(dir);                // run mode 1 if requested
    else if (mode == 2) run_mode_2(dir);           // run mode 2 if requested
    else if (mode == 3) run_mode_3(dir);           // run mode 3 if requested
    else printf("Invalid mode.\n"):                 // syntax error here: stray ':' instead of ';' — prints invalid mode
    return 0;                                      // normal exit
}
