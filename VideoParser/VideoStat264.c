#include "stdafx.h"
#include "VideoStat.h"
#include "VideoStatCommon.h"
#include "libavcodec/h264dec.h"

static void   EstimateBlackBorder264( H264Context* Ctx, H264SliceContext* sl, AVFrame* frame, uint32_t  MbType, int NumBlkQ, int FrameType ) ;
static void   MV_Statistics264( VIDEO_STAT* FrmStat, H264SliceContext* Sl, H264Picture* CurrPic, uint32_t MbType, uint16_t* Subtypes,
              int16_t( *motionL0 )[2], int16_t( *motionL1 )[2], int8_t( *motiondifL0 )[2], int8_t( *motiondifL1 )[2], float* NormFieldX, float* NormFieldY, int8_t* RefL0, int8_t* RefL1, int width, int SubStride, int FrameType ) ;
static int    GetMbTypeIdx264 ( double* MbStat,    uint32_t CurrType, uint16_t* SubMbType );
static void   GetShapeIdx264  ( double* ShapeStat, uint32_t CurrType, uint16_t* SubType );
double        GroundTruthTI264(  int16_t  ( *MotionVal )[2], int width, int height, double  FilteredOutput[2] ) ;
extern int    GroundTruth_P910( AVFrame* frame, VIDEO_STAT* FrmStat, int width, int height, int pix_fmt ) ;


/******************************************************************************/

int GetVideoStatisticsH264( AVCodecContext* av_ctx, AVFrame* frame, VIDEO_STAT* FrameStat )
  {
  H264Context*  Ctx = (H264Context*) av_ctx->priv_data ;
  VIDEO_STAT*   FrmStat = &(frame->FrmStat) ;

  int        NumMb ;

  int        FrameType = frame->pict_type, FrameTypeInt ;

  if( !Started && (FrameType != AV_PICTURE_TYPE_I) )
    return(0) ;

  if( FrmStat->Seq.BitDepth == 0 )
    {
    FrmStat->Seq.BitDepth     = Ctx->ps.sps->bit_depth_luma ;
    FrmStat->Seq.Profile      = Ctx->ps.sps->profile_idc ;
    FrmStat->Seq.Level        = Ctx->ps.sps->level_idc ;
    FrmStat->Seq.ChromaFormat = Ctx->ps.sps->chroma_format_idc ;
    FrmStat->Seq.ArithmCoding = Ctx->ps.pps->cabac ;
    FrmStat->Seq.FrameWidth   = frame->width  ;
    FrmStat->Seq.FrameHeight  = frame->height ;
    FrmStat->Seq.Codec        = 1 ;
    } ;

  Started            = 1 ;
  NumMb              = Ctx->mb_width * Ctx->mb_height ;
  FrmStat->FrameIdx  = FrameStat->FrameIdx ;

  GroundTruth_P910( frame, FrmStat, Ctx->width, Ctx->height, Ctx->avctx->pix_fmt ) ;
  if( (FrmStat->FrameIdx > 1) )
    GroundTruthTI264( Ctx->cur_pic.motion_val[0], (Ctx->mb_width  << 2), (Ctx->mb_height  << 2), FrmStat->TI_Mot )  ;

  if( (FrameStat->FrameIdx > 1) && (FrameType != AV_PICTURE_TYPE_I) )
    GroundTruthTI( FrmStat->S.NormalizedField, (frame->width >> 2), (frame->height >> 2), FrmStat->TI_Mot )  ;

  FrmStat->FrameType = FrameType ;
  FrmStat->FrameSize = av_frame_get_pkt_size( frame ) ;
  FrameTypeInt       = (FrameType == AV_PICTURE_TYPE_I) ? 2 : ((FrameType == AV_PICTURE_TYPE_P) ? 0 : 3) ;
  CoefsStatistics( FrmStat, frame->FrmStat.S.AverageCoefs, frame->FrmStat.S.AverageCoefsSQR, frame->FrmStat.S.AverageCoefsBlkCnt, FrmStat->TrShapes, 2, FrameTypeInt ) ;

  FinishStatistics( FrmStat, FrameType, frame->width*frame->height, NumMb*16 ) ;
  memcpy( FrameStat, FrmStat, sizeof( VIDEO_STAT ) ) ;

  assert( (FrmStat->TrShapes[0] + FrmStat->TrShapes[2] + 4*FrmStat->TrShapes[3]) ==  FrmStat->AnalyzedMBs*16) ;   // 4x4 based
  assert( (FrmStat->MbTypes [0] + FrmStat->MbTypes [1] + FrmStat->MbTypes[2] + FrmStat->MbTypes[3] + FrmStat->MbTypes[4] + FrmStat->MbTypes[5] + FrmStat->MbTypes[6]) ==  FrmStat->AnalyzedMBs*4) ; // 8x8 based
  assert( (FrmStat->BlkShapes[0] + FrmStat->BlkShapes[1]*2 + FrmStat->BlkShapes[2]*4 + FrmStat->BlkShapes[3]*8 + FrmStat->BlkShapes[4]*16) == FrmStat->AnalyzedMBs*16 ) ;

  return( 1 );
  }


/********************************************************************************************************************/

static void GetShapeIdx264( double* ShapeStat, uint32_t CurrType, uint16_t* SubType )
  {
  int    i ;

  if( (CurrType & MB_TYPE_16x16) || (CurrType & MB_TYPE_INTRA16x16) )
    ShapeStat[SHAPE_16x16] ++ ;
  else if( (CurrType & MB_TYPE_16x8) || (CurrType & MB_TYPE_8x16) )
    ShapeStat[SHAPE_16x8] += 2 ;
  else if( (CurrType & MB_TYPE_8x8) )
    {
    for( i = 0 ; i < 4 ; i++ )
      {
      if( SubType[i] & MB_TYPE_16x16 )             // symbols reused 16 means 8 here and 8 means 4
        ShapeStat[SHAPE_8x8]  ++ ;
      if( (SubType[i] & MB_TYPE_8x16) || (SubType[i] & MB_TYPE_16x8) )            // symbols reused 16 means 8 here and 8 means 4
        ShapeStat[SHAPE_8x4]  += 2 ;
      if( SubType[i] & MB_TYPE_8x8 )             // symbols reused 16 means 8 here and 8 means 4
        ShapeStat[SHAPE_4x4]  += 4 ;
      } ;
    }
  else if( (CurrType & MB_TYPE_INTRA4x4) )
    ShapeStat[SHAPE_4x4] += 16 ;
  }

/************************************************************************/

static int CountMbType( double* MbStat, int CurrType, int Fkt )
  {
  int  Type=-1 ;

  if( CurrType & MB_TYPE_DIRECT2 )
    Type = DIRECT ;
  else if( CurrType & MB_TYPE_SKIP )
    Type = SKIPPED ;
  else if( (CurrType & MB_TYPE_L0) && !(CurrType & MB_TYPE_L1) )
    Type = FORWARD ;
  else if( !(CurrType & MB_TYPE_L0) && (CurrType & MB_TYPE_L1) )
    Type = BACKWARD ;
  else if( (CurrType & MB_TYPE_L0) && (CurrType & MB_TYPE_L1) )
    Type = BIDIRECT ;
  else if( CurrType & MB_TYPE_INTRA4x4 )
    Type = INTRA_BLK ;
  else if( CurrType & MB_TYPE_INTRA16x16 )
    Type = INTRA_PLN ;

  if( Type >= 0 )
    MbStat[Type] += Fkt ;
  return(Type) ;
  }

/************************************************************************/

static int GetMbTypeIdx264( double* MbStat, uint32_t CurrType, uint16_t* SubMbType )
  {
  int    i, Type=-1 ;

  if( CurrType & MB_TYPE_16x16 )
    Type = CountMbType( MbStat, CurrType, 4) ;
  else if( (CurrType & MB_TYPE_DIRECT2) || (CurrType & MB_TYPE_8x8) )
    for( i = 0 ; i < 4 ; i++ )
      {
      Type = CountMbType( MbStat, SubMbType[i], 1 ) ;
      }
  else if( (CurrType & MB_TYPE_16x8) || (CurrType & MB_TYPE_8x16))
    {
    if(       (CurrType & MB_TYPE_P0L0) && !(CurrType & MB_TYPE_P0L1) )
     Type = FORWARD ;
    else if( !(CurrType & MB_TYPE_P0L0) &&  (CurrType & MB_TYPE_P0L1) )
      Type = BACKWARD ;
    else if(  (CurrType & MB_TYPE_P0L0) &&  (CurrType & MB_TYPE_P0L1) )
      Type = BIDIRECT ;
    MbStat[Type] += 2 ;

    if(       (CurrType & MB_TYPE_P1L0) && !(CurrType & MB_TYPE_P1L1) )
      Type = FORWARD ;
    else if( !(CurrType & MB_TYPE_P1L0) &&  (CurrType & MB_TYPE_P1L1) )
      Type = BACKWARD ;
    else if(  (CurrType & MB_TYPE_P1L0) &&  (CurrType & MB_TYPE_P1L1) )
      Type = BIDIRECT ;

    MbStat[Type] += 2 ;
    return(Type) ;
    }
  else         // INTRA Types 
    return( CountMbType( MbStat, CurrType, 4 ) ) ;
  return(Type) ;
  } ;

/*********************************************************************************************/

static void EstimateBlackBorder264( H264Context* Ctx, H264SliceContext* sl, AVFrame* frame, uint32_t  MbType, int NumBlkQ, int FrameType )
  {
  int       j, LoopEnd ;
  int       NonZeroCoefs ;
  uint8_t( *NZC_Table )[48];
  uint16_t  *Cbp_Table ;

  // New value calculated only for I-Frames
  if( FrameType == AV_PICTURE_TYPE_I )
    {
    NZC_Table = Ctx->non_zero_count ;                  // (MbRows+1) * (MbCols + 1)  -> N 16x16 grid
    Cbp_Table = Ctx->cbp_table ;                       // (MbRows+1) * (MbCols + 1)  -> N 16x16 grid
    LoopEnd   = Ctx->mb_height*Ctx->mb_stride ;

   if( MbType == 0 )
      return;

   for( j = 0, NonZeroCoefs=0 ; j<16 ; NonZeroCoefs += NZC_Table[sl->mb_xy][j++] ) ;

    if( (MbType & MB_TYPE_INTRA4x4) || (MbType & MB_TYPE_INTRA16x16) )
      {
      if( ((sl->mb_x != 0) || (sl->mb_y != 0)) && !Cbp_Table[sl->mb_xy] && (NonZeroCoefs == 0) )
        Ctx->BlackLine[sl->mb_y] = min( 255, Ctx->BlackLine[sl->mb_y] + 1 ) ;
      } ;
    } ;
  }

/*********************************************************************************************/

static void MV_Statistics264( VIDEO_STAT* FrmStat, H264SliceContext* Sl, H264Picture* CurrPic, uint32_t CurrType, uint16_t* Subtypes, int16_t( *motionL0 )[2], int16_t( *motionL1 )[2], 
                int8_t( *motiondifL0 )[2], int8_t( *motiondifL1 )[2], float* NormFieldX, float* NormFieldY, int8_t* RefL0, int8_t* RefL1, int width, int SubStride, int FrameType )
  {
  int          blk4, blk8, mvIdx, NrmIdx, DirCnt ;
  int          Ref0POC, Ref1POC ;
  int          isFWD, isBWD ;
  double       NormFwd, NormBwd ;
  double       MV_LengthXY, MV_LengthdXY ;
  double       mvX, mvY, mvdX, mvdY ;
  FRAME_SUMS*  S = &FrmStat->S ;


  CurrType = CurrType & 0xffff ;

  for( blk8 = 0 ; blk8 < 4 ; blk8++ )
    {
    S->NumBlksMv += 4 ;               // includes DIRECT mode where no motion is coded, but default prediction is used
    Ref0POC = Sl->ref_list[0][RefL0[ scan8[blk8<<2] ]].poc - ((Sl->ref_list[0][ RefL0[scan8[blk8<<2] ]].poc > 32768) ? 65536 : 0) ;
    Ref1POC = Sl->ref_list[1][RefL1[ scan8[blk8<<2] ]].poc - ((Sl->ref_list[1][ RefL1[scan8[blk8<<2] ]].poc > 32768) ? 65536 : 0) ;

    if( (CurrType & MB_TYPE_L0) || (CurrType & MB_TYPE_DIRECT2))
      NormFwd = 1.0 / (2.0 * fabs( ((double)FrmStat->CurrPOC - (double)Ref0POC) / FrmStat->POC_DIF )) ;
    if( (CurrType & MB_TYPE_L1) || (CurrType & MB_TYPE_DIRECT2))
      NormBwd = 1.0 / (2.0 * fabs( ((double)FrmStat->CurrPOC - (double)Ref1POC) / FrmStat->POC_DIF ));

    CurrType = ((FrameType == AV_PICTURE_TYPE_B) && (CurrType & MB_TYPE_8x8))?  Subtypes[ (blk8 & 1) + (int)(blk8 > 1)*SubStride ] : CurrType ;

    isFWD    = ((CurrType & MB_TYPE_16x16) && (CurrType & MB_TYPE_P0L0))
            || ((CurrType & MB_TYPE_8x8  ) && (CurrType & MB_TYPE_L0))
            || ((CurrType & MB_TYPE_16x8 ) && ((( blk8 < 2) && (CurrType & MB_TYPE_P0L0)) || ((blk8 > 1) && (CurrType & MB_TYPE_P1L0))))
            || ((CurrType & MB_TYPE_8x16 ) && ((!(blk8 & 1) && (CurrType & MB_TYPE_P0L0)) || ((blk8 & 1) && (CurrType & MB_TYPE_P1L0)))) ;

    isBWD    = ((CurrType & MB_TYPE_16x16) && (CurrType & MB_TYPE_P0L1))
            || ((CurrType & MB_TYPE_8x8  ) && (CurrType & MB_TYPE_L1))
            || ((CurrType & MB_TYPE_16x8 ) && ((( blk8 < 2) && (CurrType & MB_TYPE_P0L1)) || ((blk8 > 1) && (CurrType & MB_TYPE_P1L1))))
            || ((CurrType & MB_TYPE_8x16 ) && ((!(blk8 & 1) && (CurrType & MB_TYPE_P0L1)) || ((blk8 & 1) && (CurrType & MB_TYPE_P1L1)))) ;

    if( isFWD && ((FrmStat->CurrPOC - Ref0POC) == 0) )
      Ref0POC = Ref0POC ;
    if( isBWD && ((FrmStat->CurrPOC - Ref1POC) == 0) )
      Ref1POC = Ref1POC ;


    for( blk4 = 0 ; blk4 < 4 ; blk4++ )
      {
      mvIdx  = scan8[(blk8 << 2) + blk4] ;
      NrmIdx = ((blk8 & 0x2) + (blk4 >> 1))*width   + ((blk8 & 1) << 1) + (blk4 & 1) ;
      mvX   = mvY = mvdX = mvdY = DirCnt = 0 ;
      if( isFWD )
        {
        DirCnt++;
        mvX   = fabs( motionL0[mvIdx][0] )   * NormFwd ;
        mvY   = fabs( motionL0[mvIdx][1] )   * NormFwd ;
        mvdX  = fabs( motiondifL0[mvIdx][0]) * NormFwd ;
        mvdY  = fabs( motiondifL0[mvIdx][1]) * NormFwd ;
        mvdX  = (mvdX > 127) ? mvdX - 255 : mvdX ;
        mvdY  = (mvdY > 127) ? mvdY - 255 : mvdY ;
        mvdX  = fabs( mvdX ) ;
        mvdY  = fabs( mvdY ) ;
        NormFieldX[NrmIdx] = (float)mvX ;
        NormFieldY[NrmIdx] = (float)mvY ;
        }

      if( isBWD )
        { 
        DirCnt++;
        mvX  += fabs( motionL1[mvIdx][0] )    * NormBwd ;
        mvY  += fabs( motionL1[mvIdx][1] )    * NormBwd ;
        mvdX += fabs( motiondifL1[mvIdx][0])  * NormBwd ;
        mvdY += fabs( motiondifL1[mvIdx][1])  * NormBwd ;
        mvdX  = (mvdX > 127) ? mvdX - 255 : mvdX ;
        mvdY  = (mvdY > 127) ? mvdY - 255 : mvdY ;
        mvdX  = fabs( mvdX ) ;
        mvdY  = fabs( mvdY ) ;
        } ;
      if( DirCnt )
        {
        mvX  /= DirCnt ; mvY  /= DirCnt ;
        mvdX /= DirCnt ; mvdY /= DirCnt ;
        MV_LengthXY        = sqrt( SQR(  mvX ) + SQR(  mvY ) ) ;
        MV_LengthdXY       = sqrt( SQR( mvdX ) + SQR( mvdY ) ) ;
        S->MV_Length      += MV_LengthXY ;            // Add up of motion vector length for mean value
        S->MV_dLength     += MV_LengthdXY ;            // Add up of motion vector length for mean value
        S->MV_SumSQR      += SQR( MV_LengthXY ) ; //Add up square of motion vector length for variance value
        S->MV_DifSumSQR   += SQR( MV_LengthdXY ) ;
        S->MV_LengthX     += mvX ;
        S->MV_LengthY     += mvY ;
        S->MV_XSQR        += SQR( mvX ) ;
        S->MV_YSQR        += SQR( mvY ) ;
        } ;
      } ;
    } ;
  }

/************************************************************************************************************************/

void MbStatistics264(  H264Context *h, H264SliceContext *sl, int mb_type )
  {
  H264Picture*      CurrPic = h->cur_pic_ptr ;
  AVFrame*          frame=CurrPic->f ;
  VIDEO_STAT*       FrmStat = &(frame->FrmStat) ;
  double            Coef, CoefSum ;
  int16_t*          Coefs = sl->mb ;
  int               blk8, blk4, blk, i, j, ii, MbTypeI ;
  int               FrameType = CurrPic->f->pict_type ;
  int               MbType, NumBlkT, NumBlkQ, NoBorder ;
  float             *NormFieldX, *NormFieldY ;


  NumBlkT   = CurrPic->mb_type_buf->size;         // (MbCols + 1) * (MbRows + 1) * 4  -> 16*16 grid
  NumBlkQ   = CurrPic->qscale_table_buf->size ;   // (MbCols + 1) * (MbRows + 2) + 1  -> 16x16 grid
  assert( NumBlkT == (NumBlkQ << 2) ) ;
  
  if( FrameType == AV_PICTURE_TYPE_I )
    EstimateBlackBorder264( h, sl, frame, mb_type, NumBlkQ, FrameType ) ;

  if( Opened && (mb_type != 0) )
    {
    MbTypeI  = (int)((mb_type & 0x7) != 0) ;
    NoBorder = ((sl->mb_y << 4) >= FrmStat->BlackBorder) && ((sl->mb_y << 4) < (h->height - FrmStat->BlackBorder)) ;
    FrmStat->S.MbCnt++ ;


    MbType = GetMbTypeIdx264( FrmStat->MbTypes,  mb_type, sl->sub_mb_type ) ;
    GetShapeIdx264( FrmStat->BlkShapes, mb_type, sl->sub_mb_type ) ;
    QPStatistics( FrmStat, sl->qscale, MbType,  NoBorder ) ;

    if( (FrameType != AV_PICTURE_TYPE_I) && !MbTypeI )
      { 
      for( j = 0 ; j < 4 ; j++ )
        FrmStat->FarFWDRef[(sl->ref_cache[0][scan8[j << 2]] != 255) && (sl->ref_cache[0][scan8[j << 2]] > 0)]++ ;

      NormFieldX = FrmStat->S.NormalizedField[0] + (sl->mb_y*h->mb_width << 2) + (sl->mb_x << 2) ;
      NormFieldY = FrmStat->S.NormalizedField[1] + (sl->mb_y*h->mb_width << 2) + (sl->mb_x << 2) ;
      if( mb_type & MB_TYPE_SKIP )
        FrmStat->S.NumBlksSkip += 4 ;
      else
        MV_Statistics264( FrmStat, sl, CurrPic, mb_type, sl->sub_mb_type, sl->mv_cache[0], sl->mv_cache[1], sl->mvd_cache[0], sl->mvd_cache[1], 
                          NormFieldX, NormFieldY, sl->ref_cache[0], sl->ref_cache[1], h->mb_width<<2, (h->mb_stride << 1), FrameType ) ;
      }
    else
      FrmStat->S.NumBlksIntra += 16 ;
      
    for( blk8=0 ; blk8<4 ; blk8++ )
      if( (sl->cbp & (1 << blk8)) )
        FrmStat->TrShapes[(int)(IS_8x8DCT( mb_type )!=0) + 2] += (IS_8x8DCT( mb_type))? 1:4  ;       
      else
        FrmStat->TrShapes[0] += 4  ;       


    if( (IS_INTRA( mb_type ) && (sl->slice_type_nos == AV_PICTURE_TYPE_I)) || (!IS_INTRA( mb_type ) && (sl->slice_type_nos != AV_PICTURE_TYPE_I)) )
      {
      if( IS_INTRA( mb_type ) || (sl->cbp & 0x0f) )   // there must be at least one non zero coefficient
        {
        if( IS_8x8DCT( mb_type ) )
          {
          for( blk8 = 0 ; blk8 < 4 ; blk8++ )
            if( IS_INTRA( mb_type ) || (sl->cbp & (1 << blk8)) )
              {
              for( i = 0 ; i < 64 ; i++ )
                {
                Coef = (double)abs( sl->mb0[(blk8 << 6) + ((i << 3) & 0x3f) + (i >> 3)] ) ;
                FrmStat->S.AverageCoefs[TRANSF_8X8][i]     += Coef ;
                FrmStat->S.AverageCoefsSQR[TRANSF_8X8][i ] += Coef*Coef ;
              FrmStat->S.AverageCoefsBlkCnt[TRANSF_8X8][i] += (double)(Coef != 0)  ;
                } ;
              } 
          }
        else
          {
          for( blk = 0 ; blk < 16 ; blk++ )
            {
            blk8 = (((int)blk > 7) << 1) + (int)((blk & 0x02) != 0) ;
            blk4 = (blk & 1) + ((int)((blk & 4) != 0) << 1) ;
            if( IS_INTRA( mb_type ) || (sl->cbp & (1 << blk8)) )
              {
              for( i = 0, CoefSum = 0 ; i < 16 ; i++ )
                {
                ii = ((i << 2) & 0xf) + (i >> 2) ;
                Coef = (double)abs( sl->mb0[((blk4 + (blk8 << 2)) << 4) + ii] ) ;
                CoefSum += Coef ;
                FrmStat->S.AverageCoefs[TRANSF_4X4][i]       += Coef ;
                FrmStat->S.AverageCoefsSQR[TRANSF_4X4][i]    += Coef*Coef ;
                FrmStat->S.AverageCoefsBlkCnt[TRANSF_4X4][i] += (double)(Coef != 0) ;
                } ;
              } 
            } ;
          } ;
        } 
      } ;
    memset( sl->mb0, 0, sizeof( sl->mb0 ) ) ;
    sl->cbp = 0 ;
    } ;
  }


/*******************************************************************************************************/

void SaveDCs264( int16_t* MbDst, int16_t* MbSrc )
  {
  for( int i = 0 ; i < 256 ; i += 16 )
    MbDst[i] = MbSrc[i] ;
  }

/*******************************************************************************************************/

void InitFrameStatistics264( H264Context *h, H264SliceContext* sl, int CurrBlackBorder )
  {
  static int        PrevPOC=0, POC_Dif=8 ;
  static int64_t    PrevPTS ;
  int               PTS_Dif ;
  H264Picture*      CurrPic = h->cur_pic_ptr ;
  AVFrame*          frame = CurrPic->f ;
  VIDEO_STAT*       FrmStat = &(frame->FrmStat) ;


  int FieldSize = (h->mb_width*h->mb_height) << 4 ;

  SAFE_FREE( FrmStat->S.NormalizedField[0] ) ;
  SAFE_FREE( FrmStat->S.NormalizedField[1] ) ;
  memset( FrmStat, 0, sizeof( VIDEO_STAT ) ) ;

  memset( sl->sub_mb_type, 0, sizeof( sl->sub_mb_type ) ) ;
  h->BlackLine = calloc( h->height >> 2, sizeof( int ) ) ;
  FrmStat->BlackBorder = CurrBlackBorder ;
  FrmStat->IsIDR       = frame->key_frame ;
  FrmStat->InitialQP   = FrmStat->Av_QP = FrmStat->Av_QPBB = h->slice_ctx->qscale ;
  FrmStat->max_QP      = 0 ;
  FrmStat->min_QP      = 100 ;
  FrmStat->CurrPOC     = CurrPic->poc - ((CurrPic->poc > 32768) ? 65536 : 0) ;

  if( abs( FrmStat->CurrPOC - PrevPOC ) != 0 )
    {
    if( (frame->pts == 0) || (frame->pkt_duration == 0) )
      POC_Dif = min( POC_Dif, abs( FrmStat->CurrPOC - PrevPOC ) ) ;
    else
      {
      PTS_Dif = max( 1, (int)nearbyint( fabs( (double)frame->pts - PrevPTS ) / (double)frame->pkt_duration)) ;
      POC_Dif = abs( FrmStat->CurrPOC - PrevPOC ) / PTS_Dif ;
      } ;
    } 
  FrmStat->POC_DIF     = POC_Dif ;
  PrevPOC              = FrmStat->CurrPOC ;
  PrevPTS              = frame->pts ;

  FrmStat->S.NormalizedField[0] = calloc( FieldSize, sizeof( float )) ;
  FrmStat->S.NormalizedField[1] = calloc( FieldSize, sizeof( float ))  ;
  }

/*******************************************************************************************************/


double GroundTruthTI264( int16_t ( *MotionVal )[2], int width, int height, double FilteredOutput[2] )

  {
  int      i, j, Idx ;
  double   Valxh1, Valyh1, Valxv1, Valyv1, Val1, ValX1, ValY1 ;
  double   Valxh2, Valyh2, Valxv2, Valyv2, Val2, ValX2, ValY2 ;
  int      NumBlk  = width * height ;
  double   SobelSum = 0.0, LaplaceSum = 0.0 ;

  for ( i=1; i<height-1; i++)  
    for ( j=1; j<width-1; j++) 
      {
      Idx     =  (i - 1)*width + j ;
      Valxh1  =  MotionVal[Idx-1][0] + (MotionVal[Idx][0] << 1) + MotionVal[Idx + 1][0] ;                    // obere Zeile 
      Valyh1  =  MotionVal[Idx-1][1] + (MotionVal[Idx][1] << 1) + MotionVal[Idx + 1][1] ;
      Valxh2  =  MotionVal[Idx-1][0] +  MotionVal[Idx][0]       + MotionVal[Idx + 1][0] ;
      Valyh2  =  MotionVal[Idx-1][1] +  MotionVal[Idx][1]       + MotionVal[Idx + 1][1] ;

      Valxh2 +=  MotionVal[Idx+width-1][0] - (MotionVal[Idx+width][0] << 3) + MotionVal[Idx+width + 1][0] ;  // mittlere Zeile
      Valyh2 +=  MotionVal[Idx+width-1][1] - (MotionVal[Idx+width][1] << 3) + MotionVal[Idx+width + 1][1] ;

      Idx    +=  2 * width ;
      Valxh1 += -MotionVal[Idx-1][0] - (MotionVal[Idx][0] << 1) - MotionVal[Idx + 1][0] ;                    // untere Zeile
      Valyh1 += -MotionVal[Idx-1][1] - (MotionVal[Idx][1] << 1) - MotionVal[Idx + 1][1] ;
      Valxh2 +=  MotionVal[Idx-1][0] +  MotionVal[Idx][0]       + MotionVal[Idx + 1][0] ;
      Valyh2 +=  MotionVal[Idx-1][1] +  MotionVal[Idx][1]       + MotionVal[Idx + 1][1] ;
      // ------------- vertikal ---------------------------------------------------
      Idx     =  i*width + j-1 ;
      Valxv1  =  MotionVal[Idx-width][0] + (MotionVal[Idx][0] << 1) + MotionVal[Idx+width][0] ;              // linke Spalte 
      Valyv1  =  MotionVal[Idx-width][1] + (MotionVal[Idx][1] << 1) + MotionVal[Idx+width][1] ;
      Valxv2  =  MotionVal[Idx-width][0] +  MotionVal[Idx][0]       + MotionVal[Idx+width][0] ;                   
      Valyv2  =  MotionVal[Idx-width][1] +  MotionVal[Idx][1]       + MotionVal[Idx+width][1] ;
      Idx    ++ ;
      Valxv2 +=  MotionVal[Idx-width][0] - (MotionVal[Idx][0] << 3) + MotionVal[Idx+width][0] ;              // mittlere Spalte
      Valyv2 +=  MotionVal[Idx-width][1] - (MotionVal[Idx][1] << 3) + MotionVal[Idx+width][1] ;
      Idx    ++ ;
      Valxv1 += -MotionVal[Idx-width][0] - (MotionVal[Idx][0] << 1) - MotionVal[Idx+width][0] ;              // rechte Spalte 
      Valyv1 += -MotionVal[Idx-width][1] - (MotionVal[Idx][1] << 1) - MotionVal[Idx+width][1] ;
      Valxv2 +=  MotionVal[Idx-width][0] +  MotionVal[Idx][0]       + MotionVal[Idx+width][0] ;                   
      Valyv2 +=  MotionVal[Idx-width][1] +  MotionVal[Idx][1]       + MotionVal[Idx+width][1] ;

      ValX1 = sqrt( SQR( Valxv1 ) + SQR( Valxh1 ) ) ;
      ValY1 = sqrt( SQR( Valyv1 ) + SQR( Valyh1 ) ) ;
      Val1  = sqrt( SQR( ValX1  ) + SQR( ValY1  ) ) ;
      SobelSum += ICLIP( SobelMax, SobelMin, Val1 ) ;

      ValX2 = sqrt( SQR( Valxv2 ) + SQR( Valxh2 ) ) ;
      ValY2 = sqrt( SQR( Valyv2 ) + SQR( Valyh2 ) ) ;
      Val2  = sqrt( SQR( ValX2  ) + SQR( ValY2  ) ) ;
      LaplaceSum += ICLIP( SobelMax, SobelMin, Val2 ) ;
      if( SobelSum == 0 && LaplaceSum!= 0 )
        SobelSum = SobelSum ;
      }

  FilteredOutput[0] = SobelSum / NumBlk ; 
  FilteredOutput[1] = LaplaceSum / NumBlk ; 
  return(  FilteredOutput[0] ) ;
  }

