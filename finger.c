#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pwd.h>
#include <utmp.h>
#include <time.h>
#include <sys/stat.h>
#include <string.h>

void get_user_gecos(const char *username, char *gecos_info, size_t len) {
    struct passwd *pwd = getpwnam(username);
    if (pwd) {
        strncpy(gecos_info, pwd->pw_gecos, len - 1);
        gecos_info[len - 1] = '\0';  // Ensure null termination
    } else {
        strncpy(gecos_info, "N/A", len);
    }
}

char* format_login_time(time_t login_time) {

    time_t now;
    time(&now);

    double diff = difftime(now, login_time); 

    if (diff > 365 * 24 * 3600) {
         struct tm* time_info = localtime(&login_time);
        
        // Allocate memory for the output string
        char* time_string = (char*)malloc(5);  // 4 characters for the year + null terminator
        
        // Format the year string
        strftime(time_string, 5, "%Y", time_info);
        
        return time_string;
    }

    // Allocate memory for the output string
    char *time_string = (char*) malloc(18);  // 17 characters for "Mese GG hh:mm" + null terminator
    if (!time_string) {
        return NULL;  // Handle memory allocation error
    }

    // Convert time to tm structure
    struct tm time_info;
    localtime_r(&login_time, &time_info);

    // Format the time string
    strftime(time_string, 18, "%b %d %H:%M", &time_info);

    return time_string;  // Return the formatted time string
}

char* time_to_string(time_t time_value) {
  // Allocate memory for the output string
  char* time_string = (char*) malloc(6);  // 5 characters for "hh:mm" + null terminator
  if (!time_string) {
    return NULL;  // Handle memory allocation error
  }
  // Convert time to tm structure
  struct tm time_info;
  localtime_r(&time_value, &time_info);

  // Format the time string
  snprintf(time_string, 6, "%02d:%02d", time_info.tm_hour, time_info.tm_min);

  return time_string;  // Return the formatted time string
}

long calculate_idle_time(const char *tty_name) {
    struct stat statbuf;
    time_t current_time;
    
    if (stat(tty_name, &statbuf) == -1) {
        perror("stat");
        return -1;
    }
    
    time(&current_time);
    return (long)difftime(current_time, statbuf.st_atime);
}

char* s_format(struct utmp utmp_record) {
    char tty_path[256];
                snprintf(tty_path, sizeof(tty_path), "/dev/%s", utmp_record.ut_line);
            
                long idle_time = calculate_idle_time(tty_path);
                
                printf("%-15s %-15s %-15s %-15s %-15s", 
                    utmp_record.ut_user, 
                    utmp_record.ut_host, 
                    time_to_string(idle_time),
                    format_login_time(utmp_record.ut_tv.tv_sec),
                    utmp_record.ut_line);
                printf("\n");
}


int main(int argc, char *argv[]) {

    if (argc <= 1) {

        int fd_utmp, fd_pwd; 
        struct utmp utmp_record; 
        //struct pwd passwd_record;

        fd_utmp = open(UTMP_FILE, O_RDONLY);

        if(fd_utmp == -1) {
            perror("Something went wrong:");
            return 0;
        }

        printf("%-15s %-15s %-15s %-15s %-15s %-15s\n", "Nome Utente", "Tty", "Idle", "Login Time", "Office", "Office Phone");

        while(read(fd_utmp, &utmp_record, sizeof(utmp_record)) == sizeof(utmp_record)) {
            if (utmp_record.ut_type == USER_PROCESS) {

                s_format(utmp_record);

            }
        }

    } else {
        
        int* config = (int*) calloc(4, sizeof(int));

        for(int i = 1; i < argc; i++) {
            if(argv[i][0] == '-') {
                int a = strlen(argv[i]);
                for(int j = 1; j < a; j++){
                    switch(argv[i][j]) {
                        case 'l': config[0] = 1; break;
                        case 'm': config[1] = 1; break;
                        case 's': config[2] = 1; break;
                        case 'p': config[3] = 1; break;
                        default: printf("Invalid Option"); exit(EXIT_FAILURE);
                    }
                }
            }

            if(config[0] == 1) {
                config[2] = 0;
            }
            
        }
        

        free(config);

    }

    return 0;

    
}