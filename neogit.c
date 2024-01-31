#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>

#define MAX_FILENAME_LENGTH 1000
#define MAX_COMMIT_MESSAGE_LENGTH 2000
#define MAX_LINE_LENGTH 1000
#define MAX_MESSAGE_LENGTH 1000

#define debug(x) printf("%s\n", x);


int create_configs(char *username, char *email);

int run_add(int argc, char * const argv[]);
int add_to_staging(char *filepath,int num);

int run_reset(int argc, char * const argv[]);
int remove_from_staging(char *filepath);

int run_commit(int argc, char * const argv[]);
int inc_last_commit_ID();
bool check_file_directory_exists(char *filepath);
int track_file(char *filepath);
bool is_tracked(char *filepath);
int find_file_last_commit(char* filepath);
int reset_staging();

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
        if (find_in_file("/home/radal/.base/list", cwd, strlen(cwd)) == 0){
            FILE * list = fopen("/home/radal/.base/list", "a");
            fprintf(list,"%s\n",cwd);
            fclose(list);
        }
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
    fprintf(file, "last_commit_ID: %d\n", 10000);
    fprintf(file, "current_commit_ID: %d\n", 0);
    fprintf(file, "branch: %s\n", "master");

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

    file = fopen(".neogit/heads", "w");
    fprintf(file,"master 0\n");
    fclose(file);

    file = fopen(".neogit/commits/0", "w");
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
        char * pri = find_source();
        strcat(pri, "/all/");
        strcat(pri, pathto_(filepath)) ;
        fprintf(fl, "%s\n", pri);
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

int reset_staging(){
    for (int i = 0; i <= 10; i++){
        char * stg = find_source();
        char * del = find_source();
        strcat(stg, "/staging_");
        strcat(del, "/deleted_");
        strcat(stg,numtostr(i));
        strcat(del,numtostr(i));
        FILE* file = fopen(stg, "w");
        fclose(file);
        file = fopen(del, "w");
        fclose(file);
        free(del);
        free(stg);
    }
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

    char * src = find_source();
    char * des = find_source();
    strcat(src, "/commits/");
    strcat(src, numtostr(commit_ID));
    strcat(des,"/config");
    FILE * file2 = fopen(des, "r");
    if (file2 == NULL) return 1;

    time_t t = time(NULL);
    struct  tm tm = *localtime(&t);
    char chert[1000],user[1000], email[1000], branch[1000],par[1000];
    fscanf(file2, "%s %s", chert, user);
    fscanf(file2, "%s %s", chert, email);
    for (int i = 0; i < 5; i++) fscanf(file2, "%s", chert);
    fscanf(file2, "%s", branch);
    char* head = find_source();
    strcat(head, "/heads");
    FILE * H = fopen(head, "r");
    while(fscanf(H, "%s %s", chert, par) != EOF){
        if (strcmp(chert, branch) == 0) break;
    }
    fclose(H);
    fclose(file2);

    int t_commit = 0;
    char* src_stage = find_source();
    strcat(src_stage, "/staging_0");
    file2 = fopen(src_stage, "r");
    char line[2024];
    while(fgets(line, 2024, file2) != NULL){
        t_commit++;
    }
    fclose(file2);
    char * src_deleted = find_source();
    strcat(src_deleted, "/deleted_0");
    file2 = fopen(src_deleted, "r");
    while(fgets(line, 2024, file2) != NULL){
        t_commit++;
    }  
    fclose(file2);
    if (t_commit == 0){
        fprintf(stderr, "there's no staged file\n");
        return 1;
    }
    //write commit info
    FILE * file = fopen(src, "w");
    fprintf(file, "TIME : %d/%d/%d    %d:%d:%d\n", tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900, tm.tm_hour, tm.tm_min, tm.tm_sec);
    fprintf(file, "MESSAGE : %s\n", message);
    fprintf(file, "user : %s\n", user);
    fprintf(file, "email : %s\n", email);
    fprintf(file, "commit_id : %d\n", commit_ID);
    fprintf(file, "branch : %s\n", branch);
    fprintf(file, "number of commited files : %d\n", t_commit);    
    fprintf(file, "par : %s\n", par);
    //write commit
    char * last_commit = find_source();
    strcat(last_commit, "/commits/");
    strcat(last_commit, par);
    FILE* lst = fopen(last_commit, "r");
    if (strcmp(par,"0") != 0) for (int i = 0; i < 8; i++) fgets(line, 2024, lst);
    while (fgets(line, 2024, lst) != NULL){
        if (find_in_file(src_deleted, line, strlen(line) - 6)) continue;
        if (find_in_file(src_stage, line, strlen(line) - 6)) continue;
        fprintf(file, "%s", line);
    }
    fclose(lst);
    file2 = fopen(src_stage, "r");
    while (fgets(line, 2024, file2) != NULL){
        fprintf(file, "%s", line);
    }
    fclose(file);
    fclose(file2);
    //change head
    char* tmp = find_source();
    strcat(tmp, "/tmp");
    file2 = fopen(tmp, "w");
    file = fopen(head, "r");
    while (fgets(line, 2024, file) != NULL){
        if (strncmp(line, branch, strlen(branch)) == 0)
            fprintf(file2, "%s %d\n", branch, commit_ID);
        else
            fprintf(file2,"%s", line);
    }
    fclose(file2);
    fclose(file);
    remove(head);
    rename(tmp, head);

    if (reset_staging()) return 1;
    free(tmp);
    free(last_commit);
    free(src_deleted);
    fprintf(stdout, "commit successfully with commit ID %d\n", commit_ID);
    return 0;
}

// returns new commit_ID
int inc_last_commit_ID() {
    char * src = find_source();
    char * des = find_source();
    strcat(src, "/config");
    FILE *file = fopen(src, "r");
    if (file == NULL) return -1;
    
    strcat(des, "/tmp_config");
    FILE *tmp_file = fopen(des, "w");
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

    remove(src);
    rename(des, src);
    free(src);
    free(des);
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

bool is_val_branch(char * branch_name){
    char* src = find_source();
    strcat(src, "/heads");
    FILE* file = fopen(src, "r");
    char line[1000],name[500];
    while (fgets(line, 1000, file) != NULL){
        sscanf(line, "%s", name);
        if (strcmp(branch_name, name) == 0) return 1;
    }
    fclose(file);
    free(src);
    return 0;
}

char * get_branch(char* commit_id){
    char line[1000],tmp[20];
    char *name = (char *)malloc(100);
    char * src = find_source();
    strcat(src, "/commits/");
    strcat(src, commit_id);
    FILE * file = fopen(src, "r");
    for (int i = 0; i < 6; i++) fgets(line, 1000, file);
    sscanf(line, "%s %s %s", tmp, tmp, name);
    fclose(file);
    free(src);
    return name;
}

char * get_auth(char* commit_id){
    char line[1000],tmp[20];
    char *name = (char *)malloc(100);
    char * src = find_source();
    strcat(src, "/commits/");
    strcat(src, commit_id);
    FILE * file = fopen(src, "r");
    for (int i = 0; i < 3; i++) fgets(line, 1000, file);
    sscanf(line, "%s %s %s", tmp, tmp, name);
    fclose(file);
    free(src);
    return name;
}

char * get_messaage(char* commit_id){
    char* line = (char*) malloc(1000);
    char * src = find_source();
    strcat(src, "/commits/");
    strcat(src, commit_id);
    FILE * file = fopen(src, "r");
    for (int i = 0; i < 2; i++) fgets(line, 1000, file);
    fclose(file);
    free(src);
    return line + 10;
}

int run_log(int argc, char * const argv[]){
    bool isbr = (strcmp("-branch", argv[2]) == 0);
    bool isauth = (strcmp("-author", argv[2]) == 0);
    if (isbr && is_val_branch(argv[3]) == 0){
        fprintf(stderr,"Not a valid branch\n");
        return 1;
    }
    if (argc == 3){
        fprintf(stderr,"Invalid command\n");
        return 1;
    }
    char* coms[1000];
    int num = 0;
    char *address = find_source();
    if (address == NULL){
        fprintf(stderr, "neogit storage not found!\n");
        return 1;
    }
    strcat(address, "/commits");
    DIR * dir = opendir(address);
    struct dirent *entry;
    if (argc == 2 || (argc == 4 && (strcmp("-n", argv[2]) == 0 || isbr || isauth))){
        while ((entry = readdir(dir)) != NULL){
            if (entry->d_type != DT_REG || strcmp(entry->d_name,"0") == 0) continue;
            if (isbr && strcmp(argv[3], get_branch(entry->d_name)) != 0) continue;
            if (isauth && strcmp(argv[3], get_auth(entry->d_name)) != 0) continue;
            coms[num] = (char *)malloc(20);
            strcpy(coms[num],entry->d_name);
            num++;
        }
    }
    else if(argc >= 4 && strcmp("-search", argv[2]) == 0){
        while ((entry = readdir(dir)) != NULL){
            if (entry->d_type != DT_REG || strcmp(entry->d_name,"0") == 0) continue;
            bool ok = 0;
            char * mess = get_messaage(entry->d_name);
            for (int i = 3; i < argc; i++){
                if (strstr(mess, argv[i]) != NULL){
                    ok = 1;
                    break;
                }
            }
            if (ok){
                coms[num] = (char *)malloc(20);
                strcpy(coms[num],entry->d_name);
                num++;
            }
        }
    }
    closedir(dir);
    for (int i = 1; i < num; i++){
        int j = i;
        while (j > 0 && strcmp(coms[j], coms[j-1]) > 0){
            char * tmp = coms[j];
            coms[j] = coms[j-1];
            coms[j-1] = tmp;
            j--;
        }
    }
    strcat(address,"/");
    int ln = strlen(address);
    char line[2024];
    if (argc == 4 && strcmp(argv[2], "-n") == 0){
        int n = atoi(argv[3]);
        if (num > n) num = n;
    }
    for (int i = 0; i < num; i++){
        strcat(address, coms[i]);
        FILE * file = fopen(address, "r");
        for (int j = 0; j < 7; j++){
            fgets(line, 2024, file);
            printf("%s", line);
        }
        printf("---------------------------------\n\n");
        fclose(file);
        address[ln] = '\0';
    }
    free(address);
    return 0;        
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
    if (strcmp(command, "log") == 0){
        return run_log(argc, argv);
    }
    if (strcmp(command, "checkout") == 0) {
        return run_checkout(argc, argv);
    }
    
    return 0;
}
