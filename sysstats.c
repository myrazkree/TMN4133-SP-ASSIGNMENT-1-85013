#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <limits.h>  // for PATH_MAX

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <directory_path>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *dirpath = argv[1];
    DIR *dir = opendir(dirpath);
    if (!dir) {
        perror("Error opening directory");
        return EXIT_FAILURE;
    }

    struct dirent *entry;
    struct stat fileStat;
    char filepath[PATH_MAX];
    unsigned int fileCount = 0;
    unsigned long long totalSize = 0;

    printf("Listing files in directory: %s\n\n", dirpath);
    while ((entry = readdir(dir)) != NULL) {
        // Skip "." and ".."
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
        // Build full path safely
        if (snprintf(filepath, sizeof(filepath), "%s/%s", dirpath, entry->d_name) >= sizeof(filepath)) {
            fprintf(stderr, "Path too long: %s/%s\n", dirpath, entry->d_name);
            continue;
        }
      
        // Get file metadata
        if (stat(filepath, &fileStat) == -1) {
            fprintf(stderr, "Cannot stat file %s: %s\n", filepath, strerror(errno));
            continue;
        }

        // Format last modification time
        char modTime[64];
        struct tm *tm_info = localtime(&fileStat.st_mtime);
        strftime(modTime, sizeof(modTime), "%Y-%m-%d %H:%M:%S", tm_info);
      
        // Display file info
        printf("File: %s\n", entry->d_name);
        printf("Size: %lld bytes\n", (long long)fileStat.st_size);
        printf("Last Modified: %s\n\n", modTime);

        // Count only regular files
        if (S_ISREG(fileStat.st_mode)) {
            fileCount++;
            totalSize += fileStat.st_size;
        }

    }

    closedir(dir);
  
    // Display totals
    printf("Total files: %u\n", fileCount);
    printf("Total cumulative size: %llu bytes\n", totalSize);

    return EXIT_SUCCESS;

}
