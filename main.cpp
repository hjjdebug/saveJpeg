extern "C"
{
#include "libavformat/avformat.h"
}

int savePicture(AVFrame *pFrame, char *out_name) ;

int main(int argc, char *argv[]) {//解码视频
    if (argc <= 2) {
        printf("Usage: %s <input file> <output file>\n", argv[0]);
		printf("Example: %s  /opt/test/test1.ts /tmp/test\n",argv[0]);
		printf("this will open /opt/test/test1.ts, and save every frame\n");
		printf("to a jpg file in /tmp/test directory.\n");
        exit(0);
    }

    const char *in_filename = argv[1];
    const char *out_filename = argv[2];
 
    // 1 open_input
    AVFormatContext *fmt_ctx = NULL;
    if (avformat_open_input(&fmt_ctx, in_filename, NULL, NULL) < 0) {
        printf("Could not open source file %s\n", in_filename);
        exit(1);
    }
    
    if (avformat_find_stream_info(fmt_ctx, NULL) < 0) {
        printf("Could not find stream information\n");
        exit(1);
    }
 
    av_dump_format(fmt_ctx, 0, in_filename, 0);
 
    AVPacket avpkt;
//    av_init_packet(&avpkt);
    avpkt.data = NULL;
    avpkt.size = 0;
 
     // 2 find_stream
    int v_stream_index = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if (v_stream_index < 0) {
        fprintf(stderr, "Could not find %s stream in input file '%s'\n",
                av_get_media_type_string(AVMEDIA_TYPE_VIDEO), in_filename);
        return -1;
    }
 
    AVStream *stream = fmt_ctx->streams[v_stream_index];
 
    // 3 find_decoder
    AVCodec *codec = avcodec_find_decoder(stream->codecpar->codec_id);
    if (codec == NULL) {
        return -1;
    }
 
    // 4 alloc codec_ctx
    AVCodecContext *codeCtx = avcodec_alloc_context3(NULL);
    if (!codeCtx) {
        fprintf(stderr, "Could not allocate video codec context\n");
        exit(1);
    }
 
 
    // 5 fill codec_ctx param
    int ret;
    if ((ret = avcodec_parameters_to_context(codeCtx, stream->codecpar)) < 0) {
        fprintf(stderr, "Failed to copy %s codec parameters to decoder context\n",
                av_get_media_type_string(AVMEDIA_TYPE_VIDEO));
        return ret;
    }
 
    // 6 open_codec_ctx
    avcodec_open2(codeCtx, codec, NULL);
 
 
    //初始化frame，解码后数据
    AVFrame *frame = av_frame_alloc();
    if (!frame) {
        fprintf(stderr, "Could not allocate video frame\n");
        exit(1);
    }
 
    int frame_count = 0;
    char buf[1024];
    // 7
    while (av_read_frame(fmt_ctx, &avpkt) >= 0) {
        if (avpkt.stream_index == v_stream_index) {
            // 8
            int re = avcodec_send_packet(codeCtx, &avpkt);
            if (re < 0) {
                continue;
            }
            
            // 9 这里必须用while()，逻辑会比较严谨.
			while(1)
			{
				int ret=avcodec_receive_frame(codeCtx, frame); 
				if(ret == 0) 
				{
					// 拼接图片路径、名称
					snprintf(buf, sizeof(buf), "%s/picture-%d.jpg", out_filename, frame_count);
					savePicture(frame, buf); //保存为jpg图片
				}
				else if(ret==-EAGAIN) break;
				else
				{
					printf("error receive frame:%d\n",ret);
					exit(1);
				}
			}
 
            frame_count++;
        }
        av_packet_unref(&avpkt);
    }
 
	avformat_close_input(&fmt_ctx);
}
int savePicture(AVFrame *pFrame, char *out_name) 
{
	//编码保存图片
    int width = pFrame->width;
    int height = pFrame->height;
    AVCodecContext *pCodeCtx = NULL;
    
    
    AVFormatContext *pFormatCtx = avformat_alloc_context();
    // 设置输出文件格式
    pFormatCtx->oformat = av_guess_format("mjpeg", NULL, NULL);
 
    // 创建并初始化输出AVIOContext
    if (avio_open(&pFormatCtx->pb, out_name, AVIO_FLAG_READ_WRITE) < 0) {
        printf("Couldn't open output file.\n");
        return -1;
    }
 
    // 构建一个新stream, 仅为拿到它的stream-codec-parameter
    AVStream *pAVStream = avformat_new_stream(pFormatCtx, 0);
    if (pAVStream == NULL) {
        return -1;
    }
	
	//修改avstream的codecpar
    AVCodecParameters *parameters = pAVStream->codecpar;
    parameters->codec_id = pFormatCtx->oformat->video_codec;
    parameters->codec_type = AVMEDIA_TYPE_VIDEO;
    parameters->format = AV_PIX_FMT_YUVJ420P;
    parameters->width = pFrame->width;
    parameters->height = pFrame->height;
 
	//查找codec及分配codec_ctx
    AVCodec *pCodec = avcodec_find_encoder(pAVStream->codecpar->codec_id);
 
    if (!pCodec) {
        printf("Could not find encoder\n");
        return -1;
    }
 
    pCodeCtx = avcodec_alloc_context3(pCodec);
    if (!pCodeCtx) {
        fprintf(stderr, "Could not allocate video codec context\n");
        exit(1);
    }
 
    if ((avcodec_parameters_to_context(pCodeCtx, pAVStream->codecpar)) < 0) {
        fprintf(stderr, "Failed to copy %s codec parameters to decoder context\n",
                av_get_media_type_string(AVMEDIA_TYPE_VIDEO));
        return -1;
    }
 
    pCodeCtx->time_base = (AVRational) {1, 25};
 
	//打开codec_ctx
    if (avcodec_open2(pCodeCtx, pCodec, NULL) < 0) {
        printf("Could not open codec.");
        return -1;
    }
 
	//书写文件头
    int ret = avformat_write_header(pFormatCtx, NULL);
    if (ret < 0) {
        printf("write_header fail\n");
        return -1;
    }
 
    int y_size = width * height;
 
    //Encode
    // 给AVPacket分配足够大的空间
    AVPacket pkt;
    av_new_packet(&pkt, y_size * 3);
 
    // 编码数据
    ret = avcodec_send_frame(pCodeCtx, pFrame);
    if (ret < 0) {
        printf("Could not avcodec_send_frame.");
        return -1;
    }
 
    // 得到编码后数据
    ret = avcodec_receive_packet(pCodeCtx, &pkt);
    if (ret < 0) {
        printf("Could not avcodec_receive_packet");
        return -1;
    }
 
	//书写frame
    ret = av_write_frame(pFormatCtx, &pkt);
 
    if (ret < 0) {
        printf("Could not av_write_frame");
        return -1;
    }
 
    av_packet_unref(&pkt);
 
    //Write Trailer
    av_write_trailer(pFormatCtx);
 
 
    avio_close(pFormatCtx->pb);
    avcodec_close(pCodeCtx);
    avformat_free_context(pFormatCtx);
 
    return 0;
}
