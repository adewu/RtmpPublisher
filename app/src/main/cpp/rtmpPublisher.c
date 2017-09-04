#include <stdio.h>
#include <jni.h>
#include <time.h>
#include <assert.h>
#include <android/log.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavformat/avio.h>
#include <libavfilter/avfilter.h>
#include <libswscale/swscale.h>
#include <libavutil/log.h>
#include <libavutil/error.h>
#include <libavutil/time.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>


#define LOGE(...)  __android_log_print(ANDROID_LOG_ERROR, "(-_-!)","%s", ##__VA_ARGS__)
#define LOGI(...)  __android_log_print(ANDROID_LOG_INFO,  "(^_^)","%s", ##__VA_ARGS__)


AVFormatContext *g_fmt_ctx;
AVStream *g_stream;//视音频流对应的结构体，用于视音频编解码。
AVCodecContext *g_codec_ctx;
AVCodec *g_codec;
AVPacket g_packet; // 存储压缩数据（视频对应H.264等码流数据，音频对应AAC/MP3等码流数据）
AVFrame *g_frame; // 存储非压缩的数据（视频对应RGB/YUV像素数据，音频对应PCM采样数据）

int g_frame_count = 0;
int g_yuv_width;
int g_yuv_height;
int g_y_length;
int g_uv_length;
int64_t g_start_time;


//Output FFmpeg's av_log()
void custom_log(void *ptr, int level, const char *fmt, va_list vl) {
    FILE *fp = fopen("/storage/emulated/0/av_log.txt", "a+");
    if (fp) {
        vfprintf(fp, fmt, vl);
        fflush(fp);
        fclose(fp);
    }
}

void print_error(int err) {
    char errbuf[128];
    const char *errbuf_ptr = errbuf;
    if (av_strerror(err, errbuf, sizeof(errbuf)) < 0)
        errbuf_ptr = strerror(AVUNERROR(err));
//    av_log(NULL, AV_LOG_ERROR, "%s: %s\n", "hehe", errbuf_ptr);
    LOGE(errbuf_ptr);
}


JNIEXPORT jint JNICALL
Java_github_adewu_rtmppublisher_widgets_PreviewSurfaceView_initFFmpeg(JNIEnv *env, jobject thiz,
                                                                      jint width, jint height,
                                                                      jstring url) {

    int ret;

    const char *PATH = (*env)->GetStringUTFChars(env, url, 0);

    g_yuv_width = width;
    g_yuv_height = height;
    g_y_length = width * height;
    g_uv_length = width * height / 4;

    av_log_set_callback(custom_log);

    av_register_all();

    ret = avformat_network_init();

    // output initialize

    ret = avformat_alloc_output_context2(&g_fmt_ctx, NULL, "flv", PATH);

    // output encoder initialize

    g_codec = avcodec_find_encoder(AV_CODEC_ID_H264);

    if (!g_codec) {
        LOGE("can not find encoder!\n");
        return -1;
    }

    g_codec_ctx = avcodec_alloc_context3(g_codec);
    g_codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
    g_codec_ctx->width = width;
    g_codec_ctx->height = width;
    g_codec_ctx->time_base.num = 1;
    g_codec_ctx->time_base.den = 25;
    g_codec_ctx->bit_rate = 400000;
    g_codec_ctx->gop_size = 250;

    // some formats want stream headers to be separate

    if (g_fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
        g_codec_ctx->flags |= CODEC_FLAG_GLOBAL_HEADER;
    g_codec_ctx->qmin = 10;
    g_codec_ctx->qmax = 50;
    g_codec_ctx->max_b_frames = 1;

    // set h264 preset and tune
    AVDictionary *param = 0;
    av_opt_set(g_codec_ctx->priv_data, "preset", "superfast", 0);
    av_opt_set(g_codec_ctx->priv_data, "tune", "zerolatency", 0);

    //open encoder
    if (avcodec_open2(g_codec_ctx, g_codec, &param) < 0) {
        LOGE("Fail to open encoder!");
        return -1;
    }

    //add a new stream to output,should be called by the user before avformat_write_header() for muxing
    g_stream = avformat_new_stream(g_fmt_ctx, g_codec);
    if (g_stream == NULL) {
        return -1;
    }
    g_stream->time_base.num = 1;
    g_stream->time_base.den = 25;
    g_stream->codec = g_codec_ctx;

    //open output url ,set before avformat_write_header() for muxing
    ret = avio_open(&g_fmt_ctx->pb, PATH, AVIO_FLAG_READ_WRITE);
    if (ret < 0) {
        LOGE("Failed to open ouput url!");
        print_error(ret);
        return -1;
    }

    //write file header
    ret = avformat_write_header(g_fmt_ctx, NULL);

    g_start_time = av_gettime();

    LOGI("FFMpeg initial success!");
    return 0;
}

JNIEXPORT jint JNICALL
Java_github_adewu_rtmppublisher_widgets_PreviewSurfaceView_encodeVideo(JNIEnv *env, jobject obj,
                                                                  jbyteArray yuv) {

    if(yuv == NULL)
        return 0;

    int ret;
    int enc_got_frame = 0;

    //allocate memory for frame
    g_frame = av_frame_alloc();

    uint8_t *out_buffer = (uint8_t *) av_malloc(
            av_image_get_buffer_size(AV_PIX_FMT_YUV420P, g_codec_ctx->width, g_codec_ctx->height,
                                     1));
    av_image_fill_arrays(g_frame->data, g_frame->linesize, out_buffer, AV_PIX_FMT_YUV420P,
                         g_codec_ctx->width, g_codec_ctx->height, 1);

    //安卓摄像头数据为NV21格式，此处将其转换为YUV420P格式
    jbyte *in = (jbyte *) (*env)->GetByteArrayElements(env, yuv, 0);

    if (in == NULL) {
        LOGE("get nv21 data failed!");
        return -1;
    }

    memcpy(g_frame->data[0], in, g_y_length);
    for (int i = 0; i < g_uv_length; i++) {
        *(g_frame->data[2] + i) = *(in + g_y_length + i * 2); // address + width * height + i *2
        *(g_frame->data[1] + i) = *(in + g_y_length + i * 2 + 1);
    }

    g_frame->format = AV_PIX_FMT_YUV420P;
    g_frame->width = g_yuv_width;
    g_frame->height = g_yuv_height;

    g_packet.data = NULL;
    g_packet.size = 0;
    // 定义AVPacket对象后,请使用av_init_packet进行初始化
    av_init_packet(&g_packet);

    /** 编码一帧视频数据
    * int avcodec_encode_video2(AVCodecContext *avctx, AVPacket *avpkt,
    const AVFrame *frame, int *got_packet_ptr);

    该函数每个参数的含义在注释里面已经写的很清楚了，在这里用中文简述一下：
    avctx：编码器的AVCodecContext。
    avpkt：编码输出的AVPacket。
    frame：编码输入的AVFrame。
    got_packet_ptr：成功编码一个AVPacket的时候设置为1。
    函数返回0代表编码成功。*/
    ret = avcodec_encode_video2(g_codec_ctx, &g_packet, g_frame, &enc_got_frame);

    if (ret < 0) {
        LOGE("Failed to encode !");
        print_error(ret);
    }
    av_frame_free(&g_frame);

    if (enc_got_frame == 1) {
        LOGI("Succeed to encode frame: %d\t,size:%d\n", g_frame_count, g_packet.size);
        g_frame_count++;
        //标识该AVPacket所属的视频/音频流。
        g_packet.stream_index = g_stream->index;

        //Write PTS
        AVRational time_base = g_fmt_ctx->streams[0]->time_base;//{1,1000}
        AVRational r_framerate1 = {60, 2}; //{50,2}
        AVRational time_base_q = {1, AV_TIME_BASE};
        //duration between 2 frames(us)
        int64_t calc_duration = (double) (AV_TIME_BASE) * (1 / av_q2d(r_framerate1));

        //parameters
        g_packet.pts = av_rescale_q(g_frame_count * calc_duration, time_base_q,
                                    time_base); //Rescale a 64-bit integer by 2 rational numbers.
        g_packet.dts = g_packet.pts;
        g_packet.duration = av_rescale_q(calc_duration, time_base_q, time_base);
        g_packet.pos = -1;

        //delay
        int64_t pts_time = av_rescale_q(g_packet.dts, time_base, time_base_q);
        int64_t now_time = av_gettime() - g_start_time;
        if (pts_time > now_time) {
            av_usleep(pts_time - now_time);
        }

        ret = av_interleaved_write_frame(g_fmt_ctx, &g_packet);
    }

    av_packet_unref(&g_packet);
    return 0;
}

JNIEXPORT jint JNICALL
Java_github_adewu_rtmppublisher_widgets_PreviewSurfaceView_flush(JNIEnv *env, jobject obj) {
    int ret;
    int got_frame;
    AVPacket enc_packet;
    if (!(g_fmt_ctx->streams[0]->codec->codec->capabilities & CODEC_CAP_DELAY)) {
        return 0;
    }
    while (1) {
        enc_packet.data = NULL;
        enc_packet.size = 0;
        av_init_packet(&enc_packet);
        ret = avcodec_encode_video2(g_fmt_ctx->streams[0]->codec, &enc_packet, NULL, &got_frame);
        if (ret < 0)
            break;
        if (!got_frame) {
            ret = 0;
            break;
        }
        LOGI("flush encoder:succeed to encode 1 frame!\tsize:%5d\n", enc_packet.size);
        //write pts
        AVRational time_base = g_fmt_ctx->streams[0]->time_base; //{1,1000}
        AVRational r_framerate1 = {60, 2};
        AVRational time_base_q = {1, AV_TIME_BASE};

        //duration between 2 frames(us)
        int64_t calc_duration = (double) (AV_TIME_BASE) * (1 / av_q2d(r_framerate1));

        //parameters
        enc_packet.pts = av_rescale_q(g_frame_count * calc_duration, time_base_q, time_base);
        enc_packet.dts = enc_packet.pts;
        enc_packet.duration = av_rescale_q(calc_duration, time_base_q, time_base);

        //convert PTS/DTS
        enc_packet.pos = -1;
        g_frame_count++;
        g_fmt_ctx->duration = enc_packet.duration * g_frame_count;

        //mux encoded frame
        ret = av_interleaved_write_frame(g_fmt_ctx, &enc_packet);
        if (ret < 0)
            break;
    }
    //write file trailer
    av_write_trailer(g_fmt_ctx);
    return 0;
}

JNIEXPORT jint JNICALL
Java_github_adewu_rtmppublisher_widgets_PreviewSurfaceView_close(JNIEnv *env, jobject obj) {
    if (g_stream) {
        avcodec_close(g_stream->codec);
    }
    avio_close(g_fmt_ctx->pb);
    avformat_free_context(g_fmt_ctx);
    return 0;
}
