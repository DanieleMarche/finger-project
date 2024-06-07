#include <utmp.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <bits/waitflags.h>

int main(int argc, char *argv[]) {


    printf("%d", argc);
    struct passwd *passwd_record;
    FILE* fd;

    
    

    // Loop through all entries in the passwd file
    while ((passwd_record = getpwent()) != NULL) {
        printf("%-15s\t%-15s\t%ld\n",
               passwd_record->pw_name,
               passwd_record->pw_shell,
               (long)passwd_record->pw_uid);
    }

    // Close the passwd file
    endpwent();

    return 0;
}



