#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <ctype.h>
#include <regex.h>
#include <sys/stat.h>

#define MAX_PATH 1024
#define MAX_FILES 1000

typedef struct {
    char original_name[MAX_PATH];
    char new_name[MAX_PATH];
    int episode_number;
} FileEntry;

// Function to get the file extension
const char *get_file_extension(const char *filename) {
    const char *dot = strrchr(filename, '.');
    if (!dot || dot == filename) return "";
    return dot;
}

// Function to extract episode number from filename
int extract_episode_number(const char *filename) {
    regex_t regex;
    regmatch_t matches[2];
    char episode_str[10] = {0};

    // Try different regex patterns to find episode numbers (both 1 or 2 digits)
        const char *patterns[] = {
        "Episode[ ]*([0-9]{1,3})",         // Episode 1, Episode 12, Episode015
        "Ep[ ]*([0-9]{1,3})",              // Ep 1, Ep12
        "E([0-9]{1,3})([^0-9]|$)",         // E01, E12
        "-[ ]*([0-9]{1,3})([^0-9]|$)",     // - 01, -12
        "S[0-9]+[ ]*-[ ]*([0-9]{1,3})",    // S2 - 10
        "S[0-9]+[ ]+([0-9]{1,3})",         // S2 08 (your case)
        " ([0-9]{1,2})[^0-9]"              // Fallback:  01 or 12 not part of larger number
        };

    for (int i = 0; i < sizeof(patterns) / sizeof(patterns[0]); i++) {
        if (regcomp(&regex, patterns[i], REG_EXTENDED) != 0) {
            continue;
        }

        if (regexec(&regex, filename, 2, matches, 0) == 0) {
            int length = matches[1].rm_eo - matches[1].rm_so;
            if (length < sizeof(episode_str)) {
                strncpy(episode_str, filename + matches[1].rm_so, length);
                episode_str[length] = '\0';
                regfree(&regex);

                // Pad to two digits if only one digit is detected (e.g., E1 -> E01)
                if (strlen(episode_str) == 1) {
                    char padded_episode_str[3] = {'0', episode_str[0], '\0'};
                    return atoi(padded_episode_str);
                }
                return atoi(episode_str);
            }
        }
        regfree(&regex);
    }
    // Fallback: search for isolated 2-digit numbers that aren't part of larger numbers or resolutions
    for (size_t i = 0; i < strlen(filename) - 1; i++) {
        if (isdigit(filename[i]) && isdigit(filename[i + 1])) {
            // Check if it's followed by a non-digit character or end-of-string (so it’s not part of a larger number)
            if ((i == 0 || !isdigit(filename[i - 1])) &&
                (i + 2 >= strlen(filename) || !isdigit(filename[i + 2]))) {
                char num[3] = {filename[i], filename[i + 1], '\0'};
                return atoi(num);
            }
        }
    }
    return 0; // If no episode number found
}

// Compare function for qsort
int compare_files(const void *a, const void *b) {
    FileEntry *fileA = (FileEntry *)a;
    FileEntry *fileB = (FileEntry *)b;
    return fileA->episode_number - fileB->episode_number;
}

int main() {
    char show_name[MAX_PATH];
    char folder_path[MAX_PATH];
    char confirm[10];
    FileEntry files[MAX_FILES];
    int file_count = 0;
    DIR *dir;
    struct dirent *entry;

    // Ask for show name and folder path
    printf("Enter show name: ");
    fgets(show_name, sizeof(show_name), stdin);
    show_name[strcspn(show_name, "\n")] = 0; // Remove newline

    printf("Enter folder path: ");
    fgets(folder_path, sizeof(folder_path), stdin);
    folder_path[strcspn(folder_path, "\n")] = 0; // Remove newline

    // Open directory
    dir = opendir(folder_path);
    if (dir == NULL) {
        perror("Unable to open directory");
        return 1;
    }

    // Read directory contents
    while ((entry = readdir(dir)) != NULL && file_count < MAX_FILES) {
        // Skip . and .. directories
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        // Skip directories
        char full_path[MAX_PATH];
        snprintf(full_path, sizeof(full_path), "%s/%s", folder_path, entry->d_name);

        struct stat path_stat;
        stat(full_path, &path_stat);
        if (!S_ISREG(path_stat.st_mode))
            continue;

        // Get file extension
        const char *extension = get_file_extension(entry->d_name);

        // Only process video files
        if (strcasecmp(extension, ".mp4") != 0 &&
            strcasecmp(extension, ".mkv") != 0 &&
            strcasecmp(extension, ".avi") != 0)
            continue;

        // Extract episode number
        int episode_num = extract_episode_number(entry->d_name);
        if (episode_num == 0) continue; // Skip if no episode number found

        // Store file information
        strcpy(files[file_count].original_name, entry->d_name);
        files[file_count].episode_number = episode_num;

        // Generate new filename
        snprintf(files[file_count].new_name, MAX_PATH, "%s - %02d%s",
                show_name, episode_num, extension);

        file_count++;
    }

    closedir(dir);

    if (file_count == 0) {
        printf("No suitable video files found in the directory.\n");
        return 1;
    }

    // Sort files by episode number
    qsort(files, file_count, sizeof(FileEntry), compare_files);

    // Display the rename plan
    printf("\nRename Plan:\n");
    printf("%-70s -> %s\n", "Original Filename", "New Filename");
    printf("--------------------------------------------------------------------------------\n");

    for (int i = 0; i < file_count; i++) {
        char orig_truncated[71] = {0};
        strncpy(orig_truncated, files[i].original_name, 70);
        if (strlen(files[i].original_name) > 70) {
            strcpy(orig_truncated + 67, "...");
        }
        printf("%-70s -> %s\n", orig_truncated, files[i].new_name);
    }

    // Ask for confirmation
    printf("\nContinue with renaming? (yes/no): ");
    fgets(confirm, sizeof(confirm), stdin);

    if (strncasecmp(confirm, "yes", 3) == 0 || strncasecmp(confirm, "y", 1) == 0) {
        // Perform renaming
        int success_count = 0;

        for (int i = 0; i < file_count; i++) {
            char old_path[MAX_PATH];
            char new_path[MAX_PATH];

            snprintf(old_path, sizeof(old_path), "%s/%s", folder_path, files[i].original_name);
            snprintf(new_path, sizeof(new_path), "%s/%s", folder_path, files[i].new_name);

            if (rename(old_path, new_path) == 0) {
                success_count++;
            } else {
                perror("Error renaming file");
                printf("Failed to rename: %s\n", files[i].original_name);
            }
        }

        printf("\nRenaming complete! %d of %d files successfully renamed.\n", success_count, file_count);
    } else {
        printf("Operation cancelled.\n");
    }

    return 0;
}
