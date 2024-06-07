#include <stdio.h>
#include <utmp.h>
#include <fcntl.h>
#include <unistd.h>
#include <bits/fcntl-linux.h>

int main() {
    struct utmp current_record;
    int utmp_file;
    
    // Open the utmp file
    utmp_file = open(UTMP_FILE, O_RDONLY);
    if (utmp_file == -1) {
        perror("Error opening utmp file");
        return 1;
    }

    // Read the utmp records
    while (read(utmp_file, &current_record, sizeof(current_record)) == sizeof(current_record)) {
        // Check if the record type is a user process
        if (current_record.ut_type == USER_PROCESS) {
            printf("%-8s  %-8s  %-8s  %s", 
                   current_record.ut_user, 
                   current_record.ut_line, 
                   current_record.ut_id, 
                   current_record.ut_host);
            printf("\n");
        }
    }

    // Close the utmp file
    close(utmp_file);

    return 0;
}