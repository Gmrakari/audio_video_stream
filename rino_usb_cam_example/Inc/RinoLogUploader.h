#ifndef RINO_LOG_UPLOADER_H
#define RINO_LOG_UPLOADER_H

// 初始化日志上传配置

//handlingMethod   1: 清空日志 2: 删除日志 3: 不处理
void rino_init_log_upload_config(const char *logFilePath, int handlingMethod);

// 根据提供的参数处理日志上传任务
void rino_handleLogUpload(int enable, int type, int intervalTime);

#endif // LOG_UPLOADER_H
