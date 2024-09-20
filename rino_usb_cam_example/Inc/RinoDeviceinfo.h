#ifndef RINO_DEVICEINFO_H
#define RINO_DEVICEINFO_H

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define MAX_LINE_LENGTH 128 
#define MAX_KEY_VALUE_PAIRS 128


#define DEVICEINFO_FILE "./deviceinfo"  // 默认配置文件

typedef struct {
    char key[MAX_LINE_LENGTH];
    char value[MAX_LINE_LENGTH];
} KeyValuePair;


// 获取 deviceinfo 文件中的所有键值对
int getAllKeyValuePairs(KeyValuePair *pairs, int *count);

// 打印 deviceinfo 文件中的所有键值对
void printAllKeyValuePairs(void);


// 去除字符串前后的空格
char *str_trim(char *str);

// 通过 key 读取 deviceinfo 文件中的值
int readDeviceinfoValueByKey(const char *key, char *value,size_t value_size);

// 将整数值写入 deviceinfo 文件的特定 key
int writeDeviceinfoKeyIntValue(const char *key, int value);

// 将字符串值写入 deviceinfo 文件的特定 key
int writeDeviceinfoKeyValue(const char *key, const char *value);

// 读取 deviceinfo 文件中的特定键值，如 pid, uuid, secret, mac, storagePath
int readdeviceinfo(char *pid, char *uuid, char *secret, char *mac, char *storagePath);

#endif // RINO_DEVICEINFO_H
