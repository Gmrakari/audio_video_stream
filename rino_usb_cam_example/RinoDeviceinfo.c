
 
#include <ctype.h> 
#include <string.h>  
#include <stdio.h> 
#include "RinoDeviceinfo.h"


int getAllKeyValuePairs(KeyValuePair *pairs, int *count) {
    FILE *pf = fopen(DEVICEINFO_FILE, "r");
    if (pf == NULL) return -1; // 文件打开失败

    char line[MAX_LINE_LENGTH];
    int pairCount = 0;
    
    while (fgets(line, sizeof(line), pf) && pairCount < MAX_KEY_VALUE_PAIRS) {
        char *pos = strchr(line, '=');
        if (pos != NULL && pos != line && *(pos + 1) != '\0') {
            *pos = '\0'; // 替换等号以分割 key 和 value
            strncpy(pairs[pairCount].key, str_trim(line), MAX_LINE_LENGTH - 1);
            strncpy(pairs[pairCount].value, str_trim(pos + 1), MAX_LINE_LENGTH - 1);
            pairs[pairCount].value[strcspn(pairs[pairCount].value, "\n")] = '\0'; // 移除换行符
            pairCount++;
        }
    }

    fclose(pf);

    *count = pairCount;
    return 0; // 成功读取所有键值
}

void printAllKeyValuePairs(void) {
    KeyValuePair pairs[MAX_KEY_VALUE_PAIRS];
    int count = 0;

    // 调用函数获取所有键值对
    int result = getAllKeyValuePairs(pairs, &count);

    if (result == 0) {
        // 打印标题和分隔线
        printf("All key-value pairs in deviceinfo file:\n");
        printf("--------------------------------------\n");

        // 打印所有键值对
        int i = 0;
        for (i = 0; i < count; i++) {
            printf("Key: %-20s | Value: %s\n", pairs[i].key, pairs[i].value);
        }

        // 打印底部分隔线
        printf("--------------------------------------\n");
    } else {
        // 处理错误
        printf("An error occurred while reading the deviceinfo file.\n");
    }
}


char *str_trim(char *str) {
    while (isspace((unsigned char)*str)) str++; // 跳过前导空格

    char *end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--; // 跳过尾随空格

    *(end + 1) = '\0'; // 截断尾随空格

    return str;
}

int readDeviceinfoValueByKey(const char *key, char *value,size_t value_size) { 
    FILE *pf = fopen(DEVICEINFO_FILE, "r");
    if (pf != NULL) {
        char line[MAX_LINE_LENGTH];
        while (fgets(line, sizeof(line), pf)) {
            char *pos = strchr(line, '=');
            if (pos != NULL && pos != line && *(pos + 1) != '\0' && *(pos + 1) != '\n') {
                *pos = '\0'; // 分割 key 和 value
                if (strcmp(line, key) == 0) {
                    strncpy(value, pos + 1, value_size - 1);
                    value[value_size - 1] = '\0'; // 确保字符串以空字符结尾
                     // 移除 value 中的换行符，如果存在
                    char *newline_pos = strchr(value, '\n');
                    if (newline_pos != NULL) {
                        *newline_pos = '\0';
                    } 
                    fclose(pf);
                    return 0; // 成功找到 key
                }
            }
        }
        fclose(pf);
    }
    return -1; // 文件打开失败或未找到 key
}
int writeDeviceinfoKeyIntValue(const char *key, int value) {
	char signal4GValueStr[10];
	snprintf(signal4GValueStr, sizeof(signal4GValueStr), "%d", value);
	return writeDeviceinfoKeyValue(key,signal4GValueStr); 
} 


int writeDeviceinfoKeyValue(const char *key, const char *value) {
    FILE *pf = fopen(DEVICEINFO_FILE, "r");
    if (pf == NULL) return -1; // 文件打开失败

    char lines[128][MAX_LINE_LENGTH]; // 假设文件最多包含128行，每行最多128字符
    int lineCount = 0;
    int keyFound = 0;

    // 读取整个文件到临时数组
    while (fgets(lines[lineCount], sizeof(lines[lineCount]), pf)) {
        char line[128];
        strncpy(line, lines[lineCount], 128);
        char *posEq = strchr(line, '=');
        if (posEq != NULL) {
            *posEq = '\0';
            char *lineKey = str_trim(line);
            if (strcmp(lineKey, key) == 0) {
                snprintf(lines[lineCount], sizeof(lines[lineCount]), "%s=%s\n", key, value);
                keyFound = 1;
            }
        }
        lineCount++;
    }

    fclose(pf);

    // 如果未找到 key，则在末尾添加新行
    if (!keyFound) {
        snprintf(lines[lineCount], sizeof(lines[lineCount]), "%s=%s\n", key, value);
        lineCount++;
    }

    // 将临时数组的内容写回文件
    pf = fopen(DEVICEINFO_FILE, "w");
    if (pf == NULL) return -1; // 文件打开失败
	int i = 0;
    for (i = 0; i < lineCount; i++) {
        fputs(lines[i], pf);
    }

    fclose(pf);
    return 0; // 成功写入或更新键值
}

int readdeviceinfo(char *pid, char *uuid, char *secret, char *mac, char *storagePath) {
    FILE *pf = fopen(DEVICEINFO_FILE, "r");
    if (pf == NULL) return -1; // 文件打开失败

    char line[MAX_LINE_LENGTH];
    while (fgets(line, sizeof(line), pf)) {
        char *pos = strchr(line, '=');
        if (pos != NULL && pos != line && *(pos + 1) != '\0') {
            *pos = '\0'; // 替换等号以分割 key 和 value
            char *key = str_trim(line);
            char *value = str_trim(pos + 1);
            value[strcspn(value, "\n")] = '\0'; // 移除换行符 
            if (strcmp(key, "pid") == 0) {
                strncpy(pid, value, 127);
            } else if (strcmp(key, "uuid") == 0) {
                strncpy(uuid, value, 127);
            } else if (strcmp(key, "secret") == 0) {
                strncpy(secret, value, 127);
            } else if (strcmp(key, "mac") == 0) {
                strncpy(mac, value, 127);
            } else if (strcmp(key, "storagePath") == 0) {
                strncpy(storagePath, value, 127);
            }   
        }
    }
    fclose(pf);
    return 0; // 成功读取所有键值
}

