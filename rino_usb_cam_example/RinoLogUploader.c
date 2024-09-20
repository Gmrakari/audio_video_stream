#include "RinoLogUploader.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>

#include "RinoLink_API.h"
typedef struct {
    int enable;
    int type;
    int intervalTime;
    int running;
    int logHandlingMethod; // 1: 清空日志 2: 删除日志 3: 不处理
    char logFilePath[256];
    pthread_t upload_thread_id;
} Rino_UP_LogConfig;

static Rino_UP_LogConfig g_log_config;

static pthread_mutex_t config_mutex = PTHREAD_MUTEX_INITIALIZER;


// 假设的上传日志函数
static void upload_log() {
    
    struct stat st;
    if (stat(g_log_config.logFilePath, &st) != 0) {
        // 获取文件信息失败
        printf("Error getting file info for %s\n", g_log_config.logFilePath);
        return;
    }
    
    // 判断文件大小是否为0
    if (st.st_size == 0) {
        printf("Log file is empty, skipping upload\n");
        return;
    } 
    pthread_mutex_lock(&config_mutex);
    int ret = Rinolink_log_file_upload(g_log_config.logFilePath);
   

    if(ret !=0){
        printf("upload Rinolink_log_file_upload  failed\n");
        pthread_mutex_unlock(&config_mutex);
        return;
    }
    switch (g_log_config.logHandlingMethod) {
        case 1: // 清空日志
            {
                FILE* f = fopen(g_log_config.logFilePath, "w");
                if (f) {
                    fclose(f);
                }
            }
            break;
        case 2: // 删除日志
            remove(g_log_config.logFilePath);
            break;
        case 3: // 不处理
        default:
            break;
    }
    pthread_mutex_unlock(&config_mutex);
}



static void* log_upload_thread_func(void *arg) {
    Rino_UP_LogConfig *config = (Rino_UP_LogConfig *)arg;

    if (config->type == 1) {
        g_log_config.running = 0;
        upload_log();
    } else if (config->type == 2) {
        while (config->running) {
            upload_log();
            sleep(config->intervalTime * 60); // intervalTime is in minutes
        }
    }
    return NULL;
}


// handlingMethod  1: 清空日志 2: 删除日志 3: 不处理
void rino_init_log_upload_config(const char *logFilePath, int handlingMethod) {
    g_log_config.enable = 0;
    g_log_config.type = 0;
    g_log_config.intervalTime = 0;
    g_log_config.running = 0;
    g_log_config.logHandlingMethod = handlingMethod;
    strncpy(g_log_config.logFilePath, logFilePath, sizeof(g_log_config.logFilePath) - 1);
    g_log_config.logFilePath[sizeof(g_log_config.logFilePath) - 1] = '\0'; // Null terminate in case of overflow
}



void rino_handleLogUpload(int enable, int type, int intervalTime) {
    g_log_config.enable = enable;
    g_log_config.type = type;
    g_log_config.intervalTime = intervalTime; 
 

    if (g_log_config.enable == 1 && !g_log_config.running) {
        g_log_config.running = 1;
        pthread_t upload_thread;
        pthread_create(&upload_thread, NULL, log_upload_thread_func, &g_log_config);
    } else {
        g_log_config.running = 0;
    }
}
