#include "stdafx.h"
#include "VideoParser.h"


int  api_mode = API_MODE_NEW_API_REF_COUNT, Started=0, Opened=0 ;
int  CurrBlackBorder ;

extern "C"
  {
  int GetVideoStatisticsH264( AVCodecContext* CtxV, AVFrame* frame, VIDEO_STAT* FrameStat);
  int GetVideoStatisticsHEVC( AVCodecContext* CtxV, AVFrame* frame,  VIDEO_STAT* FrameStat);
  int GetVideoStatisticsVP9 ( AVCodecContext* CtxV, AVFrame* frame,  VIDEO_STAT* FrameStat);
  int  DebugCnt ;

  VIDEOPARSER_API int OpenVideo( char*  FileName, BYTE**  ParserStore )
    {
    CVideoParser*  Parser ;

    Parser = new(CVideoParser) ;
    *ParserStore = (BYTE*)Parser ;
    return(Parser->OpenVideo( FileName )) ;
    }

  /***************************************************************************************/

  VIDEOPARSER_API int ReadNextFrame( BYTE* ParserStore )
    {
    CVideoParser* Parser = (CVideoParser*)ParserStore ;
    return(Parser->ReadNextFrame()) ;
    }

  /***************************************************************************************/

   int   ForcedGOB[16] = { 0 } ;
   //int   ForcedGOB[16] = { 1, 251, 501 } ;            //  6-HD
 //int    ForcedGOB[16] = { 1,2, 69, 319, 569, 587 } ; //  4-HD
  // int   ForcedGOB[16] = { 1,2, 251, 501, 595 } ;     //  3-HD
   //int   ForcedGOB[16] = { 1, 251, 501 } ;            //  2-HD
  //int   ForcedGOB[16] = { 1, 2, 45, 295, 545, 597 } ;//  1-HD

  VIDEOPARSER_API int  GetFrameStatistics( BYTE* ParserStore, VIDEO_STAT* FrameStatGlobal, VIDEO_STAT SeqStat[][4] )
    {
    CVideoParser* Parser  = (CVideoParser*)ParserStore ;
    VIDEO_STAT*   FrmStat = &(Parser->FrameStat) ;
    VIDEO_STAT*   CurrSeq = SeqStat[Parser->GOB_Cnt] + FrmStat->FrameType ;
    int           i, FieldSize ;
    float*        TmpMotionField[2] = { 0 } ;

    memcpy( FrameStatGlobal, (void*)FrmStat, sizeof( VIDEO_STAT ) ) ;

    if( ((ForcedGOB[0] == 0) && (FrameStatGlobal->FrameType == 1)) || (Parser->PicCnt == ForcedGOB[Parser->GOB_Cnt+1]) )
      {
      if( Parser->GOB_Cnt >= 0 )
        for( int i = 0 ; i < 4 ; SeqStat[Parser->GOB_Cnt][i++].NumFrames = Parser->PicCnt - SeqStat[Parser->GOB_Cnt][i].FirstFrame ) ;
      Parser->GOB_Cnt       = min( 7, Parser->GOB_Cnt + 1 ) ;
      CurrSeq               = SeqStat[Parser->GOB_Cnt] + FrmStat->FrameType ;
      for( int i=0 ; i<4 ; SeqStat[Parser->GOB_Cnt][i++].FirstFrame   = Parser->PicCnt)  ;
      }

    SAFE_FREE(FrmStat->S.NormalizedField[0]);
    SAFE_FREE(FrmStat->S.NormalizedField[1]);
    memset( (void*)FrmStat, 0, sizeof( VIDEO_STAT ) ) ;
    Parser->SumUp_FrameStat( CurrSeq, FrameStatGlobal ) ;
    return( Parser->GOB_Cnt ) ;
    }

  /***************************************************************************************/

  VIDEOPARSER_API void GetSeqStatistic( BYTE* ParserStore, VIDEO_STAT* SeqStat )
    {
    int   i, j, k, l ;
    VIDEO_STAT*   CurrStat ;
    CVideoParser* Parser  = (CVideoParser*)ParserStore ;

    if( SeqStat[1].NumFrames == 0)
      for( i=0 ; i<4 ; SeqStat[i++].NumFrames = Parser->PicCnt - SeqStat[i].FirstFrame ) ;


    for( int i=1 ; i<4 ; i++ )
      {
      CurrStat = SeqStat + i ;
      if( CurrStat->FrameCnt )
        {
        CurrStat->Av_QP            /= CurrStat->FrameCnt ;
        CurrStat->Av_QPBB          /= CurrStat->FrameCnt ;
        CurrStat->Av_Motion        /= CurrStat->FrameCnt ;
        CurrStat->Av_MotionDif     /= CurrStat->FrameCnt ;
        CurrStat->StdDev_Motion    /= CurrStat->FrameCnt ;
        CurrStat->StdDev_MotionDif /= CurrStat->FrameCnt ;
        CurrStat->PredFrm_Intra    /= CurrStat->FrameCnt ;
        CurrStat->SKIP_mb_ratio    /= CurrStat->FrameCnt ;
        CurrStat->FrameSize        /= CurrStat->FrameCnt ;
        CurrStat->S.CodedMv        /= CurrStat->FrameCnt ;

        CurrStat->NumBlksIntra     /= CurrStat->FrameCnt ;
        CurrStat->NumBlksMerge     /= CurrStat->FrameCnt ;
        CurrStat->NumBlksMv        /= CurrStat->FrameCnt ;
        CurrStat->NumBlksSkip      /= CurrStat->FrameCnt ;
        CurrStat->BitCntCoefs      /= CurrStat->FrameCnt ;
        CurrStat->BitCntMotion     /= CurrStat->FrameCnt ;


        for( j=0 ; j<3 ; CurrStat->TemporalComplexety[j++]  /= CurrStat->FrameCnt ) ;
        for( j=0 ; j<3 ; CurrStat->SpatialComplexety [j++]  /= CurrStat->FrameCnt ) ;
        CurrStat->TI_Mot[0]  /= CurrStat->FrameCnt ;
        CurrStat->TI_Mot[1]  /= CurrStat->FrameCnt ;
        CurrStat->TI_910     /= CurrStat->FrameCnt ;
        CurrStat->SI_910     /= CurrStat->FrameCnt ;

        for( j=0 ; j<4 ; j++ )
          {
          for( l=0 ; l<3 ; l++ )
            {
            for( k=0 ; k<6 ; CurrStat->Blockiness [l]   [k++] /= CurrStat->FrameCnt ) ;
            for( k=0 ; k<3 ; CurrStat->HF_LF_ratio[l][j][k++] /= CurrStat->FrameCnt ) ;
            } ;

          for( k = 0 ; k < 32 ; k++ )
            for( l = 0 ; l < 32 ; CurrStat->Av_Coefs[j][k][l++] /= CurrStat->FrameCnt ) ;
          } ;

        for( j=0 ; j<7 ; j++ )
          {
          CurrStat->MbQPs   [j]    /= CurrStat->FrameCnt ;
          CurrStat->MbTypes [j]    /= CurrStat->FrameCnt ;
          CurrStat->TrShapes[j]    /= CurrStat->FrameCnt ;
          CurrStat->BlkShapes[j+2] /= CurrStat->FrameCnt ;
          } ;
        } ;
      } ;
    }

  }

/***************************************************************************************/
/***************************************************************************************/
/***************************************************************************************/

  FILE*  TempFile=NULL ;
  int    FrameCnt = 0 ;

void CVideoParser::SumUp_FrameStat( VIDEO_STAT* Seq_Stat, VIDEO_STAT* FrmStat )
  {
  int  i=0, j, k, T = FrmStat->FrameType ;

  //if( TempFile == NULL )
  //  TempFile = fopen( "C:\\seq\\VIR-Q\\QP\\Groundtruth_QP00.csv", "w" ) ;
  //if( TempFile )
  //  if( TempFile && FrmStat->FrameType == 2 )
  //fprintf( TempFile, "%6d; %7.2lf; %7.2lf; %7.2lf; %7.2lf; %7.2lf; %7d;  %7.2lf;  %7.2lf;  %7.2lf \n", ++FrameCnt, FrmStat->Av_Motion, FrmStat->Av_MotionDif, FrmStat->StdDev_Motion, FrmStat->StdDev_MotionDif, FrmStat->Av_QP, FrmStat->FrameSize, (FrmStat->FrameType != 1) ? FrmStat->S.BitCntMotion : 0, (FrmStat->FrameType != 1) ? FrmStat->S.BitCntCoefs : 0, FrmStat->SpatialComplexety[1] ) ;
  //  fprintf( TempFile, "%6d;  %1d; %7.2lf; %7.2lf; %7.2lf;  %7.2lf;  %7.2lf;  %7.2lf;  %7.2lf\n", ++FrameCnt, FrmStat->IsShrtFrame, FrmStat->Av_Motion, FrmStat->Av_MotionDif, FrmStat->Av_QP, (FrmStat->FrameType != 1) ? FrmStat->S.BitCntMotion : 0, (FrmStat->FrameType != 1) ? FrmStat->S.BitCntCoefs : 0, FrmStat->SpatialComplexety[0], FrmStat->SpatialComplexety[1] ) ;
  //fprintf( TempFile, "%6d;  %1d; %7.2lf;  %7.2lf;  %7.2lf\n", ++FrameCnt, FrmStat->IsShrtFrame, FrmStat->Av_Motion, FrmStat->Av_MotionDif, (FrmStat->FrameType != 1) ? FrmStat->TemporalComplexety[2] * 24 : 0.0 ) ;
  //

  Seq_Stat->Av_Motion            += FrmStat->Av_Motion ;
  Seq_Stat->Av_MotionDif         += FrmStat->Av_MotionDif ;
  Seq_Stat->StdDev_Motion        += FrmStat->StdDev_Motion ;
  Seq_Stat->StdDev_MotionDif     += FrmStat->StdDev_MotionDif ;

  for( j=0 ; j<3 ; Seq_Stat->TemporalComplexety[j++] += FrmStat->TemporalComplexety[j] ) ;
  for( j=0 ; j<3 ; Seq_Stat->SpatialComplexety [j++] += FrmStat->SpatialComplexety [j] ) ;
  Seq_Stat->TI_Mot[0]            += FrmStat->TI_Mot[0] ;
  Seq_Stat->TI_Mot[1]            += FrmStat->TI_Mot[1] ;
  Seq_Stat->TI_910               += FrmStat->TI_910 ;
  Seq_Stat->SI_910               += FrmStat->SI_910 ;
  Seq_Stat->Av_QP                += FrmStat->Av_QP ;
  Seq_Stat->Av_QPBB              += FrmStat->Av_QPBB ;
  Seq_Stat->PredFrm_Intra        += FrmStat->PredFrm_Intra ;
  Seq_Stat->SKIP_mb_ratio        += FrmStat->PredFrm_Intra ;
  Seq_Stat->FrameSize            += FrmStat->FrameSize ;
  Seq_Stat->BitCntCoefs          += FrmStat->BitCntCoefs ;
  Seq_Stat->BitCntMotion         += FrmStat->BitCntMotion ;
  Seq_Stat->S.CodedMv            += FrmStat->S.CodedMv ;
  Seq_Stat->NumBlksSkip          += FrmStat->NumBlksSkip ;
  Seq_Stat->NumBlksIntra         += FrmStat->NumBlksIntra ;
  Seq_Stat->NumBlksMv            += FrmStat->NumBlksMv ;
  Seq_Stat->NumBlksMerge         += FrmStat->NumBlksMerge ;
  Seq_Stat->FrameCnt++ ;

  for( i = 0 ; i < 7 ; i++ )
    {
    Seq_Stat->MbQPs   [i] += FrmStat->MbQPs [i] ;
    Seq_Stat->MbTypes [i] += FrmStat->MbTypes [i] ;
    Seq_Stat->TrShapes[i] += FrmStat->TrShapes[i] ;
    } ;
  for( i=0 ; i<9 ; i++ )
    Seq_Stat->BlkShapes[i] += FrmStat->BlkShapes[i] ;
  for( i=0 ; i<4 ; i++ )
    {
    for( k=0 ; k<3 ; k++ )
      {
      for( j=0 ; j<3 ; Seq_Stat->HF_LF_ratio[k][i][j++]  += FrmStat->HF_LF_ratio[k][i][j] ) ;
      for( j=0 ; j<6 ; Seq_Stat->Blockiness[k][j++]      += FrmStat->Blockiness[k][j] ) ;
      } ;
    for( j=0 ; j<32 ; j++ )
      for( k=0 ; k<32; Seq_Stat->Av_Coefs[i][j][k++] += FrmStat->Av_Coefs[i][j][k] ) ;
    } ;
  }

/***************************************************************************************/

CVideoParser::CVideoParser()
  {
  memset(this, 0, sizeof(CVideoParser) ) ;
  video_stream_idx = audio_stream_idx = -1 ;
  GOB_Cnt = -1 ;
  PicCnt  =  0 ;
  }

/**********************************************************************************/

int CVideoParser::OpenCodecContext(int *stream_idx, AVFormatContext *fmt_ctx, enum AVMediaType type, char* SrcName)
  {
  int               ret, stream_index;
  AVStream*         st ;
  AVCodecContext*   dec_ctx=NULL ;
  AVCodec*          dec=NULL ;
  AVDictionary*     opts=NULL ;

  if( (ret = av_find_best_stream(fmt_ctx, type, -1, -1, NULL, 0) ) < 0)
    {
    fprintf(stderr, "Could not find %s stream in input file '%s'\n", av_get_media_type_string(type), SrcName);
    return ret;
    }
  else
    {
    stream_index = ret;
    st           = fmt_ctx->streams[stream_index];
    dec_ctx      = st->codec;                                                                         // find decoder for the stream
    if( (dec     = avcodec_find_decoder(dec_ctx->codec_id)) == NULL)
      {
      fprintf(stderr, "Failed to find %s codec\n", av_get_media_type_string(type));
      return AVERROR(EINVAL);
      }

    if (api_mode == API_MODE_NEW_API_REF_COUNT)
      av_dict_set(&opts, "refcounted_frames", "1", 0);
    if ((ret = avcodec_open2(dec_ctx, dec, &opts)) < 0)
      {
      fprintf(stderr, "Failed to open %s codec\n", av_get_media_type_string(type));
      return ret;
      }
    *stream_idx = stream_index;
    } ;

  av_init_packet ( &pkt );
  pkt.data = NULL;
  pkt.size = 0 ;
  EoP      = 1 ;
  EoS      = 0 ;

  return 0;
  }

/***************************************************************************************/

int CVideoParser::OpenVideo(char* FileName)
  {
  int ret;
  av_register_all();     // register all formats and codecs

  if (avformat_open_input(&fmt_ctx, FileName, NULL, NULL) < 0)          // open input file, and allocate format context
    ErrorReturn("Could not open source file %s\n", FileName, FALSE)

  if (avformat_find_stream_info(fmt_ctx, NULL) < 0)                     // retrieve stream information
    ErrorReturn("Could not find stream information\n", NULL, FALSE)

    if (OpenCodecContext(&video_stream_idx, fmt_ctx, AVMEDIA_TYPE_VIDEO, FileName) >= 0)
      {
      video_stream  = fmt_ctx->streams[video_stream_idx];
      CtxV          = video_stream->codec;
      Started       = FALSE ;
      Opened        = TRUE ;
      InitialWidth  = CtxV->width ;
      InitialHeight = CtxV->height ;
      TS0           = AV_NOPTS_VALUE ;
      InitialFormat = CtxV->pix_fmt ;
      CtxV->thread_count = 1;

      if ((ret = av_image_alloc(video_dst_data, video_dst_linesize, CtxV->width, CtxV->height, CtxV->pix_fmt, 1)) < 0)     // allocate image buffer where the decoded image will be put
        ErrorReturn("Could not allocate raw video buffer\n", NULL, FALSE);
      video_dst_bufsize = ret;
      }
    else
      ErrorReturn("File does not contain a recognizable video-stream\n", NULL, FALSE);

  av_dump_format(fmt_ctx, 0, FileName, 0);    // dump input information to stderr

  if ((frame = av_frame_alloc()) == NULL)
    ErrorReturn("Could not allocate frame\n", NULL, FALSE);

  return TRUE;
  }

/**************************************************************************************************************/

int CVideoParser::ReadNextFrame()
  {
  AVPacket orig_pkt ;
  int      got_frame=0, result=0 ;

  if( EoP && !EoS )           // EoP: The packet read in last call may contain another frame. EoS: there may be more frames in pipeline
    if( av_read_frame( fmt_ctx, &pkt ) < 0 )
      {
      pkt.data = NULL ;       // to flush the pipeline at EoF condition
      pkt.size = 0;
      EoS      = 1;
      } ;

  // Only read video streams
  if( pkt.stream_index != video_stream->index )
    return( 0 ) ;

  if( EoP && !EoS )
    orig_pkt = pkt;
  if( (result = decode_packet( &got_frame )) < 0 )
    {
    av_log( NULL, AV_LOG_ERROR, "Error decoding frame\n" );
    return result;
    }
  else if( got_frame )
    {
    AnalysePacket( CtxV, frame, &FrameStat ) ;
    pkt.data += result ;
    pkt.size -= result ;
  if( (EoP = (int)(pkt.size == 0)) && !EoS )
      av_free_packet( &orig_pkt );
    } ;

  if ( got_frame && api_mode == API_MODE_NEW_API_REF_COUNT)
    av_frame_unref(frame);

  return( ( got_frame )? 1:-EoS ) ;
  }

/*********************************************************************************************************/


int CVideoParser::decode_packet(int *got_frame )
  {
  int ret = 0;
  char ErrorStr[AV_ERROR_MAX_STRING_SIZE] = { 0 };

  *got_frame = 0;

  if (pkt.stream_index == video_stream_idx)
    if((ret = avcodec_decode_video2( CtxV, frame, got_frame, &pkt)) < 0)
      {
      av_strerror(ret, ErrorStr, AV_ERROR_MAX_STRING_SIZE);
      fprintf(stderr, "Error decoding video frame: (%s)\n", ErrorStr );
      return ret;
      } ;

  return( ret );
  }


/************************************************************************************************/

int CVideoParser::AnalysePacket(AVCodecContext *CtxV, AVFrame* frame,  VIDEO_STAT* FrameStat )
  {
  int64_t  TS ;
  int ret = 0;
  char ts_str[AV_TS_MAX_STRING_SIZE]     = { 0 };

if( (InitialWidth != CtxV->width) || (InitialHeight != CtxV->height) || (InitialFormat != CtxV->pix_fmt) )
    {
    fprintf( stderr, "Error: Width, height and pixel format have to be constant in a rawvideo file, but the width, height or pixel format of the input video changed\n" ) ;
    return -1 ;
  } ;

  PicCnt++ ;
  FrameStat->FrameIdx = DebugCnt = PicCnt ;
  if( (CtxV->codec_id == AV_CODEC_ID_H264) )
    GetVideoStatisticsH264( CtxV, frame, FrameStat );
  else if( CtxV->codec_id == AV_CODEC_ID_HEVC )
    GetVideoStatisticsHEVC( CtxV, frame,  FrameStat ) ;
  else if( CtxV->codec_id == AV_CODEC_ID_VP9 )
    GetVideoStatisticsVP9( CtxV, frame, FrameStat ) ;
  else
    {
    fprintf( stderr, "Error: No statistics available for unknown video codec (ID=%d)\n", CtxV->codec_id ) ;
    return -1;
    } ;

  if( frame )
    {
    if( frame->pkt_dts  != AV_NOPTS_VALUE )
      TS = frame->pkt_dts ;
    else if( frame->pts != AV_NOPTS_VALUE )
      TS = frame->pts ;
    else
      TS = AV_NOPTS_VALUE ;

    if( TS0 == AV_NOPTS_VALUE )
      TS0 = TS ;
    TS -= TS0 ;

    av_ts_make_time_string( ts_str, TS, &CtxV->time_base ) ;
    fprintf( stderr, "FrmNum:%5d  FrmType: %c   FrmSize: %7d   POC: %5d   Blcky: %5.3lf  %5.3lf  %5.3lf\n", video_frame_count++, av_get_picture_type_char(frame->pict_type), FrameStat->FrameSize, FrameStat->CurrPOC,
             FrameStat->Blockiness[0][4], FrameStat->Blockiness[1][4], FrameStat->Blockiness[2][4] );

    FrameStat->PTS = frame->pts ;
    FrameStat->DTS = frame->pkt_dts ;
    } ;

  return 1;
  }

/***********************************************************************************************/

void CVideoParser::CloseParser()
  {
  av_packet_unref(&pkt);

  avcodec_close(CtxV);
  avformat_close_input(&fmt_ctx);
  if (api_mode == API_MODE_OLD)
    av_frame_free(&frame);
  av_free(video_dst_data[0]);
  }

/************************************************************************************************/
