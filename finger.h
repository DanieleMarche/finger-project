#include <pwd.h>
#include <utmp.h>
#include <time.h>

//Questa variabile contiene le informazioni riguardo le opzioni richieste dall'untente.
extern int config[4];

//Identificatore di un unico utente
struct user {
    char *login_name;

    struct passwd *pw;

    //Un array dove contentere tutti le entry utmp dell'utente, anche nel caso sia 
    //connesso su più dispostivi
    struct utmp *utmps;
    int utmp_records;
};

//Prototipi di funzioni inerenti allo struct user:


//Ritorna un array contente tutti gli utenti loggati nel sistema, prendendo come 
//parametro il codice fd del file utmp e un intero "total_user" che viene aggiornato
//con il numero degli utenti loggati al sisitema.
struct user *get_logged_users(int fd, int *total_users);

//Aggiunge alla lista degli utenti "users" passsata come paramero, l'utente "u" salvato in  
//uno struct user già esistene e aggiorna l'intero "total_users".
struct user *add_existing_user(int *total_users, struct user *users, struct user u);


//Crea uno struct user con i dati dell'utente passati per parametri e aggiunge l'user creato  
//all'array di utenti "users", aggiornano la variabile contatore "total_users".
struct user *add_user(struct user *users, int *total_users, char *login_name, struct utmp *utmp);


//Aggiunge il record utmp passato come parametro all'array di utmp nell'user record passato come 
//parametro. 
void add_utmp_record_to_user(struct user *user_record, struct utmp *utmp);

//Cerca l'utente come login name "name" all'interno dell'array di utenti "users". Se trova una corrispondenza
//ritorna il puntatore all' struct, altrimenti ritorna NULL.
struct user *find_user(struct user *users, int total_users, char *name);




/////////////////////////////////////////////////////////////////////////////////

//Prototipi di funzioni per il main: 

//Aggiorna la configurazione delle opzioni con la stringa presa come parametro.
void change_config(int* config, char* string);

//////////////////////////////////////////////////////////////////////////////////

//Prototipi di funzioni utils per la manipolazione di dati


//Formatta un numero di telefono inserendo trattini per leggere meglio il numero.
char *format_phone_number(char *unformatted_phone_number);

//Prende come parametri i nomi di una directory e un filename. Cerca il filename
//nella directory. Ritorna il FILE del file trovato, altrimenti ritorna NULL.
FILE* find_and_open_file(const char *filename, const char *directory);

//Ritorna la stringa passata per parametro con tutti i caratteri in lower-case.
char *str_to_lower(const char *str);

//Ritorna un array con tutti gli utenti trovati con il nome passato per parametro.
//Ricerca sia nei nomi di login che nei nomi reali nei GECOS di ogni utente.
//La variabile total_login_names viene incrementata per ogni utente aggiunto all'
//array.
char **get_all_login_names_from_name(char *name, int *total_login_names);

// Funzione per dividere la stringa GECOS e inserirla in un array di stringhe.
char** split_gecos(char *gecos);

// Funzione per liberare la memoria occupata dall'array di stringhe GECOS.
void free_gecos_fields(char **fields);

//Aggiunge la stringa "new_str" all'array di stringe "array" incrementando la 
//varibile "size"
char** add_str(char** array, int *size, char* new_str);

//Prende come parametro il nome di un terminale e ne ritorna l'idle time come long.
long calculate_idle_time(const char *tty_name);

//Questa funzione prende un tempo con tipo "time_t" e ne ritorna la formattazione come stringa.
char* time_to_string(time_t time_value);

//Ritorna la formattazione specifica per l'idle time.
char* format_login_time(time_t login_time);

//Ritorna un deep copy dello struct passwd passato come paramentro.
struct passwd *deep_copy_passwd(struct passwd *passwd);

//Ritorna il puntatore al record passwd dell'utente con il login name passato come parametro.
struct passwd *get_pwd_record_by_login_name(char *login_name);




///////////////////////////////////////////////////////////////////////////////////////

//Prototipi di funzioni di stampa dei dati di un user:

//Stampa i dati in long format dell'utente passato come parametro, 
//La variabile "p" viene utilizzata per decidere se stampare o no i dati 
//riguardo il "plan" dell'utente.
void print_long_format_single_user(int p, struct user *usr);

//Stampa i dati di tutti gli utenti all'interno dell'array "*user"
//La variabile "total_user" viene usata per tenere traccia della lunghezza
//dell'array. La variabile "p" viene usata per decidere se stampare o no 
//i dati riguardo il "plan" dell'utente.
void print_l_format(int p, int total_users, struct user *users);

//Stampa i dati riguardanti il device utilizzato dall'utente passato come 
//parametro.
void print_device_information(struct user *user);

//Stampa il plan dell'utente proprietario della home directory passata
//come parametro.
void print_plan(char* home_dir);

//Stampa i dati dell'utente passato come parametro in short format.
void print_s_format_single_user(struct user u);

//Stampa i dati degli utenti all'interno dell'array user in short format.
//La varibile "users_count" contiene la lunghezza dell'array.
void print_short_format(int users_count, struct user *users);

//Stampa i dati contenti nel file passato come parametro.
void print_file_content(FILE *file);

//Stampa le informazioni rigaurdo le mail dell'utente con login name passato come 
//parametro.
void print_mail_status(const char *username);

