#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#define MAX_FILENAME_LENGTH 1000
#define MAX_COMMIT_MESSAGE_LENGTH 2000
#define MAX_LINE_LENGTH 1000
#define MAX_MESSAGE_LENGTH 1000
#define delete_message "this_file_has_been_deleted_goodbye_:/"

#define debug(x) printf("%s\n", x);


int create_configs(char *username, char *email);

int run_add(int argc, char * const argv[]);
int add_to_staging(char *filepath,int num);

int run_reset(int argc, char * const argv[]);
int remove_from_staging(char *filepath);

int run_commit(int argc, char * const argv[]);
int inc_last_commit_ID();
bool check_file_directory_exists(char *filepath);
int commit_staged_file(int commit_ID, char *filepath);
int track_file(char *filepath);
bool is_tracked(char *filepath);
int create_commit_file(int commit_ID, char *message);
int find_file_last_commit(char* filepath);

int run_checkout(int argc, char *const argv[]);
int find_file_last_change_before_commit(char *filepath, int commit_ID);
int checkout_file(char *filepath, int commit_ID);
int add_to_staging_deleted(char * filepath);

char* numtostr(int num){
    char *ans = (char *)malloc(10);
    if (num == 0){
        ans[0] = '0';
        ans[1] = '\0';
        return ans;
    }
    int i = 0;
    while (num){
        ans[i] = (char)(num%10 + '0');
        num /= 10;
        i++;
    }
    ans[i] = '\0';
    for (int j = 0; j < i - j - 1; j++){
        char k = ans[j];
        ans[j] = ans[i - j - 1];
        ans[i - j - 1] = k;
    }
    return ans;
}

void print_command(int argc, char * const argv[]) {
    for (int i = 0; i < argc; i++) {
        fprintf(stdout, "%s ", argv[i]);
    }
    fprintf(stdout, "\n");
}

char * find_source(){
    char cwd[2024];
    if (getcwd(cwd, sizeof(cwd)) == NULL) return "NULL";
    char* ret = (char *)malloc(2024);
    strcpy(ret, cwd);
    char* p = strstr(ret, ".neogit");
    p[7] = '\0';
    return ret;
}

char * pathto_(char * filepath){
    int ln = strlen(filepath);
    char * ans = (char *)malloc(ln + 1);
    for (int i = 0; i < ln; i++){
        if (filepath[i] == '/') ans[i] = '_';
        else ans[i] = filepath[i];
    }
    ans[ln] = '\0';
    return ans;
}

void copy_file(char *src,char * des){
    FILE * s = fopen(src, "r");
    FILE * d = fopen(des, "w");
    char * line = (char *)malloc(1000);
    while (fgets(line, 1000, s) != NULL){
        fprintf(d,"%s",line);
    }
    fclose(d);
    fclose(s);
    free(line);
}

void copy_folder(char *src, char* des){
    struct dirent *entry;
    DIR * dir = opendir(des);
    char tmp[2024];
    strcpy(tmp,des);
    strcat(tmp,"/");
    int ln = strlen(tmp);
    while ((entry = readdir(dir)) != NULL){
        if (strcmp(entry->d_name,".") == 0 || strcmp(entry->d_name,"..") == 0) continue;
        strcat(tmp,entry->d_name);
        remove(tmp);
        tmp[ln] = '\0';
    } 
    closedir(dir);
    dir = opendir(src);
    while ((entry = readdir(dir)) != NULL){
        if (strcmp(entry->d_name,".") == 0 || strcmp(entry->d_name,"..") == 0) continue;
        char addr[2024],addr2[2024];
        strcpy(addr, src);
        strcpy(addr2, des);
        strcat(addr, "/");
        strcat(addr2, "/");
        strcat(addr, entry->d_name);
        strcat(addr2, entry->d_name);
        copy_file(addr, addr2);
    }
    closedir(dir);
}

int add_to_line(char * src,char* word, char* key){
    FILE * com = fopen(src, "r");
    if (com == NULL) return 0;
    bool exist = 0;
    char* line = (char *)malloc(1000);
    char part[100];
    int line_num = 0;
    while (fgets(line, 1000, com) != NULL){
        line_num++;
        while (sscanf(line, "%s", part) != EOF){
            line += strlen(part);
            if (line[0] != '\0') line++;
            if (strcmp(part, key) == 0){
                exist = 1;
                break;
            }
        }
        if (exist){
            break;
        }
    }
    if (exist == 0){
        return 0;
    }
    fclose(com);
    char* des = "/home/radal/.base/commands_copy";
    copy_file(src, des);
    int i = 0;
    com = fopen(src, "w");
    FILE* com2 = fopen(des, "r");
    while (fgets(line, 1000, com2) != NULL){
        i++;
        line[strlen(line) - 1] = '\0';
        if (i == line_num){
            fprintf(com, "%s %s\n", line, word);
        }
        else{
            fprintf(com, "%s\n", line);
        }
    }
    fclose(com);
    fclose(com2);
    remove(des);
    return 1;
}

bool find_in_file(char * filepath, char* path,int num){
    char line[1000];
    FILE * file = fopen(filepath, "r");
    while (fgets(line, 1000, file) != NULL){
        line[strlen(line) - 1] = '\0';
        if (strncmp(line, path, num) == 0) return 1;
    }
    return 0;
}

char* abs_path(char* path){
    char cwd[1000];
    if (getcwd(cwd,1000) == NULL) return NULL;
    strcat(cwd, "/");
    strcat(cwd, path);
    char * ret = (char *)calloc(1000,1);
    strcpy(ret, cwd);
    return ret;
}

int is_dir(char* path){
    struct stat st_buf;
    int status = stat(path, &st_buf);
    if (status != 0) return -1;
    if (S_ISREG(st_buf.st_mode)) return 0;
    if (S_ISDIR(st_buf.st_mode)) return 1;
}

void write_config(char* dir,char* nw,int fl){ //fl = 0 name fl = 1 email
    char * newdir = malloc(strlen(dir)+50);
    strcpy(newdir, dir);
    strcat(newdir,"/.neogit/config\0");
    FILE* conf = fopen(newdir, "r");
    if (conf == NULL) return;
    char username[100],email[200],last_commit_id[10],current_commit_id[10],branch[100];
    char chert[40];
    fscanf(conf,"%s %s", chert, username);
    fscanf(conf,"%s %s", chert, email);
    fscanf(conf,"%s %s", chert, last_commit_id);
    fscanf(conf,"%s %s", chert, current_commit_id);
    fscanf(conf,"%s %s", chert, branch);
    fclose(conf);
    if (fl) strcpy(email,nw);
    else strcpy(username,nw);
    conf = fopen(newdir, "w");
    fprintf(conf, "username: %s\n", username);
    fprintf(conf, "email: %s\n", email);
    fprintf(conf, "last_commit_id: %s\n", last_commit_id);
    fprintf(conf, "current_commit_id: %s\n", current_commit_id);
    fprintf(conf, "branch: %s\n", branch);
    fclose(conf);
    free(newdir);
    return;
}

int config(int argc, char * const argv[]){    
    bool isglobal = (strcmp(argv[2], "-global") != 0);
    if (argc < 6 - isglobal){
        printf("Too few arguments\n");
        return 1;
    }
    if (strcmp(argv[2], "-global") == 0){
        if (strcmp(argv[3], "user") == 0){
            int fl = -1;
            if (strcmp(argv[4],"name") == 0) fl = 0;
            else if (strcmp(argv[4],"email") == 0) fl = 1;
            else{
                printf("Invalid command\n");
                return 1;
            }
            FILE* list = fopen("/home/radal/.base/list","r");
            char dir[1000];
            while (fscanf(list,"%s",dir) != EOF){
                write_config(dir, argv[5], fl);
            }
            printf("Global success!\n");
            return 0;
        }
        if (strcmp(argv[3], "alias") == 0){
            char* src = "/home/radal/.base/commands";
            bool fl = add_to_line(src, argv[4], argv[5]);
            if (fl == 0){
                printf("there's no command as %s!\n", argv[5]);
                return 1;
            }
            FILE* list = fopen("/home/radal/.base/list", "r");
            char* pth = (char *)malloc(1000);
            while (fscanf(list, "%s", pth) != EOF){
                strcat(pth,"/.neogit/commands");
                add_to_line(pth, argv[4], argv[5]);
            }
            fclose(list);
            free(pth);
            printf("alias %s was added successfully and globaly!\n", argv[4]);
            return 0;
        }
        printf("What the Fuck do you mean!\n");
        return 1;
    }
    if (strcmp(argv[2], "user") == 0){
        int fl = -1;
        if (strcmp(argv[3],"name") == 0) fl = 0;
        else if (strcmp(argv[3],"email") == 0) fl = 1;
        else{
            printf("Invalid command\n");
            return 1;
        }
        char* des = find_source();
        des[strlen(des) - 7] = '\0';
        write_config(des, argv[4], fl);
        printf("Success!\n");
        return 0;
    }
    if (strcmp(argv[2], "alias") == 0){
        char* src = find_source();
        src = realloc(src, strlen(src) + 20);
        strcat(src, "/commands");
        bool fl = add_to_line(src, argv[3], argv[4]);
        if (fl == 0){
            printf("there's no command as %s\n", argv[4]);
            return 1;
        }
        printf("alias %s was added successfully!\n", argv[3]);
        return 0;
    }
    printf("What the fuck do you mean!\n");
    return 1;
}

int run_init(int argc, char * const argv[]) {
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) == NULL) return 1;
    char tmp_cwd[1024];
    bool exists = false;
    struct dirent *entry;
    do {
        // find .neogit
        DIR *dir = opendir(".");
        if (dir == NULL) {
            perror("Error opening current directory");
            return 1;
        }
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".neogit") == 0)
                exists = true;
        }
        closedir(dir);

        // update current working directory
        if (getcwd(tmp_cwd, sizeof(tmp_cwd)) == NULL) return 1;

        // change cwd to parent
        if (strcmp(tmp_cwd, "/") != 0) {
            if (chdir("..") != 0) return 1;
        }

    } while (strcmp(tmp_cwd, "/") != 0);

    // return to the initial cwd
    if (chdir(cwd) != 0) return 1;
        
    if (!exists) {
        if (mkdir(".neogit", 0755) != 0) return 1;
        printf("successfully initialized the project!\n");
        FILE* list = fopen("/home/radal/.base/list", "a");
        fprintf(list,"%s\n",cwd);
        fclose(list);
        return create_configs("radin", "radinjarireh@gmail.com");
    }
    printf("neogit repository has already initialized\n");
    return 0;
}

int create_configs(char *username, char *email) {
    FILE *file = fopen(".neogit/config", "w");
    if (file == NULL) return 1;

    fprintf(file, "username: %s\n", username);
    fprintf(file, "email: %s\n", email);
    fprintf(file, "last_commit_ID: %d\n", 0);
    fprintf(file, "current_commit_ID: %d\n", 0);
    fprintf(file, "branch: %s", "master");

    fclose(file);

    //all files folder
    if (mkdir(".neogit/all", 0755) != 0) return 1;
    
    // create commits folder
    if (mkdir(".neogit/commits", 0755) != 0) return 1;

    // create files folder
    if (mkdir(".neogit/files", 0755) != 0) return 1;

    for (int i = 0; i <= 10; i++){
        char src[50],del[50];
        strcpy(src, ".neogit/staging_");
        strcpy(del, ".neogit/deleted_");
        strcat(src,numtostr(i));
        strcat(del,numtostr(i));
        file = fopen(src, "w");
        fclose(file);
        file = fopen(del, "w");
        fclose(file);
    }

    copy_file("/home/radal/.base/commands",".neogit/commands");

    file = fopen(".neogit/tracks", "w");
    fclose(file);

    file = fopen(".neogit/addnum", "w");
    fprintf(file, "%d\n", 0);
    fclose(file);
    return 0;
}

int run_add(int argc, char *const argv[]) {
    bool isf = (strcmp(argv[2], "-f") == 0);
    if (argc < 3 + isf) {
        printf("please specify a file\n");
        return 1;
    }

    if (argv[2][0] != '-' || isf){
        char* des = find_source();
        char* des2 = find_source();
        strcat(des, "/staging_");
        strcat(des2, "/deleted_");
        char src[2024],src2[2024];
        strcpy(src, des);
        strcpy(src2, des2);
        int ln = strlen(des);
        strcat(des, "10");
        strcat(src, "9");
        strcat(des2, "10");
        strcat(src2, "9");
        int i = 9;
        do{
            copy_file(src, des);
            copy_file(src2, des2);
            strcpy(des, src);
            strcpy(des2, src2);
            des[ln + 1] = '\0';
            des2[ln + 1] = '\0';
            i--;
            src[ln] = (char)('0' + i);
            src2[ln] = src[ln];
        }while (i >= 0);
        bool fl = 0;
        des = find_source();
        strcat(des, "/addnum");
        FILE * file = fopen(des, "r");
        int num;
        fscanf(file, "%d", &num);
        fclose(file);
        file = fopen(des, "w");
        num++;
        fprintf(file,"%d\n",num);
        fclose(file);
        for (int i = 2 + isf; i < argc; i++){
            fl |= add_to_staging(abs_path(argv[i]),num);
        }
        return fl;
    }
}

int add_to_staging_deleted(char * filepath){
        if (is_tracked(filepath) == 0){
            return 1;
        }
        char* src = find_source();
        strcat(src,"/deleted_0");
        if (find_in_file(src, filepath, 1000)) return 0;
        FILE* fl = fopen(src, "a");
        fprintf(fl, "%s\n", filepath);
        fclose(fl);   
        src = find_source();
        strcat(src,"/all/");
        strcat(src,pathto_(filepath));
        char* stg = find_source();
        strcat(stg, "/staging_0");
        if (find_in_file(stg, src, strlen(src))){
            char line[1000];
            char * add = find_source();
            strcat(add, "/tmp");
            fl = fopen(stg, "r");
            FILE *tmp = fopen(add, "w");
            while(fgets(line, 1000, fl) != NULL){
                if (strncmp(line, src, strlen(src)) == 0) continue;
                fprintf(tmp, "%s", line);
            }
            fclose(tmp);
            fclose(fl);
            remove(stg);
            rename(add, stg);
        }
        return 0;
}

int add_to_staging(char *filepath,int num) {
    int isdir = is_dir(filepath);
    if (isdir == -1){
        if (add_to_staging_deleted(filepath)){
            printf("there's no such file or folder!\n");
            return 1;
        }
        return 0;
    }
    if (isdir == 1){
        struct dirent *entry;
        DIR *dir = opendir(filepath);
        if (dir == NULL){
            printf("Error opening directory\n!");
            return 1;
        }
        int len = strlen(filepath);
        filepath[len] = '/';
        filepath[len + 1] = '\0';
        bool fl = 0;
        while ((entry = readdir(dir)) != NULL){
            if (strcmp(entry->d_name,".") == 0) continue;
            if (strcmp(entry->d_name,"..") == 0) continue;
            strcat(filepath,entry->d_name);
            fl |= add_to_staging(filepath, num);
            filepath[len + 1] = '\0';
        }
        closedir(dir);
        return fl;
    }
    char* des = find_source();
    char* src = find_source();
    strcat(des, "/all/");
    strcat(des, pathto_(filepath));
    int ln = strlen(des);
    strcat(des, numtostr(num));
    copy_file(filepath, des);
    track_file(filepath);
    strcat(src, "/staging_0");
    if (find_in_file(src, des, ln)){
        char line[1000];
        char * tmp = find_source();
        strcat(tmp,"/tmp");
        FILE * file = fopen(src, "r");
        FILE * file2 = fopen(tmp, "w");
        while (fgets(line, 1000, file) != NULL){
            line[strlen(line) - 1] = '\0';
            if (strncmp(line,des,ln) == 0) continue;
            fprintf(file2, "%s\n", line);
        }
        fclose(file);
        fclose(file2);
        remove(src);
        rename(tmp, src);
        free(tmp);
    }
    FILE * fil = fopen(src, "a");
    fprintf(fil, "%s\n", des);
    fclose(fil);
    free(des);
    free(src);
    return 0;
}

int run_reset(int argc, char *const argv[]) {
    if (argc == 3 && strcmp(argv[2], "-undo") == 0){
        char* src = find_source();
        char* des = find_source();
        char* src2 = find_source();
        char* des2 = find_source();
        strcat(src,"/staging_1");
        strcat(des,"/staging_0");
        strcat(src2,"/deleted_1");
        strcat(des2,"/deleted_0");
        int i = 1;
        int ln = strlen(des);
        while(i <= 10){
            copy_file(src, des);
            copy_file(src2, des2);
            strcpy(des, src);
            strcpy(des2, src2);
            i++;
            if (i < 10){
                src[ln - 1] = (char)('0' + i);
                src2[ln - 1] = src[ln - 1];
            }
            else{
                src[ln - 1] = '1';
                src[ln] = '0';
                src[ln + 1] = '\0';
                src2[ln - 1] = '1';
                src2[ln] = '0';
                src2[ln + 1] = '\0';
            }
        }
        return 0;
    }
    bool isf = (strcmp(argv[2], "-f") == 0);
    if (argc < 3 + isf) {
        printf("please specify a file\n");
        return 1;
    }

    if (argv[2][0] != '-' || isf){
        bool fl = 0;
        for (int i = 2 + isf; i < argc; i++){
            fl |= remove_from_staging(abs_path(argv[i]));
        }
        return fl;       
    }

    return remove_from_staging(argv[2]);
}

int remove_from_staging(char *filepath) {
    char * src = find_source(filepath);
    if (src == NULL){
        printf("neogit storage not found!\n");
        return 1;
    }
    char fake[2024];
    strcpy(fake, src);
    strcat(src, "/staging_0");
    strcat(fake, "/tmp");
    FILE * fl = fopen(src, "r");
    FILE * tmp = fopen(fake, "w");
    char line[2024];
    char * bad = find_source();
    strcat(bad, "/all/");
    strcat(bad,pathto_(filepath));
    int ln = strlen(bad);
    while (fgets(line, 1000, fl) != NULL){
        if (strncmp(bad, line, ln) == 0) continue;
        fprintf(tmp, "%s", line);
    }
    fclose(fl);
    fclose(tmp);
    remove(src);
    rename(fake, src);

    src = find_source(filepath);
    strcpy(fake, src);
    strcat(src, "/deleted_0");
    strcat(fake, "/tmp");
    fl = fopen(src, "r");
    tmp = fopen(fake, "w");
    ln = strlen(filepath);
    while (fgets(line, 1000, fl) != NULL){
        if (strncmp(filepath, line, ln) == 0) continue;
        fprintf(tmp, "%s", line);
    }
    fclose(fl);
    fclose(tmp);
    remove(src);
    rename(fake, src);
    return 0;
}

int run_commit(int argc, char * const argv[]) {
    if (argc < 4) {
        fprintf(stderr,"please use the correct format!\n");
        return 1;
    }

    int ind = 3;
    char message[MAX_MESSAGE_LENGTH];
    message[0] = '\0';
    int cur_size = 0;
    while (ind < argc){
        int ln = strlen(argv[ind]);
        if (ln + cur_size > 72){
            fprintf(stderr,"message is too long\n");
            return 1;
        }
        cur_size += ln;
        strcat(message, argv[ind]);
        if (argc != ind + 1){
             strcat(message, " ");
             cur_size++;
        }
        ind++;
    }

    int commit_ID = inc_last_commit_ID();
    if (commit_ID == -1) return 1;
    
    FILE *file = fopen(".neogit/staging_0", "r");
    if (file == NULL) return 1;
    char line[MAX_LINE_LENGTH];
    while (fgets(line, sizeof(line), file) != NULL) {
        int length = strlen(line);

        // remove '\n'
        if (length > 0 && line[length - 1] == '\n') {
            line[length - 1] = '\0';
        }

        FILE* file2 = fopen(line, "r");
        if (file2 == NULL){
            fprintf(stderr, "Don't touch my stuff!\n");
            return 1;
        }
        fclose(file2);

        if (!check_file_directory_exists(line)) {
            char dir_path[MAX_FILENAME_LENGTH];
            strcpy(dir_path, ".neogit/files/");
            strcat(dir_path, line);
            if (mkdir(dir_path, 0755) != 0) return 1;
        }
        printf("commit %s\n", line);
        commit_staged_file(commit_ID, line);
        track_file(line);
    }
    fclose(file); 
    
    // free staging
    file = fopen(".neogit/staging", "w");
    if (file == NULL) return 1;
    fclose(file);

    create_commit_file(commit_ID, message);
    fprintf(stdout, "commit successfully with commit ID %d", commit_ID);
    
    return 0;
}

// returns new commit_ID
int inc_last_commit_ID() {
    FILE *file = fopen(".neogit/config", "r");
    if (file == NULL) return -1;
    
    FILE *tmp_file = fopen(".neogit/tmp_config", "w");
    if (tmp_file == NULL) return -1;

    int last_commit_ID;
    char line[MAX_LINE_LENGTH];
    while (fgets(line, sizeof(line), file) != NULL) {
        if (strncmp(line, "last_commit_ID", 14) == 0) {
            sscanf(line, "last_commit_ID: %d\n", &last_commit_ID);
            last_commit_ID++;
            fprintf(tmp_file, "last_commit_ID: %d\n", last_commit_ID);

        } else fprintf(tmp_file, "%s", line);
    }
    fclose(file);
    fclose(tmp_file);

    remove(".neogit/config");
    rename(".neogit/tmp_config", ".neogit/config");
    return last_commit_ID;
}

bool check_file_directory_exists(char *filepath) {
    DIR *dir = opendir(".neogit/files");
    struct dirent *entry;
    if (dir == NULL) {
        perror("Error opening current directory");
        return 1;
    }
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR && strcmp(entry->d_name, filepath) == 0) return true;
    }
    closedir(dir);

    return false;
}

int commit_staged_file(int commit_ID, char* filepath) {
    FILE *read_file, *write_file;
    char read_path[MAX_FILENAME_LENGTH];
    strcpy(read_path, filepath);
    char write_path[MAX_FILENAME_LENGTH];
    strcpy(write_path, ".neogit/files/");
    strcat(write_path, filepath);
    strcat(write_path, "/");
    char tmp[10];
    sprintf(tmp, "%d", commit_ID);
    strcat(write_path, tmp);

    read_file = fopen(read_path, "r");
    if (read_file == NULL) return 1;

    write_file = fopen(write_path, "w");
    if (write_file == NULL) return 1;

    char buffer;
    buffer = fgetc(read_file);
    while(buffer != EOF) {
        fputc(buffer, write_file);
        buffer = fgetc(read_file);
    }
    fclose(read_file);
    fclose(write_file);

    return 0;
}

int track_file(char *filepath) {
    if (is_tracked(filepath)) return 0;
    char* src = find_source();
    strcat(src, "/tracks");
    FILE *file = fopen(src, "a");
    if (file == NULL) return 1;
    fprintf(file, "%s\n", filepath);
    return 0;
}

bool is_tracked(char *filepath) {
    char * src = find_source();
    strcat(src,"/tracks");
    return find_in_file(src,filepath,1000);
}

int create_commit_file(int commit_ID, char *message) {
    char commit_filepath[MAX_FILENAME_LENGTH];
    strcpy(commit_filepath, ".neogit/commits/");
    char tmp[10];
    sprintf(tmp, "%d", commit_ID);
    strcat(commit_filepath, tmp);

    FILE *file = fopen(commit_filepath, "w");
    if (file == NULL) return 1;

    fprintf(file, "message: %s\n", message);
    fprintf(file, "files:\n");
    
    DIR *dir = opendir(".");
    struct dirent *entry;
    if (dir == NULL) {
        perror("Error opening current directory");
        return 1;
    }
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG && is_tracked(entry->d_name)) {
            int file_last_commit_ID = find_file_last_commit(entry->d_name);
            fprintf(file, "%s %d\n", entry->d_name, file_last_commit_ID);
        }
    }
    closedir(dir); 
    fclose(file);
    return 0;
}

int find_file_last_commit(char* filepath) {
    char filepath_dir[MAX_FILENAME_LENGTH];
    strcpy(filepath_dir, ".neogit/files/");
    strcat(filepath_dir, filepath);

    int max = -1;
    
    DIR *dir = opendir(filepath_dir);
    struct dirent *entry;
    if (dir == NULL) return 1;

    while((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) {
            int tmp = atoi(entry->d_name);
            max = max > tmp ? max: tmp;
        }
    }
    closedir(dir);

    return max;
}

int run_checkout(int argc, char * const argv[]) {
    if (argc < 3) return 1;
    
    int commit_ID = atoi(argv[2]);

    DIR *dir = opendir(".");
    struct dirent *entry;
    while((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG && is_tracked(entry->d_name)) {
            checkout_file(entry->d_name, find_file_last_change_before_commit(entry->d_name, commit_ID));
        }
    }
    closedir(dir);

    return 0;
}

int find_file_last_change_before_commit(char *filepath, int commit_ID) {
    char filepath_dir[MAX_FILENAME_LENGTH];
    strcpy(filepath_dir, ".neogit/files/");
    strcat(filepath_dir, filepath);

    int max = -1;
    
    DIR *dir = opendir(filepath_dir);
    struct dirent *entry;
    if (dir == NULL) return 1;

    while((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) {
            int tmp = atoi(entry->d_name);
            if (tmp > max && tmp <= commit_ID) {
                max = tmp;
            }
        }
    }
    closedir(dir);

    return max;
}

int checkout_file(char *filepath, int commit_ID) {
    char src_file[MAX_FILENAME_LENGTH];
    strcpy(src_file, ".neogit/files/");
    strcat(src_file, filepath);
    char tmp[10];
    sprintf(tmp, "/%d", commit_ID);
    strcat(src_file, tmp);

    FILE *read_file = fopen(src_file, "r");
    if (read_file == NULL) return 1;
    FILE *write_file = fopen(filepath, "w");
    if (write_file == NULL) return 1;
    
    char line[MAX_LINE_LENGTH];

    while (fgets(line, sizeof(line), read_file) != NULL) {
        fprintf(write_file, "%s", line);
    }
    
    fclose(read_file);
    fclose(write_file);

    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stdout, "please enter a valid command\n");
        return 1;
    }

    char line[1000];
    char * command = calloc(20, sizeof(char));
    FILE * fl = fopen("/home/radal/.base/commands","r");
    while(fgets(line, 1000, fl) != NULL){
        if (strstr(line, argv[1]) != NULL){
            sscanf(line, "%s", command);
            break;
        }
    }
    fclose(fl);
    if (command[0] == '\0'){
        char* dir = find_source();
        strcat(dir,"/commands");
        fl = fopen(dir, "r");
        while(fgets(line, 1000, fl) != NULL){
            if (strstr(line, argv[1]) != NULL){
                sscanf(line, "%s", command);
                break;
            }
        }
        fclose(fl);
    }
    if (strcmp(command, "config") == 0){
        return config(argc, argv);
    }
    if (strcmp(command, "init") == 0) {
        return run_init(argc, argv);
    }
    if (strcmp(command, "add") == 0) {
        return run_add(argc, argv);
    }
    if (strcmp(command, "reset") == 0) {
        return run_reset(argc, argv);
    }
    if (strcmp(command, "commit") == 0) {
        return run_commit(argc, argv);
    }
    if (strcmp(command, "checkout") == 0) {
        return run_checkout(argc, argv);
    }
    
    return 0;
}
