# saveJpeg
use ffmpeg to save every frame to a jpg file

通过ffmpeg 进行文件解码和编码的过程
1. 打开一个文件进行解码的过程:
	//初始化
	avformat_open_input()
	avformat_find_stream_info()
	av_find_best_stream()
	avcodec_find_decoder()
	avcodec_parameters_to_context()
	avcodec_open2()

	//循环执行
	av_read_frame()
	avcodec_send_packet()
	avcodec_receive_frame()

	//结尾
	avformat_close_input()

2. 打开一个文件进行编码的过程:
	//初始化
	av_guess_format()
	avio_open();
	avformat_new_stream()
	avcodec_find_encoder()
	avcodec_parameters_to_context()
	avcodec_open2()
	avformat_write_header()

	//循环执行,如果只是一张图片,就不用循环了.
	avcodec_send_frame()
	avcodec_receive_packet()
	//结尾
	av_write_trailer()

	其它是内存释放过程



测试程序的运行时间。
$ time ./test_savejpg /opt/test/test1.ts /tmp/test
Input #0, mpegts, from '/opt/test/test1.ts':
  Duration: 00:01:19.98, start: 1.458667, bitrate: 2295 kb/s
  Program 1 
    Metadata:
      service_name    : Service01
      service_provider: FFmpeg
  Stream #0:0[0x100]: Video: h264 (High) ([27][0][0][0] / 0x001B), yuv420p(progressive), 1920x1080 [SAR 1:1 DAR 16:9], 25 fps, 25 tbr, 90k tbn, 50 tbc
  Stream #0:1[0x101](eng): Audio: aac (LC) ([15][0][0][0] / 0x000F), 48000 Hz, stereo, fltp, 137 kb/s

real	0m42.492s
user	0m42.210s
sys	     0m0.276s
