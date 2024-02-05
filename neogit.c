#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>

#define MAX_LINE_LENGTH 2024
#define MAX_MESSAGE_LENGTH 1000
#define MAX_LINE_NUMBER 2000

#define debug(x) printf("%s\n", x);


int create_configs(char *username, char *email);

int add_to_staging(char *filepath,int num);

int remove_from_staging(char *filepath);

int inc_last_commit_ID();
int track_file(char *filepath);
bool is_tracked(char *filepath);
bool is_staged(char * path);

void add_tcommit(int t_commit, char * path){
	char * path2 = (char *)malloc(MAX_LINE_LENGTH);
	strcpy(path2, path);
	strcat(path2, "tmp");
	FILE * file = fopen(path, "r");
	FILE * file2 = fopen(path2, "w");
	char line[MAX_LINE_LENGTH];
	for (int i = 0; i < 6; i++){
		fgets(line, MAX_LINE_LENGTH, file);
		fprintf(file2, "%s", line);
	}
	fprintf(file2, "number of commited files : %d\n", t_commit);
	while (fgets(line, MAX_LINE_LENGTH, file) != NULL){
		fprintf(file2, "%s", line);
	}
	fclose(file2);
	fclose(file);
	remove(path);
	rename(path2, path);
}

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
	char cwd[MAX_LINE_LENGTH];
	if (getcwd(cwd, sizeof(cwd)) == NULL) return NULL;
	char* ret = (char *)malloc(MAX_LINE_LENGTH);
	strcpy(ret, cwd);
	char* p = strstr(ret, ".neogit");
	if (p == NULL) return NULL;
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

char * get_source_path(char *path){
	char * ans = (char *)malloc(strlen(path));
	strcpy(ans, path);
	int ln = strlen(path);
	bool fst = 0;
	for (int i = ln - 1; i >= 0; i--){
		if (ans[i] == '/'){
			ans += i + 1;
			break;
		}
		if (ans[i] == '_'){
			if (fst)
				ans[i] = '/';
			else
				ans[i] = '\0';
			fst = 1;
		}
	}
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
	char tmp[MAX_LINE_LENGTH];
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
		char addr[MAX_LINE_LENGTH],addr2[MAX_LINE_LENGTH];
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

char * find_in_file(char * src, char* key,int num){
	char * line = (char *)malloc(1000);
	FILE * file = fopen(src, "r");
	while (fgets(line, 1000, file) != NULL){
		int ln = strlen(line);
		if (line[ln - 1] == '\n') line[ln - 1] = '\0';
		if (strncmp(line, key, num) == 0) return line;
	}
	return NULL;
}

char* abs_path(char* path){
	char cwd[1000];
	if (getcwd(cwd,1000) == NULL) return NULL;
	strcat(cwd, "/");
	strcat(cwd, path);
	char * ret = (char *)calloc(MAX_LINE_LENGTH,1);
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

char* get_head(char * branch){
	char* head = find_source();
	strcat(head, "/heads");
	char * ans = (char *) malloc(100);
	bool exist = 0;
	char chert[100];
	FILE * H = fopen(head, "r");
	while(fscanf(H, "%s %s", chert, ans) != EOF){
		if (strcmp(chert, branch) == 0){
			exist = 1;
			break;
		}
	}
	fclose(H);
	if (exist) return ans;    
	return NULL;
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
	fprintf(conf, "last_commit_ID: %s\n", last_commit_id);
	fprintf(conf, "current_commit_ID: %s\n", current_commit_id);
	fprintf(conf, "branch: %s\n", branch);
	fclose(conf);
	free(newdir);
	return;
}

int config(int argc, char * const argv[]){    
	bool isglobal = (strcmp(argv[2], "-global") != 0);
	if (argc < 6 - isglobal){
		fprintf(stderr, "Too few arguments\n");
		return 1;
	}
	if (strcmp(argv[2], "-global") == 0){
		if (strcmp(argv[3], "user") == 0){
			int fl = -1;
			if (strcmp(argv[4],"name") == 0) fl = 0;
			else if (strcmp(argv[4],"email") == 0) fl = 1;
			else{
				fprintf(stderr, "Invalid command\n");
				return 1;
			}
			FILE* list = fopen("/home/radal/.base/list","r");
			char dir[1000];
			while (fscanf(list,"%s",dir) != EOF){
				write_config(dir, argv[5], fl);
			}
			fclose(list);
			FILE * conf = fopen("/home/radal/.base/config", "r");
			int num = 0;            
			char user[1000], email[1000];
			user[0] = '\0';
			email[0] = '\0';
			char line[MAX_LINE_LENGTH];
			while (fgets(line , MAX_LINE_LENGTH, conf) != NULL){
				num++;
				if (strncmp(line, "username",8) == 0) sscanf(line, "username: %s", user);
				else sscanf(line, "email: %s", email);
			}
			conf = fopen("/home/radal/.base/config", "w");
			if (fl == 0){
				fprintf(conf, "username: %s\n", argv[5]);
				if (email[0] != '\0') fprintf(conf, "email: %s\n", email);
			}
			else{
				if (user[0] != '\0') fprintf(conf, "username: %s\n", user);
				fprintf(conf, "email: %s\n", argv[5]);
			}
			fclose(conf);
			printf("Global success!\n");
			return 0;
		}
		if (strcmp(argv[3], "alias") == 0){
			char* src = "/home/radal/.base/commands";
			FILE * file = fopen(src, "a");
			fprintf(file, "%s %s\n", argv[4], argv[5]);
			fclose(file);
			FILE* list = fopen("/home/radal/.base/list", "r");
			char* pth = (char *)malloc(1000);
			while (fscanf(list, "%s", pth) != EOF){
				strcat(pth,"/.neogit/commands");
				file = fopen(pth, "a");
				fprintf(file, "%s %s\n", argv[4], argv[5]);
				fclose(file);
			}
			fclose(list);
			free(pth);
			printf("alias %s was added successfully and globaly!\n", argv[4]);
			return 0;
		}
		fprintf(stderr, "What the Fuck do you mean!\n");
		return 1;
	}
	if (find_source() == NULL){
		fprintf(stderr, "neogit storage not found!\n");
		return 1;
	}
	if (strcmp(argv[2], "user") == 0){
		int fl = -1;
		if (strcmp(argv[3],"name") == 0) fl = 0;
		else if (strcmp(argv[3],"email") == 0) fl = 1;
		else{
			fprintf(stderr, "Invalid command\n");
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
		strcat(src, "/commands");
		FILE * file = fopen(src, "a");
		fprintf(file, "%s %s\n", argv[3], argv[4]);
		fclose(file);
		fprintf(stderr, "alias %s was added successfully!\n", argv[3]);
		return 0;
	}
	fprintf(stderr, "What the fuck do you mean!\n");
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
			fprintf(stderr, "Error opening current directory!\n");
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
		FILE * file = fopen("/home/radal/.base/config", "r");
		int num = 0;
		char line[MAX_LINE_LENGTH];
		char user[1000], email[1000];
		while (fgets(line, MAX_LINE_LENGTH, file) != NULL){
			num++;
			if (num == 1)
				sscanf(line, "username: %s", user);
			else
				sscanf(line, "email: %s", email);
		}
		fclose(file);
		if (num < 2){
			fprintf(stderr, "Please configure your email and username!\n");
			return 1;
		}
		if (mkdir(".neogit", 0755) != 0) return 1;
		printf("successfully initialized the project!\n");
		if (find_in_file("/home/radal/.base/list", cwd, strlen(cwd)) == NULL){
			FILE * list = fopen("/home/radal/.base/list", "a");
			fprintf(list,"%s\n",cwd);
			fclose(list);
		}
		return create_configs(user, email);
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

	//create tags folder
	if (mkdir(".neogit/tags", 0755) != 0) return 1;

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

	file = fopen(".neogit/commands", "w");
	fclose(file);
	return 0;
}

void dfs_on_files(char * path, int h, int num_tab){
	int isdir = is_dir(path);
	for (int i = 0; i < num_tab; i++) printf("\t");
	printf("%s", path);
	if (h != 0 || isdir == 0) printf(" : ");
	if (isdir == 0){
		if (is_staged(path)) printf("\033[1;32mstaged\n\033[0m");
		else printf("\033[1;31munstaged\n\033[0m");
		return;
	}
	printf("\n");
	if (h == 0) return;
	DIR *dir = opendir(path);
	struct dirent *entry;
	int ln = strlen(path);
	while ((entry = readdir(dir)) != NULL) {
		if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
		strcat(path,"/");
		strcat(path, entry->d_name);
		dfs_on_files(path, h - 1, num_tab + 1);
		path[ln] = '\0';
	}
	closedir(dir);
}

int run_add(int argc, char *const argv[]) {
	bool isf = (strcmp(argv[2], "-f") == 0),isn = (strcmp(argv[2], "-n") == 0);
	if (argc < 3 + isf + isn){
		fprintf(stderr, "please specify a file\n");
		return 1;
	}
	if (find_source() == NULL){
		fprintf(stderr, "neogit storage not found!\n");
		return 1;
	}

	if (argv[2][0] != '-' || isf){
		char* des = find_source();
		char* des2 = find_source();
		strcat(des, "/staging_");
		strcat(des2, "/deleted_");
		char src[MAX_LINE_LENGTH],src2[MAX_LINE_LENGTH];
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
	if (strcmp(argv[2], "-n") == 0){
		char cwd[MAX_LINE_LENGTH];
		getcwd(cwd, MAX_LINE_LENGTH);
		dfs_on_files(cwd, atoi(argv[3]), 0);
	}
}

int add_to_staging_deleted(char * filepath){
		if (is_tracked(filepath) == 0){
			return 1;
		}
		char* src = find_source();
		strcat(src,"/deleted_0");
		if (find_in_file(src, filepath, 1000) != NULL) return 0;
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
		if (find_in_file(stg, src, strlen(src)) != NULL){
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
			printf("there's no such file or folder as %s!\n", filepath);
			return 1;
		}
		return 0;
	}
	if (isdir == 1){
		struct dirent *entry;
		DIR *dir = opendir(filepath);
		if (dir == NULL){
			fprintf(stderr, "Error opening directory\n!");
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
	strcat(des, "_");
	strcat(des, numtostr(num));
	copy_file(filepath, des);
	track_file(filepath);
	strcat(src, "/staging_0");
	if (find_in_file(src, des, ln) != NULL){
		char line[1000];
		char * tmp = find_source();
		strcat(tmp,"/tmp");
		FILE * file = fopen(src, "r");
		FILE * file2 = fopen(tmp, "w");
		while (fgets(line, 1000, file) != NULL){
			int length = strlen(line) - 1;
			if (line[length] == '\n') line[length] = '\0';
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
	if (argc < 3){
		fprintf(stderr, "Invalid format\n");
		return 1;
	}
	if (find_source() == NULL){
		fprintf(stderr, "neogit storage not found!\n");
		return 1;
	}
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
	char fake[MAX_LINE_LENGTH];
	strcpy(fake, src);
	strcat(src, "/staging_0");
	strcat(fake, "/tmp");
	FILE * fl = fopen(src, "r");
	FILE * tmp = fopen(fake, "w");
	char line[MAX_LINE_LENGTH];
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
	bad = find_source();
	strcat(bad, "/all/");
	strcat(bad,pathto_(filepath));
	ln = strlen(bad);
	while (fgets(line, 1000, fl) != NULL){
		if (strncmp(bad, line, ln) == 0) continue;
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

bool is_dif(char * path1, char* path2){
	FILE * file1 = fopen(path1, "r");
	FILE * file2 = fopen(path2, "r");
	bool dif = 0;
	char line[MAX_LINE_LENGTH],line2[MAX_LINE_LENGTH];
	while (true){
		bool isnl1 = (fgets(line, MAX_LINE_LENGTH, file1) == NULL);
		bool isnl2 = (fgets(line2, MAX_LINE_LENGTH, file2) == NULL);
		if (isnl1 != isnl2){
			dif = 1;
			break;
		}
		if (isnl1) break;
		if (strcmp(line, line2) != 0){
			dif = 1;
			break;
		}
	}
	fclose(file1);
	fclose(file2);
	return dif;
}

bool is_changed(char* path, char* last_commit){
	char* src = find_source();
	strcat(src,"/all/");
	strcat(src,pathto_(path));
	FILE* commit = fopen(last_commit, "r");
	char line[MAX_LINE_LENGTH];
	bool exist = 0;
	while (fgets(line, MAX_LINE_LENGTH, commit) != NULL){
		int ln = strlen(line);
		if (line[ln - 1] == '\n') line[ln - 1] = '\0';
		if (strncmp(line, src, strlen(src)) == 0){
			exist = 1;
			break;
		}
	}
	if (!exist){
		fclose(commit);
		return 1;
	}
	fclose(commit);
	return is_dif(path, line);
}

bool is_staged(char * path){
	char * src = find_source();
	strcat(src, "/all/");
	strcat(src, pathto_(path));
	int ln = strlen(src);
	char * stg = find_source();
	strcat(stg, "/staging_0");
	char * line = find_in_file(stg, src, ln);
	if (line == NULL) return 0;
	return !is_dif(line, path);
}

bool is_staged_del(char * path){
	char * src = find_source();
	strcat(src, "/all/");
	strcat(src, pathto_(path));
	int ln = strlen(src);
	char * del = find_source();
	strcat(del, "/deleted_0");
	if (find_in_file(del, src, ln) != NULL) return 1;
	return 0;
}

char * get_cur_commit(){
	char * conf = find_source();
	strcat(conf,"/config");
	FILE * file = fopen(conf, "r");
	char * cur_com = (char*) malloc(1000);
	for (int i = 0; i < 8; i++) fscanf(file, "%s", cur_com);
	fclose(file);
	return cur_com;
}

char * get_lst_commit(){
	char * conf = find_source();
	strcat(conf,"/config");
	FILE * file = fopen(conf, "r");
	char * lst_com = (char*) malloc(1000);
	for (int i = 0; i < 6; i++) fscanf(file, "%s", lst_com);
	fclose(file);
	return lst_com;
}

char * get_cur_branch(){
	char * conf = find_source();
	if (conf == NULL)
		return NULL;
	strcat(conf,"/config");
	FILE * file = fopen(conf, "r");
	char * line = (char *)malloc(MAX_LINE_LENGTH);
	for (int i = 0; i < 5; i++) fgets(line, 1000, file);
	fclose(file);
	char tmp[100];
	char * branch = (char *)malloc(100);
	sscanf(line, "%s %s", tmp, branch);   
	return branch;
}

void check_status(char * path){
	int isdir = is_dir(path);
	if (isdir == 1){
		DIR *dir = opendir(path);
		struct dirent *entry;
		int ln = strlen(path);
		while ((entry = readdir(dir)) != NULL) {
			if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
			strcat(path,"/");
			strcat(path, entry->d_name);
			check_status(path);
			path[ln] = '\0';
		}
		closedir(dir); 
		return;
	}
	char * branch = get_cur_branch();
	char * head = get_head(branch);
	char * comm = find_source();
	strcat(comm,"/commits/");
	strcat(comm, head);
	char * nw_path = find_source();
	char * line;
	strcat(nw_path, "/all/");
	strcat(nw_path, pathto_(path));
	int ln = strlen(nw_path);
	line = find_in_file(comm, nw_path, ln);
	if (line == NULL){
		printf("%s : ", path);
		if (is_staged(path)) printf("+A\n");
		else printf("-A\n");
		return;
	}
	if (!is_dif(line, path)) return;
	printf("%s : ", path);
	if (is_staged(path)) printf("+M\n");
	else printf("-M\n");
}

int run_status(){
	char * path = find_source();
	if (path == NULL){
		fprintf(stderr, "neogit storage not found!\n");
		return 1;
	}
	strcat(path, "/files");
	check_status(path);
	char * branch = get_cur_branch();
	char * head = get_head(branch);
	char * comm = find_source();
	strcat(comm,"/commits/");
	strcat(comm, head);
	char * line = (char*)malloc(MAX_LINE_LENGTH);
	FILE * file = fopen(comm, "r");
	for (int i = 0; i < 8; i++) fgets(line, MAX_LINE_LENGTH, file);
	while (fgets(line, MAX_LINE_LENGTH, file) != NULL){
		line = get_source_path(line);
		FILE * file2 = fopen(line, "r");
		if (file2 == NULL){
			printf("%s : ", line);
			if (is_staged_del(line)) printf("+D\n");
			else printf("-D\n");
		}
		else fclose(file2);
	}
	fclose(file);
	return 0;
}

int run_commit(int argc, char * const argv[]) {
	if (argc < 4) {
		fprintf(stderr,"please use the correct format!\n");
		return 1;
	}
	if (find_source() == NULL){
		fprintf(stderr, "neogit storage not found!\n");
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
	char chert[1000],user[1000], email[1000], branch[1000];
	fscanf(file2, "%s %s", chert, user);
	fscanf(file2, "%s %s", chert, email);
	for (int i = 0; i < 5; i++) fscanf(file2, "%s", chert);
	fscanf(file2, "%s", branch);
	char* head = find_source();
	strcat(head, "/heads");
	char* par = get_head(branch);
	fclose(file2);

	int t_commit = 0;
	char* src_stage = find_source();
	strcat(src_stage, "/staging_0");
	file2 = fopen(src_stage, "r");
	char line[MAX_LINE_LENGTH];
	while(fgets(line, MAX_LINE_LENGTH, file2) != NULL){
		t_commit++;
	}
	fclose(file2);
	char * src_deleted = find_source();
	strcat(src_deleted, "/deleted_0");
	file2 = fopen(src_deleted, "r");
	while(fgets(line, MAX_LINE_LENGTH, file2) != NULL){
		t_commit++;
	}  
	fclose(file2);
	if (t_commit == 0){
		fprintf(stderr, "there's no staged file\n");
		return 1;
	}
	t_commit = 0;
	//write commit info
	FILE * file = fopen(src, "w");
	int day = tm.tm_mday, mon = tm.tm_mon + 1, year = tm.tm_year + 1900, hour = tm.tm_hour;
	int min = tm.tm_min,sec = tm.tm_sec;
	fprintf(file, "TIME : %d/%d/%d    %d:%d:%d\n", day, mon, year, hour, min, sec);
	fprintf(file, "MESSAGE : %s\n", message);
	fprintf(file, "user : %s\n", user);
	fprintf(file, "email : %s\n", email);
	fprintf(file, "commit_id : %d\n", commit_ID);
	fprintf(file, "branch : %s\n", branch);
	fprintf(file, "par : %s\n", par);
	//write commit
	char * last_commit = find_source();
	strcat(last_commit, "/commits/");
	strcat(last_commit, par);
	FILE* lst = fopen(last_commit, "r");
	if (strcmp(par,"0") != 0) for (int i = 0; i < 8; i++) fgets(line, MAX_LINE_LENGTH, lst);
	while (fgets(line, MAX_LINE_LENGTH, lst) != NULL){
		if (find_in_file(src_deleted, line, strlen(line) - 6) != NULL) continue;
		if (find_in_file(src_stage, line, strlen(line) - 6) != NULL) continue;
		t_commit++;
		fprintf(file, "%s", line);
	}
	fclose(lst);
	file2 = fopen(src_stage, "r");
	while (fgets(line, MAX_LINE_LENGTH, file2) != NULL){
		t_commit++;
		fprintf(file, "%s", line);
	}
	fclose(file);
	fclose(file2);
	add_tcommit(t_commit, src);
	//change head
	char* tmp = find_source();
	strcat(tmp, "/tmp");
	file2 = fopen(tmp, "w");
	file = fopen(head, "r");
	while (fgets(line, MAX_LINE_LENGTH, file) != NULL){
		if (strncmp(line, branch, strlen(branch)) == 0)
			fprintf(file2, "%s %d\n", branch, commit_ID);
		else
			fprintf(file2,"%s", line);
	}
	fclose(file2);
	fclose(file);
	remove(head);
	rename(tmp, head);

	//write config
	char * conf = find_source();
	strcat(conf, "/config");
	file2 = fopen(tmp, "w");
	file = fopen(conf, "r");
	for (int i = 0; i < 5; i++){
		fgets(line, MAX_LINE_LENGTH, file);
		if (i != 3){
			fprintf(file2, "%s", line);
			continue;
		}
		fprintf(file2, "current_commit_ID: %d\n", commit_ID);
	}
	fclose(file);
	fclose(file2);
	remove(conf);
	rename(tmp, conf);
	if (reset_staging()) return 1;
	free(tmp);
	free(last_commit);
	free(src_deleted);
	printf("commit successfully with commit ID %d\n", commit_ID);
	printf("commit message : %s\n", message);
	printf("TIME : %d/%d/%d    %d:%d:%d\n", day, mon, year, hour, min, sec);
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
	return (find_in_file(src,filepath,1000) != NULL);
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
	int ln = strlen(line);
	if (line[ln - 1] == '\n') line[ln - 1] = '\0';
	return line + 10;
}

char * get_par(char * commit_id){
	char* line = (char*) malloc(1000);
	char * src = find_source();
	strcat(src, "/commits/");
	strcat(src, commit_id);
	FILE * file = fopen(src, "r");
	for (int i = 0; i < 8; i++) fgets(line, 1000, file);
	fclose(file);
	free(src);        
	int ln = strlen(line);
	if (line[ln - 1] == '\n') line[ln - 1] = '\0';
	return line + 6;
}

int run_log(int argc, char * const argv[]){
	if (find_source() == NULL){
		fprintf(stderr, "neogit storage not found!\n");
		return 1;
	}
	bool isbr = (argc > 2 && (strcmp("-branch", argv[2]) == 0));
	bool isauth = (argc > 2 && (strcmp("-author", argv[2]) == 0));
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
	strcat(address, "/commits");
	DIR * dir = opendir(address);
	struct dirent *entry;
	if (argc == 4 && (strcmp(argv[2], "-since") == 0 || strcmp(argv[2], "-before") == 0)){
		int dd,mm,yy;
		sscanf(argv[3], "%d/%d/%d", &dd, &mm, &yy);
		strcat(address,"/");
		int ln = strlen(address);
		while ((entry = readdir(dir)) != NULL){
			if (entry->d_type != DT_REG || strcmp(entry->d_name,"0") == 0) continue;
			int dd2,mm2,yy2;
			strcat(address, entry->d_name);
			FILE* file = fopen(address, "r");
			fscanf(file, "TIME : %d/%d/%d", &dd2, &mm2, &yy2);
			fclose(file);
			address[ln] = '\0';
			if (strcmp(argv[2], "-since") == 0){
				if (yy2 < yy || (yy2 == yy && mm2 < mm) || (yy2 == yy && mm2 == mm && dd2 < dd)) continue;
			}
			else{
				if (yy2 > yy || (yy2 ==yy && mm2 > mm) || (yy2 == yy && mm2 == mm && dd2 > dd)) continue;
			}
			coms[num] = (char *)malloc(20);
			strcpy(coms[num],entry->d_name);
			num++;
		}  
	}

	else if (argc == 2 || (argc == 4 && (strcmp("-n", argv[2]) == 0 || isbr || isauth))){
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
	char line[MAX_LINE_LENGTH];
	if (argc == 4 && strcmp(argv[2], "-n") == 0){
		int n = atoi(argv[3]);
		if (num > n) num = n;
	}
	for (int i = 0; i < num; i++){
		strcat(address, coms[i]);
		FILE * file = fopen(address, "r");
		for (int j = 0; j < 7; j++){
			fgets(line, MAX_LINE_LENGTH, file);
			printf("%s", line);
		}
		printf("---------------------------------\n\n");
		fclose(file);
		address[ln] = '\0';
	}
	free(address);
	return 0;        
}

int run_branch(int argc, char * const argv[]){
	char * src = find_source();
	if (src == NULL){
		fprintf(stderr, "neogit storage not found!\n");
		return 1;
	}
	strcat(src, "/heads");
	if (argc == 2){
		FILE * file = fopen(src, "r");
		char line[MAX_LINE_LENGTH],name[MAX_LINE_LENGTH];
		while (fgets(line, MAX_LINE_LENGTH, file) != NULL){
			sscanf(line, "%s", name);
			printf("%s\n",name);
		}
		fclose(file);
		return 0;
	}
	if (argc != 3){
		fprintf(stderr, "Wrong format!\n");
		return 1;
	}
	if (find_in_file(src, argv[2], strlen(argv[2])) != NULL){
		fprintf(stderr, "Branch already exists\n");
		return 1;
	}
	char * branch = get_cur_branch();
	char * head = get_head(branch);
	src = find_source();
	strcat(src, "/commits/");
	char * dst = (char *)malloc(MAX_LINE_LENGTH);
	char * conf = find_source();
	strcat(conf, "/config");
	strcpy(dst, src);
	strcat(src, head);
	int commit_ID = inc_last_commit_ID();
	strcat(dst, numtostr(commit_ID));
	FILE * file2 = fopen(dst, "w");
	FILE * file_conf = fopen(conf, "r");
	time_t t = time(NULL);
	struct  tm tm = *localtime(&t);
	char chert[1000],user[1000], email[1000];
	fscanf(file_conf, "%s %s", chert, user);
	fscanf(file_conf, "%s %s", chert, email);
	fclose(file_conf);
	//commit info
	fprintf(file2, "TIME : %d/%d/%d    %d:%d:%d\n", tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900, tm.tm_hour, tm.tm_min, tm.tm_sec);
	fprintf(file2, "MESSAGE : %s\n", "new branch created");
	fprintf(file2, "user : %s\n", user);
	fprintf(file2, "email : %s\n", email);
	fprintf(file2, "commit_id : %d\n", commit_ID);
	fprintf(file2, "branch : %s\n", argv[2]);
	fprintf(file2, "number of commited files : %d\n", 0);    
	fprintf(file2, "par : %s\n", head);
	//commit files
	FILE * file = fopen(src, "r");
	char line[MAX_LINE_LENGTH];
	for (int i = 0; i < 8; i++) fgets(line, MAX_LINE_LENGTH, file);
	while (fgets(line, MAX_LINE_LENGTH, file) != NULL){
		fprintf(file2, "%s", line);
	}
	fclose(file);
	fclose(file2);

	// update heads
	char * head_path = find_source();
	strcat(head_path, "/heads");
	file = fopen(head_path, "a");
	fprintf(file, "%s %d\n", argv[2], commit_ID);
	fclose(file);
	printf("branch made\n");
	return 0;
}

void delete_all_files(char * path){
	int isdir = is_dir(path);
	if (isdir == 0){
		remove(path);
		return;
	}
	struct dirent *entry;
	DIR * dir = opendir(path);
	strcat(path, "/");
	int ln = strlen(path);
	while ((entry = readdir(dir)) != NULL){
		if (strcmp(entry->d_name,".") == 0 || strcmp(entry->d_name,"..") == 0) continue;
		strcat(path, entry->d_name);
		delete_all_files(path);
		path[ln] = '\0';
	} 
	closedir(dir);
}

int goto_commit(char * commit_id){
	char * src_files = find_source();
	strcat(src_files, "/files");
	delete_all_files(src_files);
	free(src_files);
	char * commit = find_source();
	strcat(commit, "/commits/");
	strcat(commit, commit_id);
	FILE * com_file = fopen(commit, "r");
	char line[MAX_LINE_LENGTH];
	for (int i = 0; i < 8; i++) fgets(line, MAX_LINE_LENGTH, com_file);
	while (fgets(line, MAX_LINE_LENGTH, com_file) != NULL){
		int ln = strlen(line) - 1;
		if (line[ln] == '\n') line[ln] = '\0';
		char *des = (char *)malloc(MAX_LINE_LENGTH);
		strcpy(des, line);
		bool fst = 0;
		for (int i = ln - 1; i >= 0; i--){
			if (des[i] == '_'){
				if (fst == 0){
					fst = 1;
					des[i] = '\0';
				}
				else des[i] = '/';
			}
			else if (des[i] == '/'){
				des += (i + 1);
				break;
			}
		}
		copy_file(line, des);
	}
	fclose(com_file);
	return 0;
}

bool check_change_recur(char * path){
	int isdir = is_dir(path);
	if (isdir == 1){
		DIR *dir = opendir(path);
		struct dirent *entry;
		int ln = strlen(path);
		while ((entry = readdir(dir)) != NULL) {
			if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
			strcat(path,"/");
			strcat(path, entry->d_name);
			if (check_change_recur(path)){
				closedir(dir);
				return 1;
			}
			path[ln] = '\0';
		}
		closedir(dir); 
		return 0;
	}
	char * branch = get_cur_branch();
	char * head = get_head(branch);
	char * comm = find_source();
	strcat(comm,"/commits/");
	strcat(comm, head);
	char * nw_path = find_source();
	char * line;
	strcat(nw_path, "/all/");
	strcat(nw_path, pathto_(path));
	int ln = strlen(nw_path);
	line = find_in_file(comm, nw_path, ln);
	if (line == NULL) return 1;
	return is_dif(line, path);
}

bool check_change(){
	char * path = find_source();
	strcat(path, "/files");
	if (check_change_recur(path)) return 1;
	char * branch = get_cur_branch();
	char * head = get_head(branch);
	char * comm = find_source();
	strcat(comm,"/commits/");
	strcat(comm, head);
	char * line = (char*)malloc(MAX_LINE_LENGTH);
	FILE * file = fopen(comm, "r");
	for (int i = 0; i < 8; i++) fgets(line, MAX_LINE_LENGTH, file);
	while (fgets(line, MAX_LINE_LENGTH, file) != NULL){
		line = get_source_path(line);
		FILE * file2 = fopen(line, "r");
		if (file2 == NULL) return 1;
		else fclose(file2);
	}
	fclose(file);
	return 0;
}

int run_checkout(int argc, char* const argv[]){
	if (argc < 3){
		fprintf(stderr, "Please enter a valid commit id\n");
		return 1;
	}
	if (find_source == NULL){
		fprintf(stderr, "neogit storage not found!\n");
		return 1;
	}
	if (check_change() && strcmp(argv[2], "HEAD") != 0){
		fprintf(stderr, "You have uncommited changes!\n");
		return 1;
	}
	char * commit_id;
	if (strncmp(argv[2], "HEAD", 4) == 0){
		commit_id = get_head(get_cur_branch());
		int n;
		if (strcmp(argv[2], "HEAD") == 0) n = 0;
		else n = atoi(argv[2] + 5);
		while (n--){
			char * par = get_par(commit_id);
			if (strcmp(par, "0") == 0) break;
			commit_id = par;
		}
	}
	else{
		commit_id = get_head(argv[2]);
		if (commit_id == NULL){
			 commit_id = argv[2];
		}
	}
	printf("You checkouted to commit %s\n", commit_id);
	goto_commit(commit_id);
	char* nw_br = get_branch(commit_id);
	char * conf = find_source();
	char * tmp = find_source();
	strcat(conf, "/config");
	strcat(tmp, "/tmp");
	FILE* src = fopen(conf, "r");
	FILE* des = fopen(tmp, "w");
	char line[MAX_LINE_LENGTH];
	for (int i = 0; i < 3; i++){
		fgets(line, MAX_LINE_LENGTH, src);
		fprintf(des, "%s", line);
	}
	fprintf(des, "current_commit_ID: %s\n", commit_id);
	fprintf(des, "branch: %s\n", nw_br);
	fclose(des);
	fclose(src);
	remove(conf);
	rename(tmp, conf);
	return 0;
}

bool check_merge_happend(int commit_id){
	char * merg = find_source();
	strcat(merg, "/last_merge");
	FILE * merge = fopen(merg, "r");
	if (merge != NULL){
		int last_merge;
		fscanf(merge, "%d", &last_merge);
		fclose(merge);
		if (last_merge > commit_id){
			return 1;
		}
	}
	return 0;
}

int run_revert(int argc, char * const argv[]){
	char * src = find_source();
	if (src == NULL){
		fprintf(stderr, "neogit storage not found!\n");
		return 1;
	}
	if (argc >= 3 && strcmp(argv[2], "-n") == 0){
		char * commit_id;
		if (argc == 4) commit_id = argv[3];
		else commit_id = get_lst_commit();
		if (check_merge_happend(atoi(commit_id))){
			fprintf(stderr, "some merge happened after this commit\n");
			return 1;
		}
		goto_commit(commit_id);
		printf("Project is now at %s\n", commit_id);
		return 0;
	}
	int is_m = (strcmp(argv[2], "-m") == 0);
	if (argc < 3 + 2*is_m){
		fprintf(stderr, "Invalid format\n");
		return 1;
	}
	char * message = NULL;
	if (is_m) message = argv[3];
	char * commit_id;
	if (strncmp(argv[argc - 1], "HEAD-", 5) == 0){
		int n = atoi(argv[argc - 1] + 5);
		commit_id = get_cur_commit();
		while (n--){
			char * par = get_par(commit_id);
			if (strcmp(par,"0") == 0) break;
			commit_id = par;
		}
	}
	else commit_id = argv[argc - 1];
	if (check_merge_happend(atoi(commit_id))){
		fprintf(stderr, "some merge happened after this commit\n");
		return 1;        
	}
	if (!is_m) message = get_messaage(commit_id);
	goto_commit(commit_id);
	int nw_id = inc_last_commit_ID();
	strcat(src, "/commits/");
	char * des = (char *)malloc(MAX_LINE_LENGTH);
	strcpy(des, src);
	strcat(des, numtostr(nw_id));
	strcat(src, commit_id);
	char * conf = find_source();
	strcat(conf, "/config");
	FILE* file = fopen(conf, "r");
	char user[100],email[1000];
	fscanf(file, "%s %s", user, user);
	fscanf(file, "%s %s", email, email);
	fclose(file);
	file = fopen(src, "r");
	char line[MAX_LINE_LENGTH];
	for (int i = 0; i < 5; i++) fgets(line, MAX_LINE_LENGTH, file);
	FILE* file2 = fopen(des, "w");
	time_t t = time(NULL);
	struct  tm tm = *localtime(&t);
	fprintf(file2, "time : %d/%d/%d    %d:%d:%d\n", tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900, tm.tm_hour, tm.tm_min, tm.tm_sec);
	fprintf(file2, "username: %s\n", user);
	fprintf(file2, "email: %s\n", email);
	fprintf(file2, "message: %s\n", message);
	fprintf(file2, "commit_id: %d\n", nw_id);    
	while (fgets(line, MAX_LINE_LENGTH, file) != NULL){
		fprintf(file2, "%s", line);
	}
	fclose(file2);
	fclose(file);
	printf("Project is now at %d\n", nw_id);
	return 0;
}

int run_tag(int argc, char* const argv[]){
	char * src = find_source();
	if (src == NULL){
		fprintf(stderr, "neogit storage not found!\n");
		return 1;
	}
	if (argc == 2){
		strcat(src,"/tags");
		char* tags[1000];
		int num_tag = 0;
		struct dirent *entry;
		DIR *dir = opendir(src);
		if (dir == NULL) {
			fprintf(stderr, "Error opening current directory!\n");
			return 1;
		}
		while ((entry = readdir(dir)) != NULL) {
			if (entry->d_type == DT_DIR) continue;
			tags[num_tag] = (char *)malloc(strlen(entry->d_name) + 10);
			strcpy(tags[num_tag], entry->d_name);
			num_tag++;
		}
		closedir(dir);        
		for (int i = 1; i < num_tag; i++){
			int j = i; 
			while (j > 0 && strcmp(tags[j],tags[j-1]) < 0){
				char * tmp = tags[j];
				tags[j - 1] = tags[j];
				tags[j] = tmp;
				j--;
			}
		}
		for (int i = 0; i < num_tag; i++){
			printf("%s\n", tags[i]);
		}
		return 0;
	}
	if (argc == 4 && strcmp(argv[2], "show") == 0){
		strcat(src, "/tags/");
		strcat(src, argv[3]);
		FILE * file = fopen(src, "r");
		if (file == NULL){
			fprintf(stderr, "Invalid tag name\n");
			return 1;
		}
		char line[MAX_LINE_LENGTH];
		while (fgets(line, MAX_LINE_LENGTH, file) != NULL)
			printf("%s",line);
		fclose(file);
		return 0;
	}
	if (argc >= 4 && strcmp(argv[2], "-a") == 0){
		char * message = "";
		bool isf = (strcmp(argv[argc - 1], "-f") == 0);
		if (argc >= 6 && strcmp(argv[4], "-m") == 0){
			message = argv[5];
		}
		char * commit_id = NULL;
		for (int i = 4; i + 1 < argc; i++){
			if (strcmp(argv[i], "-c") == 0){
				commit_id = argv[i + 1];
				break;
			}
		}
		strcat(src, "/config");
		if (commit_id == NULL)
			commit_id = get_cur_commit();
		FILE* file = fopen(src, "r");
		char user[1000],email[1000];
		fscanf(file, "username: %s", user);
		fscanf(file, "%s %s", email, email);
		fclose(file);

		src = find_source();
		strcat(src, "/tags/");
		strcat(src, argv[3]);
		file = fopen(src, "r");
		if (file != NULL){
			fclose(file);
			if (isf == 0){
				fprintf(stderr, "tag already exists!\n");
				return 1;
			}
		}
		file = fopen(src, "w");
		time_t t = time(NULL);
		struct  tm tm = *localtime(&t);
		fprintf(file, "time : %d/%d/%d    %d:%d:%d\n", tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900, tm.tm_hour, tm.tm_min, tm.tm_sec);
		fprintf(file, "username: %s\n", user);
		fprintf(file, "email: %s\n", email);
		fprintf(file, "message: %s\n", message);
		fprintf(file, "commit_id: %s\n", commit_id);
		fclose(file);
		printf("tag built successfully\n");
		return 0;
	}
	fprintf(stderr, "Invalid command\n");
	return 1;
}

bool iswhitespace(char c){
	if (c == ' ' || c == '\n' || c == '\t') return 1;
	return 0;
}

int* filetoint(char * path){
	FILE * file = fopen(path, "r");
	int * ans = (int *) malloc(MAX_LINE_NUMBER * sizeof(int));
	int num = 0,line_num = 0;
	char line[MAX_LINE_LENGTH],word[MAX_LINE_LENGTH];
	while (fgets(line, MAX_LINE_LENGTH, file) != NULL){
		int i = 0;
		while (iswhitespace(line[i])) i++;
		line_num++;
		if (line[i] == '\0') continue;
		ans[num] = line_num;
		num++;
	}
	fclose(file);
	return ans;   
}

char ** filetochar(char * path){
	FILE * file = fopen(path, "r");
	char ** ans = (char **) malloc(MAX_LINE_NUMBER * sizeof(char *));
	int num = 0;
	char line[MAX_LINE_LENGTH],word[MAX_LINE_LENGTH];
	while (fgets(line, MAX_LINE_LENGTH, file) != NULL){
		int sum_size = 0;
		while (iswhitespace(line[sum_size])) sum_size++;
		if (line[sum_size] == '\0') continue;
		ans[num] = (char *) malloc(MAX_LINE_LENGTH);
		ans[num][0] = '\0';
		strcpy(ans[num], line);
		num++;
	}
	for (int i = num; i < MAX_LINE_NUMBER; i++) ans[i] = NULL;
	fclose(file);
	return ans;
}

char * num2line(char * path, int line_num){
	char * line = malloc(MAX_LINE_LENGTH);
	FILE * file = fopen(path, "r");
	while (line_num--){
		fgets(line, MAX_LINE_LENGTH, file);
	}
	fclose(file);
	return line;
}

bool get_diff(int argc, char * const argv[]){
	bool iscommit = (strcmp(argv[0], "neogit") != 0);
	bool isdif = 0;
	char * path1 = argv[3];
	char * path2 = argv[4];
	char** first = filetochar(path1);
	char** second = filetochar(path2);
	int * line_id1 = filetoint(path1);
	int * line_id2 = filetoint(path2);
	int beg[2] = {0,0},en[2] = {MAX_LINE_NUMBER - 1, MAX_LINE_NUMBER - 1};
	while (en[0] >= 0 && first[en[0]] == NULL) en[0]--;
	while (en[1] >= 0 && second[en[1]] == NULL) en[1]--;
	if (argc >= 7){
		int x,y;
		if (strcmp(argv[5], "-line1") == 0){
			sscanf(argv[6], "%d-%d", &x, &y);
			beg[0] = en[0] + 1;
			for (int i = 0; i <= en[0]; i++){
				if (line_id1[i] >= x){
					beg[0] = i;
					break;
				}
			}
			int tmp = en[0];
			en[0] = -1;
			for (int i = tmp; i >= 0; i--){
				if (line_id1[i] <= y){
					en[0] = i;
					break;
				}
			}
		}
		else if (strcmp(argv[5], "-line2") == 0){
			sscanf(argv[6], "%d-%d", &x, &y);
			beg[1] = en[1] + 1;
			for (int i = 0; i <= en[1]; i++){
				if (line_id2[i] >= x){
					beg[1] = i;
					break;
				}
			}
			int tmp = en[1];
			en[1] = -1;
			for (int i = tmp; i >= 0; i--){
				if (line_id2[i] <= y){
					en[1] = i;
					break;
				}
			}
		}
		if (argc == 9){
			sscanf(argv[8], "%d-%d", &x, &y);
			beg[1] = en[1] + 1;
			for (int i = 0; i <= en[1]; i++){
				if (line_id2[i] >= x){
					beg[1] = i;
					break;
				}
			}
			int tmp = en[1];
			en[1] = -1;
			for (int i = tmp; i >= 0; i--){
				if (line_id2[i] <= y){
					en[1] = i;
					break;
				}
			}
		}
	}
	while (beg[0] <= en[0] && beg[1] <= en[1]){
		if (strcmp(first[beg[0]], second[beg[1]]) == 0){
			beg[0]++;
			beg[1]++;
			continue;
		}
		isdif = 1;
		printf("\033[1;31m<<<<<<<<<\n\033[0m");
		if (iscommit == 0)
			printf("\033[1;31m%s line %d\n\033[0m", argv[3], line_id1[beg[0]]);
		else
			printf("\033[1;31m %s in %s line %d\n\033[0m", get_source_path(argv[3]), argv[0], line_id1[beg[0]]);
		printf("\033[1;31m%s\033[0m", num2line(argv[3], line_id1[beg[0]]));
		printf("\033[1;32m>>>>>>>>>\n\033[0m");
		if (iscommit == 0)
			printf("\033[1;32m%s line %d\n\033[0m", argv[4], line_id2[beg[1]]);
		else
			printf("\033[1;32m %s in %s line %d\n\033[0m", get_source_path(argv[4]), argv[1], line_id2[beg[1]]);
		printf("\033[1;32m%s\033[0m", num2line(argv[4], line_id2[beg[1]]));
		beg[0]++;
		beg[1]++;
	}
	while (beg[0] <= en[0]){
		isdif = 1;
		printf("\033[1;31m<<<<<<<<<\n\033[0m");
		if (iscommit == 0)
			printf("\033[1;31m%s line %d\n\033[0m", argv[3], line_id1[beg[0]]);
		else
			printf("\033[1;31m %s in %s line %d\n\033[0m", get_source_path(argv[3]), argv[0], line_id1[beg[0]]);
		printf("\033[1;31m%s\033[0m", num2line(argv[3], line_id1[beg[0]]));        
		beg[0]++;
	}
	while (beg[1] <= en[1]){
		isdif = 1;
		printf("\033[1;32m>>>>>>>>>\n\033[0m");
		if (iscommit == 0)
			printf("\033[1;32m%s line %d\n\033[0m", argv[4], line_id2[beg[1]]);
		else
			printf("\033[1;32m %s in %s line %d\n\033[0m", get_source_path(argv[4]), argv[1], line_id2[beg[1]]);
		printf("\033[1;32m%s\033[0m", num2line(argv[4], line_id2[beg[1]]));        
		beg[1]++;
	}
	return isdif;
}

bool get_diff_com(int argc, char * const argv[],bool flag){ // 0 if new should be ignored
	char* path1 = find_source();
	char* path2 = find_source();
	strcat(path1, "/commits/");
	strcat(path1, argv[3]);
	strcat(path2, "/commits/");
	strcat(path2, argv[4]);
	FILE * file = fopen(path1, "r");
	if (file == NULL){
		fprintf(stderr, "Invalid commit id\n");
		return 0;
	}
	bool isdif = 0;
	char line[MAX_LINE_LENGTH];
	for (int i = 0; i < 8; i++) fgets(line, MAX_LINE_LENGTH, file);
	while (fgets(line, MAX_LINE_LENGTH, file) != NULL){
		int ln = strlen(line);
		if (line[ln - 1] == '\n')
			line[ln - 1] = '\0';
		while (line[ln] != '_') ln--;
		char * other = find_in_file(path2, line, ln);
		if (other == NULL && flag){
			isdif = 1;
			printf("\033[1;31mOnly commit %s has %s\n\033[0m", argv[3], get_source_path(line));
			continue;
		}
		char* arg[5] = {argv[3], argv[4], "chert", line, other};
		isdif |= get_diff(5, arg);
	}
	fclose(file);
	file = fopen(path2, "r");
	for (int i = 0; i < 8; i++) fgets(line, MAX_LINE_LENGTH, file);
	while (fgets(line, MAX_LINE_LENGTH, file) != NULL){
		int ln = strlen(line);
		if (line[ln - 1] == '\n')
			line[ln - 1] = '\0';
		while (line[ln] != '_') ln--;
		char * other = find_in_file(path1, line, ln);
		if (other == NULL && flag){
			isdif = 1;
			printf("\033[1;32mOnly commit %s has %s\n\033[0m", argv[4], get_source_path(line));
			continue;
		}
	}
	fclose(file);
	return isdif;
}

int run_diff(int argc, char * argv[]){
	if (argc < 5 || argc%2 == 0){
		fprintf(stderr, "Invalid format!\n");
		return 1;
	}
	if (strcmp(argv[2], "-f") == 0){
		argv[3] = abs_path(argv[3]);
		argv[4] = abs_path(argv[4]);
		get_diff(argc, argv);
		return 0;
	}
	if (strcmp(argv[2], "-c") == 0){
		if (find_source() == NULL){
			fprintf(stderr, "neogit storage not found!\n");
			return 1;
		}
		get_diff_com(argc, argv, 1);
		return 0;
	}
}

int run_merge(int argc, char * argv[]){
	if (argc < 5){
		fprintf(stderr, "Invalid format!\n");
		return 1;
	}
	char * src = find_source();
	if (src == NULL){
		fprintf(stderr, "neogit storage not found!\n");
		return 1;
	}
	char* head1 = get_head(argv[3]);
	char* head2 = get_head(argv[4]);
	char* arg[5] = {"neogit", "diff", "-c", head1, head2};
	if (get_diff_com(5,arg, 0)){
		fprintf(stderr, "merge failed\n");
		return 1;
	}
	char user[100], email[100];
	char* conf = find_source();
	strcat(conf,"/config");
	FILE* file2 = fopen(conf, "r");
	fscanf(file2, "%s %s", user, user);
	fscanf(file2, "%s %s", email, email);
	fclose(file2);
	int commit_id = inc_last_commit_ID();
	char * des = find_source();
	strcat(des, "/commits/");
	strcat(des, numtostr(commit_id));
	FILE * file = fopen(des, "w");
	time_t t = time(NULL);
	struct  tm tm = *localtime(&t);

	fprintf(file, "TIME : %d/%d/%d    %d:%d:%d\n", tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900, tm.tm_hour, tm.tm_min, tm.tm_sec);
	fprintf(file, "MESSAGE : merge commit\n");
	fprintf(file, "user : %s\n", user);
	fprintf(file, "email : %s\n", email);
	fprintf(file, "commit_id : %d\n", commit_id);
	fprintf(file, "branch : %s%s\n", argv[3], argv[4]);
	fprintf(file, "par : %s %s\n", head1, head2);

	strcat(src, "/commits/");
	strcat(src, head1);
	int t_commit = 0;
	file2 = fopen(src, "r");
	char line[MAX_LINE_LENGTH];
	for (int i = 0; i < 8; i++) fgets(line, MAX_LINE_LENGTH, file2);
	while (fgets(line, MAX_LINE_LENGTH, file2) != NULL){
		t_commit++;
		fprintf(file, "%s", line);
	}
	fclose(file2);
	char * src2 = find_source();
	strcat(src2, "/commits/");
	strcat(src2, head2);
	file2 = fopen(src2, "r");
	for (int i = 0; i < 8; i++) fgets(line, MAX_LINE_LENGTH, file2);
	while (fgets(line, MAX_LINE_LENGTH, file2) != NULL){
		int length = strlen(line);
		while (line[length] != '_') length--;
		if (find_in_file(src, line, length) != NULL) continue;
		t_commit++;
		fprintf(file, "%s", line);
	}
	fclose(file2);
	fclose(file);
	add_tcommit(t_commit, des);
	//update heads
	char * head_path = find_source();
	strcat(head_path, "/heads");
	file = fopen(head_path, "a");
	char branch[MAX_LINE_LENGTH];
	strcpy(branch, argv[3]);
	strcat(branch, argv[4]);
	fprintf(file, "%s %d\n", branch, commit_id);
	fclose(file);
	printf("successful merge!\n");
	// update last_merge
	char * last_merge = find_source();
	strcat(last_merge, "/last_merge");
	file2 = fopen(last_merge, "w");
	fprintf(file2, "%d", commit_id);
	fclose(file2);
	return 0;
}

void grep_in_file(char * path, char* word, bool has_n){
	FILE * file = fopen(path, "r");
	char line[MAX_LINE_LENGTH];
	int line_num = 0;
	while (fgets(line, MAX_LINE_LENGTH, file) != NULL){
		line_num++;
		if (strstr(line, word) != NULL){
			if (has_n) printf("%d- ", line_num);
			printf("%s", line);
		}
	}
	fclose(file);
}

int run_grep(int argc, char * const argv[]){
	if (argc < 6){
		fprintf(stderr, "Invalid format!\n");
		return 1;
	}
	bool has_n = (strcmp(argv[argc - 1], "-n") == 0);
	if (argc < 8){
		grep_in_file(abs_path(argv[3]), argv[5], has_n);
		return 0;
	}
	char * path = find_source();
	if (path == NULL){
		fprintf(stderr, "neogit storage not found!\n");
		return 1;
	}
	strcat(path, "/commits/");
	strcat(path, argv[7]);
	FILE * file = fopen(path, "r");
	char line[MAX_LINE_LENGTH];
	char * address = find_source();
	strcat(address, "/all/");
	strcat(address,pathto_(abs_path(argv[3])));
	int length = strlen(address);
	for (int i = 0; i < 8; i++) fgets(line, MAX_LINE_LENGTH, file);
	while (fgets(line, MAX_LINE_LENGTH, file) != NULL){
		int ln = strlen(line) - 1;
		if (line[ln] == '\n') line[ln] = '\0';
		if (strncmp(line, address, length) == 0){
			grep_in_file(line, argv[5], has_n);
			break;
		}
	}
	fclose(file);
	return 0;
}

int main(int argc, char *argv[]) {
	if (argc < 2) {
		fprintf(stdout, "please enter a valid command\n");
		return 1;
	}
	char * src = find_source();
	if (src != NULL){
		strcat(src, "/commands");
	}
	else{
		src = (char *)malloc(100);
		src = "/home/radal/.base/commands";
	}
	FILE * file  = fopen(src, "r");
	char line[MAX_LINE_LENGTH], word[MAX_LINE_LENGTH];
	char *nw_argv[100];
	for(int i = 0; i < argc; i++){
		nw_argv[i] = (char *)malloc(100);
		strcpy(nw_argv[i], argv[i]);
	}
	while (fgets(line, MAX_LINE_LENGTH, file) != NULL){
		sscanf(line, "%s", word);
		if (strcmp(word, argv[1]) != 0) continue;
		int t = 1;
		int sz = strlen(word);
		while (iswhitespace(line[sz])) sz++;
		while (sscanf(line + sz, "%s", word) != EOF){
			nw_argv[t] = (char *)malloc(100);
			strcpy(nw_argv[t], word);
			sz += strlen(word);
			while (iswhitespace(line[sz])) sz++;
			t++;   
		}
		argc = t;
		break;
	}
	fclose(file);
	if (strcmp(nw_argv[1], "config") == 0){
		return config(argc, nw_argv);
	}
	if (strcmp(nw_argv[1], "init") == 0){
		return run_init(argc, nw_argv);
	}
	if (strcmp(nw_argv[1], "add") == 0) {
		return run_add(argc, nw_argv);
	}
	if (strcmp(argv[1], "reset") == 0) {
		return run_reset(argc, nw_argv);
	}
	if (strcmp(nw_argv[1], "status") == 0) {
		return run_status();
	}
	if (strcmp(nw_argv[1], "commit") == 0) {
		return run_commit(argc, nw_argv);
	}
	if (strcmp(nw_argv[1], "log") == 0){
		return run_log(argc, nw_argv);
	}
	if (strcmp (nw_argv[1], "branch") == 0){
		return run_branch(argc, nw_argv);
	}
	if (strcmp (nw_argv[1], "checkout") == 0){
		return run_checkout(argc, nw_argv);
	}
	// phase 2
	if (strcmp (nw_argv[1], "revert") == 0){
		return run_revert(argc, nw_argv);
	}
	if (strcmp (nw_argv[1], "tag") == 0){
		return run_tag(argc, nw_argv);
	}
	if (strcmp (nw_argv[1], "diff") == 0){
		return run_diff(argc, nw_argv);
	}
	if (strcmp (nw_argv[1], "merge") == 0){
		return run_merge(argc, nw_argv);
	}
	if (strcmp (nw_argv[1], "grep") == 0){
		return run_grep(argc, nw_argv);
	}
	return 0;
}