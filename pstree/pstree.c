#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <getopt.h>
#include <stdint.h>
#include <dirent.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

// 最多容许打印1024个进程
#define PROC_SIZE  1 << 16 
#define BUFFER_SIZE 256 

uint32_t proc_count = 0;

struct proc {
    char name[BUFFER_SIZE];          // 进程名称
    uint32_t pid;                    // 进程 ID
    uint32_t ppid;                   // 父进程 ID
} proc_list[PROC_SIZE];

bool p_flag = false, n_flag = false, V_flag = false;
bool mark[PROC_SIZE] = {false};

void process_args(int argc, char *argv[]);
int is_digit_string(char *s);
int compare(const void *a, const void *b); 
void get_status_info(char *proc_path);
void create_proc_list(void);
void print_tree(uint32_t ppid, int indent);

int main(int argc, char *argv[]) {
    process_args(argc, argv);

    // 打印版本信息
    if (V_flag == true) {
        printf("pstree 0.1\n");
        printf("Copyright (C) 2024-current yultang\n");
        printf("This is free software, and you are welcome to redistribute it under\n");
        printf("the terms of the GNU General Public License.\n");
        exit(EXIT_SUCCESS);
    }

    create_proc_list();
    print_tree(0, 0);

    return 0;
}

void process_args(int argc, char *argv[]) {
    int opt;
    int long_index = 0;

    static struct option long_options[] = {
        {"show-pids", no_argument, NULL, 'p'},      // 显示进程号
        {"numeric-sort", no_argument, NULL, 'n'},   // 按照 PID 数值从小到大排序
        {"version", no_argument, NULL, 'V'},        // 打印版本信息
        {0, 0, 0, 0},
    };

    while ((opt = getopt_long(argc, argv, "pnV", long_options, &long_index)) != -1) {
        switch (opt) {
            case 'p':
                p_flag = true;
                break;
            case 'n':
                n_flag = true;
                break;
            case 'V':
                V_flag = true;
                break;
            default: 
                break;
        }
    }
    return;
}

int is_digit_string(char *s) {
    if (s == NULL || *s == '\0')
        return 0;

    for (int i = 0; i < strlen(s); i++) {
        // 存在非数字字符则返回 0
        if (!isdigit(s[i]))
            return 0;
    }
    return 1;
}

int compare(const void *a, const void *b) {
    struct proc *procA = (struct proc *)a;
    struct proc *procB = (struct proc *)b;
    return (procA->pid - procB->pid);
}

void get_status_info(char *proc_path) {
    char Name[BUFFER_SIZE];
    int Pid = 0, PPid = 0;

    uint32_t length = snprintf(NULL, 0, "%s/status", proc_path);
    char *status_path = malloc(sizeof(char) * (length + 1));
    snprintf(status_path, length + 1, "%s/status", proc_path); 

    FILE *file = fopen(status_path, "r");
    if (file == NULL) {
        printf("open status file failed: %s\n", status_path);
        exit(EXIT_FAILURE);
    }

    char buffer[BUFFER_SIZE];
    int cnt = 0;
    while (fgets(buffer, sizeof(buffer), file) != NULL) {
        if (cnt == 3)
            break;

        // 解析 Name
        if (strncmp(buffer, "Name:", 5) == 0) {
            sscanf(buffer + 6, "%s", Name);
            cnt++;
        }

        // 解析 Pid
        else if (strncmp(buffer, "Pid:", 4) == 0) {
            sscanf(buffer + 5, "%d", &Pid);
            cnt++;
        }

        // 解析 PPid
        else if (strncmp(buffer, "PPid:", 5) == 0) {
            sscanf(buffer + 6, "%d", &PPid);
            cnt++;
        }
    }

    // 将信息存储到 proc_list 中
    strcpy(proc_list[proc_count].name, Name);
    proc_list[proc_count].pid = Pid;
    proc_list[proc_count].ppid = PPid;
    
    fclose(file);
    free(status_path);
}

void create_proc_list(void) {
    const char *dir_path = "/proc";
    struct dirent *entry;

    // 读取 /proc 目录
    DIR *dir = opendir(dir_path);
    if (dir == NULL) {
        printf("open /proc directory failed\n");
        exit(EXIT_FAILURE);
    }

    while ((entry = readdir(dir)) != NULL) {
        // 如果是一个以数字命名的目录，它就是一个进程
        if (is_digit_string(entry->d_name) && entry->d_type == DT_DIR) {
            proc_count++;

            uint32_t proc_len = snprintf(NULL, 0, "%s/%s", dir_path, entry->d_name);
            char *proc_path = malloc(sizeof(char) * (proc_len + 1));
            snprintf(proc_path, proc_len + 1, "%s/%s", dir_path, entry->d_name);

            // 读取进程的 status 文件
            get_status_info(proc_path);

            free(proc_path);
        }
    }

    if (n_flag == true) {
        qsort(proc_list, proc_count, sizeof(struct proc), compare);
    }

    closedir(dir);
}

void print_tree(uint32_t ppid, int indent) {
    for (int i = 0; i < proc_count; i++) {
        if (mark[i])
            continue;

        if (proc_list[i].ppid == ppid) {
            for (int j = 0; j < indent; j++)
                printf("   ");
            printf("%s", proc_list[i].name);
            if (p_flag)
                printf(" (pid: %d)", proc_list[i].pid);
            printf("\n");

            mark[i] = true;
            print_tree(proc_list[i].pid, indent + 1);
        }
    }
}
