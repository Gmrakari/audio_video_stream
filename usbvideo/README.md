

1. 
安装库

sudo apt-get install libavcodec-dev libavformat-dev libavutil-dev libswscale-dev

FFmpeg(libavdevice, libavformat, libavcodec, libavutil, libswscale)

usb camera 转 h264流程
1. 打开设备
  a. 注册所有的USB摄像头设备 
  b. 对USB设备进行参数配置(分辨率、帧率、像素格式)
  c. 通过avformat_open_input打开设备，返回AVFormatContext指针 
2.配置编码器
  a. open_encoder
  	i). 找到H.264便阿码器并分配编码上下文
  	ii). 设置编码参数，分辨率，时间基，帧率，gop大小，最大b帧数和比特率
  	iii). 打开编码准备编码
3. 创建视频帧
	create_frame 创建视频帧，分配内存并设置帧的参数
4. 编码视频帧
	encode 将帧发送给编码器并处理编码后的数据
	  a. 发送帧并接收编码后的包
	  b. 将编码后的数据写入到输出文件中

读取yuyv422 -> yuv420p


备注:
1. h264编码器支持yuv420p,需要将yuv422转为yuv420p
2. 