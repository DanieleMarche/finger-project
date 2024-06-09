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

#define MAX_FIELDS 5 // Numero massimo di campi nel campo GECOS

struct passwd *get_pwd_record(char *username) {
    struct passwd *pwd = getpwnam(username);
    if(pwd == NULL) {
        perror("Error getting user information");
        exit(0);
    }
    return pwd;
}

// Funzione per dividere la stringa GECOS e inserirla in un array di stringhe
char** split_gecos(const char *gecos) {
    char *gecos_copy = strdup(gecos);
    if (gecos_copy == NULL) {
        perror("Memory allocation failure");
        exit(EXIT_FAILURE);
    }

    char **fields = malloc(MAX_FIELDS * sizeof(char*));
    if (fields == NULL) {
        perror("Memory allocation failure");
        free(gecos_copy);
        exit(EXIT_FAILURE);
    }

    int i = 0;
    char *token = strtok(gecos_copy, ",");
    while (token != NULL && i < MAX_FIELDS) {
        fields[i] = strdup(token);
        if (fields[i] == NULL) {
            perror("Memory allocation failure");
            // Free previously allocated memory
            for (int j = 0; j < i; j++) {
                free(fields[j]);
            }
            free(fields);
            free(gecos_copy);
            exit(EXIT_FAILURE);
        }
        i++;
        token = strtok(NULL, ",");
    }

    // Se ci sono meno di MAX_FIELDS, riempi i restanti con NULL
    for (int j = i; j < MAX_FIELDS; j++) {
        fields[j] = NULL;
    }

    free(gecos_copy);
    return fields;
}

// Funzione per liberare l'array di stringhe
void free_gecos_fields(char **fields) {
    for (int i = 0; i < MAX_FIELDS; i++) {
        if (fields[i] != NULL) {
            free(fields[i]);
        }
    }
    free(fields);
}

struct utmp* get_logged_users(int fd, int *total_users) {
    struct utmp *logged_users = NULL;
    int total_logged_users = 0;
    struct utmp utmp_buf;

    while(read(fd, &utmp_buf, sizeof(utmp_buf)) == sizeof(utmp_buf)) {
        if(utmp_buf.ut_type == USER_PROCESS) {
            total_logged_users++;
            logged_users = (struct utmp*)realloc(logged_users, total_logged_users * sizeof(struct utmp));
            if(logged_users == NULL) {
                perror("memori allocation failure");
                exit(0);
            }
        memcpy(&logged_users[total_logged_users - 1], &utmp_buf, sizeof(struct utmp));
        }

   }

   *total_users = total_logged_users;
   return logged_users;
}

void change_config(int* config, char* string) {
    int a = strlen(string);
    for(int j = 1; j < a; j++){
        switch(string[j]) {
            case 'l': config[0] = 1; break;
            case 'm': config[1] = 1; break;
            case 's': config[2] = 1; break;
            case 'p': config[3] = 1; break;
            default: printf("Invalid Option"); exit(EXIT_FAILURE);
        }
    }

    if(config[0] == 1) {
        config[2] = 0;
    }
}

char* get_user_gecos(const char *username, char *gecos_info, size_t len) {
    struct passwd *pwd = getpwnam(username);
    if (pwd) {
        strncpy(gecos_info, pwd->pw_gecos, len - 1);
        gecos_info[len - 1] = '\0';  // Ensure null termination
    } else {
        strncpy(gecos_info, "N/A", len);
    }

    return gecos_info;
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

void s_format(struct utmp utmp_record) {
    char tty_path[256];
    snprintf(tty_path, sizeof(tty_path), "/dev/%s", utmp_record.ut_line);

    long idle_time = calculate_idle_time(tty_path);

    char real_name[256];
    struct passwd *pwd = get_pwd_record(utmp_record.ut_user);
    
    char **user_gecos = split_gecos(pwd->pw_gecos);
    
    printf("%-15s %-15s %-15s %-15s %-15s %-15s", 
        utmp_record.ut_user, 
        user_gecos[0],
        utmp_record.ut_host, 
        time_to_string(idle_time),
        format_login_time(utmp_record.ut_tv.tv_sec),
        utmp_record.ut_line);
    printf("\n");

    free_gecos_fields(user_gecos);
}

void l_format(char* config, char** users, int total_users) {
    if(total_users == 0) {

    }
}


int main(int argc, char *argv[]) {

    if (argc <= 1) {

        int fd_utmp, fd_pwd; 
        //struct pwd passwd_record;

        fd_utmp = open(UTMP_FILE, O_RDONLY);

        if(fd_utmp == -1) {
            perror("Something went wrong:");
            return 0;
        }

        printf("%-15s %-15s %-15s %-15s %-15s %-15s %-15s\n", "Login", "Name", "Tty", "Idle", "Login Time", "Office", "Office Phone");
        int total_users = 0;
        struct utmp *users = get_logged_users(fd_utmp, &total_users);

        for(int i = 0; i < total_users; i++) {
            s_format(users[i]);
        }

    } else {
        
        int* config = (int*) calloc(4, sizeof(int));
        char** users = NULL;
        int total_users = 0;

        for(int i = 1; i < argc; i++) {
            if(argv[i][0] == '-') {
                change_config(config, argv[i]);
            }
            else{
                
                total_users++;
                users = (char**)realloc(users, (total_users) * sizeof(char*));
                if(users == NULL) {
                    perror("memory allocation failure");
                    exit(0);
                }

                users[total_users - 1] = (char*) malloc((strlen(argv[i]) + 1) * sizeof(char));
                if(users[total_users - 1] == NULL) {
                    perror("memory allocation failure");
                    exit(1);
                }
                
                strcpy(users[total_users -1], argv[i]);
                
            }
        }

        
        
        free(config);
        free(users);

    }

    return 0;

    
}