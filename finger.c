#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <ctype.h>
#include "finger.h"

////////////////////////////////////////////////////////////////////////////////////////////

//Funzione per la gestione delle opzioni: 

//Changes the config array witch the options passed as input
void change_config(int* config, char* string) {
    int a = strlen(string);
    for(int j = 1; j < a; j++){
        switch(string[j]) {
            case 'l': config[0] = 1; break;
            case 'm': config[1] = 1; break;
            case 's': config[2] = 1; break;
            case 'p': config[3] = 1; break;
            default: printf("Invalid Option : %c", string[j]); 
            exit(EXIT_FAILURE);
        }
    }

}

////////////////////////////////////////////////////////////////////////////////////////////

//Funzionni di utils per la manipolazione dei dati:

char *format_phone_number(char *unformatted_number) {
  static char formatted_number[16]; // Buffer to store formatted number
  int len = 0, i;

  //Controlla che tutti i caratteri all'interno della stringa siano numeri
  while (unformatted_number[len] != '\0') {
    if (!isdigit(unformatted_number[len])) {
      return unformatted_number; //Non contiene tutti numeri, ritorna la stringa
    }
    len++;
  }

  //Formatta il numero in base alla sua lunghezza
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
      return unformatted_number; //Il numero non ha una lunghezza valida
  }

  return formatted_number;
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

char **get_all_login_names_from_name(char *name, int *total_login_names) {

    if(strcmp(name, "-") == 0) {
        printf("No user with name: %s\n", name);
        *total_login_names = 0; 
        return NULL;
    }

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


struct passwd *get_pwd_record_by_login_name(char *login_name) {
    struct passwd *pw = getpwnam(login_name);
    if(pw == NULL) {
        printf("finger: %s: no such user", login_name);
        return NULL;
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

    char **fields = malloc(5 * sizeof(char*));
    if (fields == NULL) {
        perror("Memory allocation failure");
        free(gecos_copy);
        exit(EXIT_FAILURE);
    }

    int i = 0;
    char *token = strtok(gecos_copy, ",");
    while (token != NULL && i < 5) {
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
    for (int j = i; j < 5; j++) {
        fields[j] = NULL;
    }

    free(gecos_copy);
    return fields;
}


// Funzione per liberare la memoria occupata dall'array di stringhe GECOS.
void free_gecos_fields(char **fields) {
    for (int i = 0; i < 5; i++) {
        if (fields[i] != NULL) {
            free(fields[i]);
        }
    }
    free(fields);
}

struct passwd *deep_copy_passwd(struct passwd *passwd) {
    if (!passwd) return NULL;

    //Alloca memoria per la deep copy
    struct passwd *new_passwd = malloc(sizeof(struct passwd));
    if (!new_passwd) return NULL;

    
    new_passwd->pw_uid = passwd->pw_uid;
    new_passwd->pw_gid = passwd->pw_gid;

    //Effettua un deep copy di tutti i dati necezsari
    new_passwd->pw_name = strdup(passwd->pw_name);
    new_passwd->pw_gecos = strdup(passwd->pw_gecos);
    new_passwd->pw_dir = strdup(passwd->pw_dir);
    new_passwd->pw_shell = strdup(passwd->pw_shell);

    return new_passwd;
}

char* format_login_time(time_t login_time) {

    time_t now;
    time(&now);

    double diff = difftime(now, login_time); 

    if (diff > 365 * 24 * 3600) {
         struct tm* time_info = localtime(&login_time);
        
        // Allcoa la memoria per la sringa di output
        char* time_string = (char*)malloc(5);  // 4 caratteri per l'anno + carattere nullo
        
        // Format the year string
        strftime(time_string, 5, "%Y", time_info);
        
        return time_string;
    }

    // Alloca memoria per la stringa di output
    char *time_string = (char*) malloc(18);  // 17 caratteri per "Mese GG hh:mm" + carattere nullo
    if (!time_string) {
        return NULL;  // Gestisce gli errori per l'allocazione di memoria
    }

    // converte il tempo in una struct tm
    struct tm time_info;
    localtime_r(&login_time, &time_info);

    // formatta la stringa
    strftime(time_string, 18, "%b %d %H:%M", &time_info);

    return time_string;  // Ritorna la stringa formmattata
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

////////////////////////////////////////////////////////////////////////////////////////////

//Funzioni per la gestione degli struct user: 

struct user *find_user(struct user *users, int total_users, char *name) {

    //Itera per ogni utente all'interno dell'array
    for (int i = 0; i < total_users; i++) {
        if (strcmp(users[i].login_name, name) == 0) {
            return &users[i]; // Ritorna l'utente con login name uguale alla stringa passata come parametro
        }
    }
    return NULL;  // se non trova niente ritorna null
}

void add_utmp_record_to_user(struct user *user_record, struct utmp *utmp) {

    //Aggiorna il contatore
    user_record->utmp_records++;

    //Realloca spazio per l'array di struct utmp
    user_record->utmps = (struct utmp *) realloc(user_record->utmps, user_record->utmp_records * sizeof(struct utmp));
    if (user_record->utmps == NULL) {
        perror("Error reallocating memory for utmps"); // Gestisce errori di realloc
        exit(EXIT_FAILURE);
    }

    //Copia l'utmp record nell'array
    memcpy(&user_record->utmps[user_record->utmp_records - 1], &utmp, sizeof(struct utmp));
}

struct user *add_user(struct user *users, int *total_users, char *login_name, struct utmp *utmp) {

    //Controlla se esiste già quell'utente al'interno dell'array
    struct user *temp = find_user(users, *total_users, login_name);
    if(temp != NULL) {
        return users; // se l'utente già è presente ritorna l'user trovato
    }

    // Incrementa il numero di utenti
    (*total_users)++;

    // Rialloca memoria per l'array di utenti
    users = (struct user*) realloc(users, (*total_users) * sizeof(struct user));
    if (users == NULL) {
        perror("Error reallocating memory for users");
        exit(EXIT_FAILURE);
    }

    // Alloca e copia il login name
    users[*total_users - 1].login_name = strdup(login_name);

    if (users[*total_users - 1].login_name == NULL) {
        perror("Error duplicating login name");
        exit(EXIT_FAILURE);
    }

    // Gestisci i record utmp
    if (utmp != NULL) {
        users[*total_users - 1].utmps = (struct utmp *)calloc(1, sizeof(struct utmp));
        if (users[*total_users - 1].utmps == NULL) {
            perror("Error allocating memory for utmps");
            exit(EXIT_FAILURE);
        }
        memcpy(&users[*total_users - 1].utmps[0], utmp, sizeof(struct utmp));
        users[*total_users - 1].utmp_records = 1;
    } else {
        users[*total_users - 1].utmps = NULL;
        users[*total_users - 1].utmp_records = 0;
    }

    users[*total_users - 1].pw = deep_copy_passwd(get_pwd_record_by_login_name(login_name));

    return users;
}


struct user *add_existing_user(int *total_users, struct user *users, struct user u) {
    struct user *temp = find_user(users, *total_users, u.login_name);
    if(temp != NULL) {
        return users;
    }
    // Incrementare il numero totale di utenti
    (*total_users)++;

    // Riallocare memoria per l'array di utenti
    users = (struct user*) realloc(users, (*total_users) * sizeof(struct user ));
    if (users == NULL) {
        perror("realloc");
        exit(EXIT_FAILURE);
    }

    // Aggiungere il nuovo utente all'array
    (users)[*total_users - 1] = u;

    return users;
}

struct user *get_logged_users(int fd, int *total_users) {
    struct user* users = NULL;
    struct utmp utmpbuf;

    //legge tutte le entry del file utmp
    while (read(fd, &utmpbuf, sizeof(utmpbuf)) == sizeof(utmpbuf)) {

        // controlla se la entry è di un utente
        if (utmpbuf.ut_type == USER_PROCESS) {

            // Controlla se l'user è già esiste nella lista
            struct user *user_record = find_user(users, *total_users, utmpbuf.ut_user);
            if (user_record == NULL) {
                users = add_user(users, total_users, utmpbuf.ut_user, &utmpbuf);
            } else {

                //se l'user già esiste aggiunge il nuovo entry utmp allo struct dell'utente
                add_utmp_record_to_user(user_record, &utmpbuf);
            }
        }
    }
    return users;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////

//Funzioni per stampare le informazioni degli utenti:


void print_mail_status(const char *username) {
    char filepath[256];
    snprintf(filepath, sizeof(filepath), "/var/mail/%s", username);

    //Apre il file mail dell'utente
    FILE *mail_file = fopen(filepath, "r");
    if (mail_file == NULL) {
        printf("No mail.\n");  //Se il file non esisite, stampa "no mail"
        return;
    }

    // ottiene lo stat del file 
    struct stat file_stat;
    if (stat(filepath, &file_stat) == -1) {
        perror("stat");  // gestisce eventuali errori
        fclose(mail_file);
        return;
    }

    // se il file è vuoto stampa "no mail"
    if (file_stat.st_size == 0) {
        printf("No Mail.\n");
        fclose(mail_file);
        return;
    }

    time_t last_access = file_stat.st_atime;
    time_t last_modification = file_stat.st_mtime;

    // controlla il numero di mail
    char buffer[1024];
    int new_mail = 0;
    while (fgets(buffer, sizeof(buffer), mail_file)) {
        if (strncmp(buffer, "From ", 5) == 0) {
            new_mail = 1;
            break;
        }
    }


    if (new_mail) {

        //controlla quando sono arrivate
        if (last_access >= last_modification) {
            char last_read_time[64];
            strftime(last_read_time, sizeof(last_read_time), "%a %b %d %H:%M:%S %Y (%Z)", localtime(&last_access));
            printf("Mail last read %s\n", last_read_time);
        } else {
            char last_received_time[64];
            strftime(last_received_time, sizeof(last_received_time), "%a %b %d %H:%M:%S %Y (%Z)", localtime(&last_modification));
            printf("New mail received %s\n", last_received_time);
            printf("Unread since %s\n", last_received_time);
        }
    } else {
        printf("No Mail.\n"); // se non ci sono mail stampa "no mail"
    }

    fclose(mail_file);
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

void print_s_format_single_user(struct user u) {
    struct utmp *utmp = NULL;
    struct passwd *pw = u.pw;

    // se ci sono più utmp record stampa solo il primo
    if(u.utmp_records > 0) {
        utmp = &u.utmps[0];
    }

    // dati di default in caso non sia presente il record
    char default_str[] = "*";
    char tty_path[256];
    long idle_time = 0;
    char *host = "N/A";
    char *login_time_str = "N/A";
    char **user_gecos = split_gecos(pw->pw_gecos);

    //aggiornamento dei dati in caso sia presente 
    if (utmp != NULL) {
        snprintf(tty_path, sizeof(tty_path), "/dev/%s", utmp->ut_line);
        idle_time = calculate_idle_time(tty_path);
        host = utmp->ut_host;
        login_time_str = format_login_time(utmp->ut_tv.tv_sec);
    }

    //stampa i dati
    printf("%-15s %-15s %-15s %-15s %-15s %-15s %-15s\n", 
        u.login_name, 
        user_gecos[0] ? user_gecos[0] : default_str,
        utmp ? host : default_str,
        idle_time > 0 ? time_to_string(idle_time) : default_str,
        utmp ? login_time_str : default_str,
        user_gecos[1] ? user_gecos[1] : default_str,
        user_gecos[2] ? format_phone_number(user_gecos[2]) : default_str);

    free_gecos_fields(user_gecos);
}

void print_short_format(int users_count, struct user *users) {

    printf("%-15s %-15s %-15s %-15s %-15s %-15s %-15s\n", "Login", "Name",
     "Tty", "Idle", "Login Time", "Office", "Office Phone");

    for(int i = 0; i < users_count; i++) { 
        print_s_format_single_user(users[i]);
    }
}

void print_plan(char* home_dir) {

    FILE *plan = find_and_open_file(".plan", home_dir);
    FILE *project = find_and_open_file(".project", home_dir); 
    FILE *pgpkey = find_and_open_file(".pgpkey", home_dir);

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
        printf("No plan.\n");
    }
}

void print_device_information(struct user *user) {
    for (int i = 0; i < user->utmp_records; i++) {
        struct utmp *utmps = user->utmps;
        // Ultimo login
        time_t last_login_time = utmps[i].ut_tv.tv_sec;
        char last_login_str[256];
        struct tm *tm_info = localtime(&last_login_time);
        strftime(last_login_str, sizeof(last_login_str), "%a %b %d %H:%M", tm_info);
        
        // Costruire il percorso del dispositivo
        char tty_path[128];
        snprintf(tty_path, sizeof(tty_path), "/dev/%s", utmps[i].ut_line);

        // Controllare se il dispositivo ha il permesso di scrittura
        bool write_permission_denied = (access(tty_path, W_OK) == -1);

        // Stampare le informazioni del dispositivo
        printf("On since %s on %s from %s", last_login_str, utmps[i].ut_line, utmps[i].ut_host);
        if (write_permission_denied) {
            printf(" (messages off)");
        }
        printf("\n");

        // Calcolare e stampare il tempo di inattività
        long idle_time = calculate_idle_time(tty_path);
        printf("%s idle\n", time_to_string(idle_time));
    }
}

void print_long_format_single_user(int p, struct user *usr) {

    struct passwd *pwd = usr->pw;

    // Informazioni di base
    printf("Login: %s\n", usr->login_name);

    char **user_gecos = split_gecos(pwd->pw_gecos);
    
    printf("Name: %s\n", user_gecos[0] ? user_gecos[0] : "" );

    if(user_gecos[1] != NULL || user_gecos[2] != NULL){
        printf("Office:"); 
    }
    
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

    print_device_information(usr);

    FILE *forward = find_and_open_file(".forward", pwd->pw_dir);

    if(forward != NULL) {
        print_file_content(forward);
    }

    //Stampa i dati di mail status
    print_mail_status(pwd->pw_name);

    //Se non presente l'opzione p stampa i dati sul plan.
    if(p == 0) {
        print_plan(pwd->pw_dir);
    } 

    // Esci
    printf("\n");
}

void print_l_format(int p, int total_users, struct user *users) {

    for(int i = 0; i < total_users; i++) { 
        print_long_format_single_user(p, &users[i]);
    }
    
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////

//Main

int main(int argc, char *argv[]) {

    int fd_utmp; 
    int logged_users_count = 0;

    // apertura file utmp
    fd_utmp = open(UTMP_FILE, O_RDONLY);

    if(fd_utmp == -1) {
        perror("Something went wrong:"); //gestione errori
        return 0;
    }

    // si ottengono gli struct user degli utenti loggati
    struct user *logged_users = get_logged_users(fd_utmp, &logged_users_count);

    close(fd_utmp);

    
    if (argc <= 1) {
        
        //Nessun argomento viene passato, quindi si stampano le informazioni riguardo gli utenti
        //loggati in formato short
        print_short_format(logged_users_count, logged_users);

    } else {
        
        //iniziallizzazione array di config e di nomi di input
        int config[4] = {0, 0, 0, 0};
        char** input_names = NULL;
        int input_names_count = 0;


        //Si ottengono i nomi e le opzioni passati come argomenti
        for(int i = 1; i < argc; i++) {
            if(argv[i][0] == '-' && strlen(argv[i]) > 1) {
                change_config(config, argv[i]);
            }
            else{
                input_names = add_str(input_names, &input_names_count, argv[i]);
            }
        }

        //definizione dell'array struct user degli utenti passati in input
        struct user *input_users = NULL;
        int input_users_count = 0;
       
        if(input_names_count != 0) {
            for(int i = 0; i < input_names_count; i++) {

                //Controlla anche nei nomi reali degli utenti
                if(config[1] == 0) {
                    int tot = 0;
                    char **all_login_names = get_all_login_names_from_name(input_names[i], &tot);
                    
                    for(int i = 0; i < tot; i++) {
                        struct user *u = find_user(logged_users, logged_users_count, all_login_names[i]);
                        if(u != NULL) {
                            input_users = add_existing_user(&input_users_count, input_users, *u);
                        }
                        else {
                            input_users = add_user(input_users, &input_users_count, all_login_names[i], NULL);
                        }
                        
                    }
                    free(all_login_names);

                } 
                
                //Controlla solo i nomi di login degli utenti
                else {
                    struct user *u = find_user(logged_users, logged_users_count, input_names[i]);
                    if(u != NULL) {
                        input_users = add_existing_user(&input_users_count, input_users, *u);
                    }
                    else {
                        input_users = add_user(input_users, &input_users_count, input_names[i], NULL);
                    }
                }

            }

        } 
        
        //Nessun nome viene passato come argomento, quindi gli input user diventano i logged users
        else {
            input_users = logged_users;
            input_users_count = logged_users_count;
        }


        // stampa i dati degli utenti con il formato richiesto dagli argomenti
        if(config[0] == 1 || (config[0] == 0 && config[2] == 0 && input_names_count > 0)) {
            print_l_format(config[3], input_users_count, input_users);
        }
        else {
            print_short_format(input_users_count, input_users);
        }


        free(input_names);
        free(input_users);

    }

    return 0;
}