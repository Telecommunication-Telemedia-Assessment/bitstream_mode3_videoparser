#define    IAD_PROBE_VERSION        0.736
#define    IAD_PROBE_BUILD          17.20
#define    IAD_PROBE_DATE           14.11
#define    IAD_PROBE_YEAR            2018


extern "C"
  {
  #include "../ffmpeg/libavutil/imgutils.h"
  #include "../ffmpeg/libavutil/samplefmt.h"
  #include "../ffmpeg/libavutil/timestamp.h"
  #include "../ffmpeg/libavutil/log.h"
  #include "../ffmpeg/libavcodec/avcodec.h"
  #include "../ffmpeg/libavformat/avformat.h"
  #include "../ffmpeg/libavcodec/avcodec.h"
  #include "VideoStat.h"
}

enum 
  {
  API_MODE_OLD                  = 0, // old method, deprecated 
  API_MODE_NEW_API_REF_COUNT    = 1, // new method, using the frame reference counting 
  API_MODE_NEW_API_NO_REF_COUNT = 2, // new method, without reference counting 
  } ;


class VIDEOPARSER_API  CVideoParser
  {
    AVFormatContext*     fmt_ctx;
    AVCodecContext       *CtxV, *CtxA ;
    AVStream             *video_stream, *audio_stream;
    enum AVPixelFormat   pix_fmt;
    AVFrame*             frame ;
    AVPacket             pkt;
    int64_t              TS0 ;
    int                  video_dst_bufsize ;

    BYTE*                video_dst_data[4] ;
    int                  video_dst_linesize[4];
    int                  video_stream_idx , audio_stream_idx ;
    int                  video_frame_count, audio_frame_count ;
    int                  EoS, EoP ;
    int                  refcount ;
    int                  InitialWidth, InitialHeight, InitialFormat ;

    int                  OpenCodecContext(int *stream_idx, AVFormatContext *fmt_ctx, enum AVMediaType type, char* SrcName);
    int                  AnalysePacket( AVCodecContext *CtxV, AVFrame* frame, VIDEO_STAT* FrameStat ) ;
    int                  decode_packet( int *got_frame ) ;


  public:
    void                 SumUp_FrameStat( VIDEO_STAT* SeqStat, VIDEO_STAT* FrmStat ) ;

    CVideoParser()  ;
    VIDEO_STAT    FrameStat;
    int           GOB_Cnt, PicCnt ;
    int           width, height;
    int           OpenVideo(char* FileName) ;
    int           ReadNextFrame() ;
    void          CloseParser() ;
  } ;
