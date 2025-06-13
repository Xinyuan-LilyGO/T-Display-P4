截取音频：
ffmpeg -i input.wav -ss 00:00:05 -t 00:00:10 output.wav

-ss为开始时间 -t为持续时间

查看音频格式：
ffprobe input.wav

修改音频文件采样率：
ffmpeg -i input.wav -ar 16000 output.wav

多通道变单通道
ffmpeg -i input.wav -ac 1 output.wav

转换格式
ffmpeg -i input.mp3 outpit.wav

提取一个通道并重采样
ffmpeg -i input.wav -ac 1 -ar 16000 output.wav

修改采样精度（位数）
ffmpeg -y -i input.wav -acodec pcm_f32le -ac 1 -ar 16000 -vn output.wav

把 mp3 转换成 pcm 的命令
ffmpeg -i input.mp3 -f s16le -acodec pcm_s16le output.pcm
