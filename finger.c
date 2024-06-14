#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pwd.h>
#include <utmp.h>
#include <time.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <ctype.h>

#define MAX_FIELDS 5 // Numero massimo di campi nel campo GECOS

char *format_phone_number(char *unformatted_number) {
  static char formatted_number[16]; // Buffer to store formatted number
  int len = 0, i;

  // Check if all characters are digits
  while (unformatted_number[len] != '\0') {
    if (!isdigit(unformatted_number[len])) {
      return unformatted_number; // Not a valid phone number (contains non-digits)
    }
    len++;
  }

  // Format based on length
  switch (len) {
    case 11:
      snprintf(formatted_number, sizeof(formatted_number), "+%c-%c%c%c-%c%c%c-%c%c%c%c",
               unformatted_number[0], unformatted_number[1], unformatted_number[2],
               unformatted_number[3], unformatted_number[4], unformatted_number[5],
               unformatted_number[6], unformatted_number[7], unformatted_number[8],
               unformatted_number[9], unformatted_number[10]);
      break;
    case 10:       
        snprintf(formatted_number, sizeof(formatted_number), "%c%c%c-%c%c%c-%c%c%c%c",
               unformatted_number[0], unformatted_number[1], unformatted_number[2],
               unformatted_number[3], unformatted_number[4], unformatted_number[5],
               unformatted_number[6], unformatted_number[7], unformatted_number[8],
               unformatted_number[9]);
      break;
    case 7:
        snprintf(formatted_number, sizeof(formatted_number), "%c%c%c-%c%c%c%c",
               unformatted_number[0], unformatted_number[1], unformatted_number[2],
               unformatted_number[3], unformatted_number[4], unformatted_number[5],
               unformatted_number[6]);
      break;
    case 5:
      snprintf(formatted_number, sizeof(formatted_number), "x%c-%c%c%c%c",
               unformatted_number[0], unformatted_number[1], unformatted_number[2],
               unformatted_number[3], unformatted_number[4]);
      break;
    case 4:
      snprintf(formatted_number, sizeof(formatted_number), "x%c%c%c%c",
               unformatted_number[0], unformatted_number[1], unformatted_number[2],
               unformatted_number[3]);
      break;
    default:
      return unformatted_number; // Not a valid phone number (invalid length)
  }

  return formatted_number;
}

void print_file_content(FILE *file) {
    if (file == NULL) {
        perror("Error opening file");
        return;
    }

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        printf("%s", line);
    }

    fclose(file);
}

FILE* find_and_open_file(const char *filename, const char *directory) {
    DIR *dir;
    struct dirent *entry;
    struct stat filestat;
    char filepath[1024];

    // Apri la directory
    dir = opendir(directory);
    if (dir == NULL) {
        return NULL;
    }

    // Leggi le voci della directory
    while ((entry = readdir(dir)) != NULL) {
        // Costruisci il percorso completo del file
        snprintf(filepath, sizeof(filepath), "%s/%s", directory, entry->d_name);

        // Verifica se è un file regolare e se il nome corrisponde
        if (stat(filepath, &filestat) == 0 && S_ISREG(filestat.st_mode) && strcmp(entry->d_name, filename) == 0) {
            // Chiudi la directory
            closedir(dir);

            // Apri e restituisci il file
            FILE *file = fopen(filepath, "r");
            if (file == NULL) {
                perror("Error opening file");
            }
            return file;
        }
    }

    // Chiudi la directory
    closedir(dir);

    // File non trovato
    return NULL;
}

struct utmp *find_utmp_record(const char *login_name) {
    struct utmp *ut;
    FILE *utmp_file;

    // Apri il file utmp
    utmp_file = fopen(_PATH_UTMP, "rb");
    if (utmp_file == NULL) {
        perror("Errore nell'apertura del file utmp");
        return NULL;
    }

    // Leggi il file utmp alla ricerca del record dell'utente
    while ((ut = malloc(sizeof(struct utmp))) != NULL && fread(ut, sizeof(struct utmp), 1, utmp_file) == 1) {
        if (ut->ut_type == USER_PROCESS && strcmp(ut->ut_user, login_name) == 0) {
            fclose(utmp_file);
            return ut; // Trovato il record
        }
        free(ut);
    }

    // Se non troviamo il record, chiudiamo il file e restituiamo NULL
    fclose(utmp_file);
    return NULL;
}

char *str_to_lower(const char *str) {
    if (str == NULL) {
        return NULL;
    }

    // Allocare memoria per la nuova stringa
    char *lower_str = (char *)malloc(strlen(str) + 1);
    if (lower_str == NULL) {
        perror("malloc");
        return NULL;
    }

    // Convertire i caratteri in minuscolo
    for (int i = 0; str[i]; i++) {
        lower_str[i] = tolower((unsigned char)str[i]);
    }
    lower_str[strlen(str)] = '\0'; // Aggiungere il terminatore null

    return lower_str;
}


// Funzione per fare una copia profonda di una struttura passwd
struct passwd *deep_copy_passwd(struct passwd *src) {
    struct passwd *dst = malloc(sizeof(struct passwd));
    if (dst == NULL) return NULL;

    *dst = *src;

    // Copiare le stringhe puntate
    dst->pw_name = strdup(src->pw_name);
    dst->pw_passwd = strdup(src->pw_passwd);
    dst->pw_gecos = strdup(src->pw_gecos);
    dst->pw_dir = strdup(src->pw_dir);
    dst->pw_shell = strdup(src->pw_shell);

    return dst;
}

char **get_all_login_names_from_name(char *name, int *total_login_names) {
    char **all_logins = NULL;
    struct passwd *pwd;
    int total_login_found = 0;

    setpwent(); // Riavvia la lettura del file passwd

    char *low_name = str_to_lower(name);

    // Cerca tutte le corrispondenze nel campo GECOS
    while ((pwd = getpwent()) != NULL) {
        char *low_gecos = str_to_lower(pwd->pw_gecos);

        if (strstr(low_gecos, low_name) != NULL || strcmp(pwd->pw_name, name) == 0) {
            total_login_found++;
            all_logins = realloc(all_logins, total_login_found * sizeof(char *));
            if (all_logins == NULL) {
                perror("realloc");
                free(low_gecos);
                endpwent();
                return NULL;
            }
            all_logins[total_login_found - 1] = strdup(pwd->pw_name);
            if (all_logins[total_login_found - 1] == NULL) {
                perror("strdup");
                free(low_gecos);
                endpwent();
                return NULL;
            }
        }
        free(low_gecos);
    }
    endpwent();

    if (total_login_found == 0) {
        printf("No user found with name %s\n", name);
    }
    *total_login_names = total_login_found;
    free(low_name);
    return all_logins;
}


struct passwd *get_pwd_record_by_login_name(char* login_name) {
    struct passwd *pw = getpwnam(login_name);
    if(pw == NULL) {
        perror("COuldn't fine pwd entry");
        exit(0);
    }
    return pw;
}

// Funzione per dividere la stringa GECOS e inserirla in un array di stringhe
char** split_gecos(char *gecos) {
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

struct utmp *get_logged_users(int fd, int *total_users) {
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

void print_s_format_single_user(char *login_name, struct passwd *pw_record, struct utmp *utmp_record) {
    char default_str[] = "*";
    char tty_path[256];
    long idle_time = 0;
    char *host = "N/A";
    char *login_time_str = "N/A";
    char **user_gecos = split_gecos(pw_record->pw_gecos);

    if (utmp_record != NULL) {
        snprintf(tty_path, sizeof(tty_path), "/dev/%s", utmp_record->ut_line);
        idle_time = calculate_idle_time(tty_path);
        host = utmp_record->ut_host;
        login_time_str = format_login_time(utmp_record->ut_tv.tv_sec);
    }

    printf("%-15s %-15s %-15s %-15s %-15s %-15s %-15s\n", 
        login_name, 
        user_gecos[0] ? user_gecos[0] : default_str,
        utmp_record ? host : default_str,
        idle_time > 0 ? time_to_string(idle_time) : default_str,
        utmp_record ? login_time_str : default_str,
        user_gecos[1] ? user_gecos[1] : default_str,
        user_gecos[2] ? user_gecos[2] : default_str);

    free_gecos_fields(user_gecos);
}

void print_s_format(int total_logged_users, struct utmp *logged_users, int total_input_users, char** input_users) {
    printf("%-15s %-15s %-15s %-15s %-15s %-15s %-15s\n", "Login", "Name",
     "Tty", "Idle", "Login Time", "Office", "Office Phone");
    
    if(total_input_users == 0 && total_logged_users > 0) {
        for(int i = 0; i < total_logged_users; i++) { 
            print_s_format_single_user(logged_users[i].ut_user, get_pwd_record_by_login_name(logged_users[i].ut_user), &logged_users[i]);
        }
    }
    else{
        for(int i = 0; i < total_input_users; i++) { 
            print_s_format_single_user(input_users[i], get_pwd_record_by_login_name(input_users[i]), find_utmp_record(input_users[i]));
        }
    }

}

char** add_str(char** array, int *size, char* new_str) {
    (*size)++;
    char** new_array = (char**)realloc(array, (*size) * sizeof(char *));

    if(new_array == NULL) {
        perror("Memory reallocation failure");
        exit(0);
    }

    new_array[*size - 1] = (char*) malloc((strlen(new_str) + 1) * sizeof(char));
    if(new_array[*size - 1] == NULL) {
        perror("memory allocation failure");
        exit(1);
    }
    
    strcpy(new_array[*size -1], new_str);

    return new_array;
}

void print_finger_output_single_user(int p, char *login_name, struct passwd *pwd, struct utmp *ut) {
    if (pwd == NULL) {
        fprintf(stderr, "Errore: dati mancanti per l'utente %s\n", login_name);
        return;
    }

    // Informazioni di base
    printf("Login: %s\n", pwd->pw_name);

    char **user_gecos = split_gecos(pwd->pw_gecos);
    
    printf("Name: %s\n", user_gecos[0] ? user_gecos[0] : "" );
    printf("Office: ");
    if(user_gecos[1] != NULL) {
        printf("%s", user_gecos[1]);
    }
    if(user_gecos[2] != NULL) {
        printf(", %s\n", format_phone_number(user_gecos[2]));
    }
    if(user_gecos[3] != NULL) {
        printf("Home Phone: %s\n", format_phone_number(user_gecos[3]));
    }
    
    printf("Directory: %s\n", pwd->pw_dir);
    printf("Shell: %s\n", pwd->pw_shell);

    if(ut == NULL) {
        printf("Never logged in. \n");
    } else {
        // Ultimo login
        time_t last_login_time = ut->ut_tv.tv_sec;
        char last_login_str[256];
        struct tm *tm_info = localtime(&last_login_time);
        strftime(last_login_str, sizeof(last_login_str), "%a %b %d %H:%M", tm_info);
        printf("Last login %s on %s from %s\n", last_login_str, ut->ut_line, ut->ut_host);
    }

    // Mail e Plan (simulato, dato che non abbiamo accesso reale)
    printf("No mail.\n");  // Potresti voler aggiungere una vera verifica della posta qui

    if(p == 1) {
        return;
    } 

    FILE *plan = find_and_open_file(".plan", pwd->pw_dir);
    FILE *project = find_and_open_file(".project", pwd->pw_dir); 
    FILE *pgpkey = find_and_open_file(".pgpkey", pwd->pw_dir);

    if(pgpkey != NULL) {
        printf("PGP key:\n");
        print_file_content(pgpkey);
    }

    if(project != NULL) {
        printf("Project:\n");
        print_file_content(project);
    }

    if(plan != NULL) {
            printf("Plan:\n"); 
            print_file_content(plan); // Potresti voler leggere il file .plan dalla directory home
    }
    else {
        printf("No plan.");
    }


    // Esci
    printf("\n");
}

void print_l_format(int p, int total_logged_users, struct utmp *logged_users, int total_input_users, char **input_users) {
    if(total_input_users == 0 && total_logged_users > 0) {
        for(int i = 0; i < total_logged_users; i++) { 
            print_finger_output_single_user(p, logged_users[i].ut_user, get_pwd_record_by_login_name(logged_users[i].ut_user), &logged_users[i]);
        }
    }
    else{
        for(int i = 0; i < total_input_users; i++) { 
            print_finger_output_single_user(p, input_users[i], get_pwd_record_by_login_name(input_users[i]), find_utmp_record(input_users[i]));
        }
    }
}


int main(int argc, char *argv[]) {

    int fd_utmp; 
    int total_logged_users = 0;

    fd_utmp = open(UTMP_FILE, O_RDONLY);

    if(fd_utmp == -1) {
        perror("Something went wrong:");
        return 0;
    }

    struct utmp *logged_users = get_logged_users(fd_utmp, &total_logged_users);

    if (argc <= 1) {
        
        print_s_format(total_logged_users, logged_users, 0, NULL);

        free(logged_users);

    } else {
        
        int* config = (int*) calloc(4, sizeof(int));
        char** input_names = NULL;
        int input_names_count = 0;

        for(int i = 1; i < argc; i++) {
            if(argv[i][0] == '-') {
                change_config(config, argv[i]);
            }
            else{
                input_names = add_str(input_names, &input_names_count, argv[i]);
            }
        }

        char **input_users_login_names = NULL;
        int total_input_users_login_names = 0;

        if(config[1] == 0) {
            if(input_names_count != 0) {
                for(int i = 0; i < input_names_count; i++) {
                    int tot = 0;
                    char **all_login_names = get_all_login_names_from_name(input_names[i], &tot);
                    
                    for(int i = 0; i < tot; i++) {
                        input_users_login_names = add_str(input_users_login_names, &total_input_users_login_names, all_login_names[i]);
                    }
                    
                }
            }
        } else {
            input_users_login_names = input_names;
            total_input_users_login_names = input_names_count;
        }
        

        if(config[0] == 1 || (config[0] == 0 && config[2] == 0)) {
            print_l_format(config[3], total_logged_users, logged_users, total_input_users_login_names, input_users_login_names);
        }
        else {
            print_s_format(total_logged_users, logged_users, total_input_users_login_names, input_users_login_names);
        }

        free(config);
        free(input_names);

    }

    close(fd_utmp);
    return 0;

}