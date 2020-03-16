#include "stdafx.h"
#include "VideoStat.h"
#include "VideoStatCommon.h"
#include "libavcodec/hevc.h"

void   ShapeStatisticsHEVC( VIDEO_STAT* FrmStat, int PU_Stat[7][6], int NumBlk ) ;
void   MV_StatisticsHEVC( HEVCContext* Ctx, VIDEO_STAT* FrmStat, FRAME_SUMS* S, BYTE CurrType, MvField* MvF, float* NormField[2], int MinPuWidth, int MBs) ;
double GroundTruthTI( float* NormField[2], int width, int height, double FilteredOutput[2] ) ;


/**********************************************************************************************************************/
// array  | HEVC-Name    |     measure           |   found in                        |  Size                 |  type   |
//______________________________________________________________________________________________________________________
// QP     | qp_y_tab     |   Coding Block Size   |   log2_min_cb_size             = 8|  (width/8)*(height/8) |   char  |
// Type   | type_tab     |   Coding Block Size   |   log2_min_cb_size             = 8|  (width/8)*(height/8) |   char  |
// cbf    | cbf_luma     |   TransformBlockSize  |   min_tb_width / min_tb_height = 4|  (width/4)*(height/4) |   byte  |
// Motion | ref->tab_mvf |   PredictionUnitSize  |   min_pu_width / min_pu_height = 4|  (width/4)*(height/4) | MvField |
/**********************************************************************************************************************/

int GetVideoStatisticsHEVC( AVCodecContext* av_ctx, AVFrame* Frame, VIDEO_STAT* FrameStat )
  {
  HEVCContext* Ctx = (HEVCContext*) av_ctx->priv_data;
  HEVCSPS*     sps = (HEVCSPS*)Ctx->ps.sps ;
  VIDEO_STAT*  FrmStat = &(Frame->FrmStat) ;
  BYTE         FrameType=0 ;
  int          i, Tmp, NumBlk0, NumBlk1, NumBlk2, NumPel ;
  double       FramesPerSec=0 ;

  if( !Started && (Frame->pict_type != AV_PICTURE_TYPE_I) )
    return(0) ;

  if( FrmStat->Seq.BitDepth == 0 )
    {
    FrmStat->Seq.BitDepth     = sps->bit_depth ;
    FrmStat->Seq.Profile      = sps->ptl.general_ptl.profile_idc ;
    FrmStat->Seq.Level        = sps->ptl.general_ptl.level_idc ;
    FrmStat->Seq.ChromaFormat = sps->chroma_format_idc ;
    FrmStat->Seq.FramesPerSec = (double)av_ctx->time_base.den / (double)(av_ctx->ticks_per_frame * av_ctx->time_base.num) ;
    FrmStat->Seq.Bitrate      = (int)av_ctx->bit_rate ;
    FrmStat->Seq.BitsPertPel  = (double)(FrmStat->Seq.Bitrate) / (Frame->width*Frame->height * FrmStat->Seq.FramesPerSec) ;
    FrmStat->Seq.Resolution   = GetResolution( Frame->width*Frame->height ) ;
    FrmStat->Seq.FrameWidth   = Frame->width  ;
    FrmStat->Seq.FrameHeight  = Frame->height ;

    FrmStat->Seq.ArithmCoding = 1 ;
    FrmStat->Seq.Codec        = 2 ;
    }

  
  Started            = 1 ;
  FrameType          = Frame->pict_type ;
  FrmStat->FrameType = FrameType ;
  FrmStat->FrameSize = Frame->pkt_size ;
  FrmStat->FrameIdx  = FrameStat->FrameIdx ;
  NumPel             = (Frame->width*Frame->height) ;
  NumBlk0            = NumPel / ((1 << sps->log2_min_tb_size)*(1 << sps->log2_min_tb_size)) ;
  NumBlk1            = NumPel / ((1 << sps->log2_min_cb_size)*(1 << sps->log2_min_cb_size)) ;
  NumBlk2            = NumPel / ((1 << sps->log2_min_pu_size)*(1 << sps->log2_min_pu_size)) ;
  GroundTruth_P910( Frame, FrmStat, Frame->width, Frame->height, Ctx->avctx->pix_fmt ) ;
  if( (FrameStat->FrameIdx > 1) && (FrameType != AV_PICTURE_TYPE_I) )
    GroundTruthTI( FrmStat->S.NormalizedField, (Frame->width >> sps->log2_min_pu_size), (Frame->height >> sps->log2_min_pu_size), FrmStat->TI_Mot )  ;
  
  ShapeStatisticsHEVC( FrmStat, FrmStat->S.PU_Stat, NumBlk0 ) ;
  CoefsStatistics( FrmStat, FrmStat->S.AverageCoefs, FrmStat->S.AverageCoefsSQR, FrmStat->S.AverageCoefsBlkCnt, FrmStat->TrShapes, 4, FrameType ) ;
  FinishStatistics( FrmStat, FrameType, NumPel, NumBlk0 ) ;

  for( i = 0 ; i < 7 ; FrmStat->MbQPs[i++] = FrmStat->MbQPs[i] / 4.0 ) ;

  memcpy( FrameStat, FrmStat, sizeof( VIDEO_STAT ) ) ;

  for( i=0, Tmp=0 ; i<9 ; i++ )  Tmp += (int)FrmStat->BlkShapes[i] * (1 << i) ;
  //assert(Tmp == NumBlk0 ) ;
  //assert( (FrmStat->TrShapes[0] + FrmStat->TrShapes[2] + 4*FrmStat->TrShapes[3] + 16*FrmStat->TrShapes[4] + 64*FrmStat->TrShapes[5]) == NumBlk0) ;
  //assert( (FrmStat->MbTypes [0] + FrmStat->MbTypes [1] + FrmStat->MbTypes[2] + FrmStat->MbTypes[3] + FrmStat->MbTypes[4] + FrmStat->MbTypes[5] + FrmStat->MbTypes[6]) == NumBlk1) ; // 8x8 based
 
  return( 1 ) ;
  }


/**********************************************************************************************************************/

void TransfStatHEVC( HEVCContext* s, int log2_trafo_size, int cbf_luma, int y0, int x0 )
  {
  if( cbf_luma != 0 )
    s->frame->FrmStat.TrShapes[log2_trafo_size]++  ;
  else
    s->frame->FrmStat.TrShapes[0] += (1 << (log2_trafo_size - 2)) * (1 << (log2_trafo_size - 2)) ;
  }


/**********************************************************************************************************************/

void MV_StatisticsHEVC( HEVCContext* Ctx, VIDEO_STAT* FrmStat, FRAME_SUMS* S, BYTE CurrType, MvField* MvF, float* NormField[2], int MinPuWidth, int Mvs)
  {
  int       v, h, DirCnt ;
  int       Ref0POC, Ref1POC ;
  double    NormFwd=0, NormBwd=0 ;
  double    MV_LengthXY, MV_LengthdXY ;
  double    mvX, mvY, mvdX, mvdY ;
  float     *NFX, *NFY ;
  MvField*  CurrMvF ;

  // this function captures the motion values for the minimum cu bock (usually 8x8) which has one ref_idx. The Motion
  // itself can be 4x4 though and the MvF-Grid is 4x4 anyhow

  S->NumBlksMv     += (int)(Mvs * Mvs) ;
  FrmStat->CurrPOC  = Ctx->poc - ((Ctx->poc > 32768) ? 65536 : 0) ;

  Ref0POC = Ctx->ref->refPicList[0].list[MvF->ref_idx[0]] - ((Ctx->ref->refPicList[0].list[MvF->ref_idx[0]] > 32768) ? 65536 : 0) ;
  Ref1POC = Ctx->ref->refPicList[1].list[MvF->ref_idx[1]] - ((Ctx->ref->refPicList[1].list[MvF->ref_idx[1]] > 32768) ? 65536 : 0) ;
  
  if( (CurrType ==  FORWARD) || (CurrType == DIRECT) || (CurrType == BIDIRECT) )
    NormFwd = 1.0 / (2 * fabs( (FrmStat->CurrPOC - Ref0POC) / (double)FrmStat->POC_DIF )) ;
  if( (CurrType == BACKWARD) || (CurrType == DIRECT) || (CurrType == BIDIRECT) )
    NormBwd = 1.0 / (2 * fabs( (FrmStat->CurrPOC - Ref1POC) / (double)FrmStat->POC_DIF )) ;


  for( h=0 ; h<Mvs ; h++ )
    for( v=0 ; v<Mvs ; v++ )
      {
      CurrMvF = MvF          + h*MinPuWidth + v ;
      NFX     = NormField[0] + h*MinPuWidth + v ;
      NFY     = NormField[1] + h*MinPuWidth + v ;

      FrmStat->FarFWDRef[CurrMvF->ref_idx[0] > 0]++ ;

      *NFX = *NFY = 0.0 ;
      mvX = mvY = mvdX = mvdY = DirCnt = 0 ;
      if( NormFwd != 0.0)
        {
        DirCnt++;
        mvX   = abs( (int)(CurrMvF->mv [0].x) )   * NormFwd ;
        mvY   = abs( (int)(CurrMvF->mv [0].y) )   * NormFwd ;
        mvdX  = abs( (int)(CurrMvF->mvd[0].x) )   * NormFwd ;
        mvdY  = abs( (int)(CurrMvF->mvd[0].y) )   * NormFwd ;
        *NFX  = (float)mvX ;
        *NFY  = (float)mvY ;
        }

      if( NormBwd != 0.0)
        {
        DirCnt++;
        mvX  += abs( (int)(CurrMvF->mv [1].x) )   * NormBwd ;
        mvY  += abs( (int)(CurrMvF->mv [1].y) )   * NormBwd ;
        mvdX += abs( (int)(CurrMvF->mvd[1].x) )   * NormBwd ;
        mvdY += abs( (int)(CurrMvF->mvd[1].y) )   * NormBwd ;
        } ;

      if( DirCnt )
        {
        mvX  /= DirCnt ; mvY  /= DirCnt ;
        mvdX /= DirCnt ; mvdY /= DirCnt ;
        MV_LengthXY      = sqrt( SQR(  mvX ) + SQR(  mvY ) ) ;
        MV_LengthdXY     = sqrt( SQR( mvdX ) + SQR( mvdY ) ) ;
        S->MV_Length    += MV_LengthXY ;            // Add up of motion vector length for mean value
        S->MV_dLength   += MV_LengthdXY ;            // Add up of motion vector length for mean value
        S->MV_SumSQR    += SQR( MV_LengthXY ) ; //Add up square of motion vector length for variance value
        S->MV_DifSumSQR += SQR( MV_LengthdXY ) ;
        S->MV_LengthX   += mvX ;
        S->MV_LengthY   += mvY ;
        S->MV_XSQR      += SQR( mvX ) ;
        S->MV_YSQR      += SQR( mvY ) ;
        } ;
      } ;
  }

/**************************************************************************************************************/

int    SHAPE_FACTOR[8] = { 1,2,2,4,2,2,2,2 } ;
double RES_FACTOR[4]   = {1.0, 1.0, 2.0, 4.0 } ;
double NORM_FACTOR[6]  = { 2.0, 1.41, 1.0, 1.41, 2.0, 4.0 } ;
#define  FC    2.0

void ShapeStatisticsHEVC( VIDEO_STAT* FrmStat, int PU_Stat[7][6], int NumBlk )
  {
  int    i, j, k, weight ;
  int    TU4x4 = 0 ;
  double SS[5] = { 0 }, *S = FrmStat->TrShapes, CodedArea ;
  double *T = FrmStat->BlkShapes, Sum ;
  //double  Sc= 5.0 - (15.0 * min(0.21, max(FrmStat->Seq.BitsPertPel, 0.01)) + 0.85) ;   // BitPerPel from 0.01 ...0.21   to   4...1
  double  Sc = 1.;//5.0 - (15.0 * min(0.21, max((double)FrmStat->FrameSize/(NumBlk*2), 0.01)) + 0.85) ;   // BitPerPel from 0.01 ...0.21   to   4...1


  for( j=0 ; j<5 ; j++ )           //   0:4x4   1:8x8   2: 16x16   3: 32x32   4: 64x64...
    {
    k = (j << 1) ;
    FrmStat->BlkShapes[k] = PU_Stat[j +2][0] ;

    if( j > 0 )
      for( FrmStat->BlkShapes[k-1]=0, i=1 ; i<6 ; i++ )
        FrmStat->BlkShapes[k-1 - (int)(i==3)] += PU_Stat[j+2][i] * SHAPE_FACTOR[i]  ;
    } ;

  for( i=0 ; i<5 ; i++ )
    {
    weight = (i == 4) ? 0 : (1 << (3 - i)) ;
    FrmStat->BlkShapes[9] += 1.5*weight*FrmStat->BlkShapes[i << 1] + weight*FrmStat->BlkShapes[(i << 1) + 1] ;
    } ;
  FrmStat->BlkShapes[9] /= NumBlk ;

  for( i=1 ; i<5 ; i++ )
    {
    weight = (i==4)? 0 : (1 << (3 - i)) ;
    FrmStat->TrShapes[6] += weight*FrmStat->TrShapes[i] ;
    } ;
  FrmStat->TrShapes[6] /= NumBlk ;

  CodedArea = (64.0*S[5] + 16*S[4] + 4*S[3] + S[2]) / (double)NumBlk ;
  for( i=2,Sum=0 ; i<6 ; Sum+=S[i++] );
  for( i = 0 ; i < 4 ; SS[i++] = S[i + 2] * NORM_FACTOR[i+ 3-FrmStat->Seq.Resolution] ) ;
  for( j = 0 ; j < 4 ; j++ )
    SS[j] = SS[j] * RES_FACTOR[FrmStat->Seq.Resolution] ;// *Sum / NumBlk ;
  }
 
/***************************************************************************************************/

void InitStatisticsHEVC( HEVCContext* s )
  {
  VIDEO_STAT*    FrmStat = &(s->frame->FrmStat) ;
  static int     PrevPOC=0, POC_Dif=8 ;
  static int64_t PrevPTS ;
  int            PTS_Dif ;

  SAFE_FREE( FrmStat->S.NormalizedField[0] ) ;
  SAFE_FREE( FrmStat->S.NormalizedField[1] ) ;

  memset( FrmStat, 0, sizeof( VIDEO_STAT ) ) ;

  int FieldSize = (s->ps.sps->width * s->ps.sps->height) >> 4 ;

  FrmStat->min_QP      = 1000.0 ;
  FrmStat->InitialQP   = s->sh.slice_qp ;
  FrmStat->BlackBorder = CurrBlackBorder ;
  FrmStat->CurrPOC     = s->poc ;
  FrmStat->IsIDR       = IS_IDR(s) ; 

  if( abs( FrmStat->CurrPOC - PrevPOC ) != 0 )
    {
    if( (s->frame->pts == 0) || (s->frame->pkt_duration == 0) )
      POC_Dif = min( POC_Dif, abs( FrmStat->CurrPOC - PrevPOC ) ) ;
    else
      {
      PTS_Dif = max( 1, nearbyint( fabs( (double)s->frame->pts - PrevPTS ) / (double)s->frame->pkt_duration)) ;
      POC_Dif = abs( FrmStat->CurrPOC - PrevPOC ) / PTS_Dif ;
      } ;
    } 
  FrmStat->POC_DIF     = POC_Dif ;
  PrevPOC              = FrmStat->CurrPOC ;
  PrevPTS              = s->frame->pts ;

  FrmStat->S.NormalizedField[0] = calloc( FieldSize, sizeof( float ) ) ;
  FrmStat->S.NormalizedField[1] = calloc( FieldSize, sizeof( float ) )  ;
  }

/***************************************************************************************************/

void CoefStatisticsHEVC( HEVCContext* s, int log_size, int16_t* coeffs, int c_idx )
  {
  int       size = 1 << log_size ;
  int       i ;

  if( !c_idx )
    {
    for( i = 0 ; i < size*size ; i++ )
      {
      s->frame->FrmStat.S.AverageCoefs    [log_size - 2][i] += (double)abs( coeffs[i] ) ;
      s->frame->FrmStat.S.AverageCoefsSQR [log_size - 2][i] += (double)coeffs[i] * (double)coeffs[i] ;
      s->frame->FrmStat.S.AverageCoefsBlkCnt[log_size-2][i] += (double)(coeffs[i] != 0) ;
      } ;
    } ;
  }


/***************************************************************************************************/
/***********called for every block of log_cb_size **************************************************/

void MbStatistcsHEVC( HEVCContext* s, HEVCLocalContext* lc, int y0, int x0, int log_cb_size )
  {
  HEVCSPS*     sps = (HEVCSPS*)s->ps.sps ;
  VIDEO_STAT*  FrmStat = &(s->frame->FrmStat) ;
  MvField*     MvF ;
  float        *NormField[2] ;

  int          FrameType = s->frame->pict_type ;
  int          i, j, x_pu, y_pu, Type, NoBorder ;
  int          qp_length, MBs ;

//#if defined _DEBUG
//  {
//  if( (x0 >= 128 ) || (y0 >= 128) )
//  return ;
//  }
//#endif

  EMMS() ;
  if( Opened )
    {
    qp_length = 1 << (log_cb_size - sps->log2_min_cb_size) ;
    x_pu = x0 >> sps->log2_min_pu_size ;
    y_pu = y0 >> sps->log2_min_pu_size ;
    for( j = 0 ; j < qp_length ; j++ )
      for( i = 0 ; i < qp_length ; i++ )
        {
        MvF          = s->ref->tab_mvf      + (y_pu + j)*sps->min_pu_width + x_pu + i ;
        NormField[0] = FrmStat->S.NormalizedField[0] + (y_pu + j)*sps->min_pu_width + x_pu + i ;
        NormField[1] = FrmStat->S.NormalizedField[1] + (y_pu + j)*sps->min_pu_width + x_pu + i ;

        NoBorder  = (y0 >= FrmStat->BlackBorder) && ((sps->height - y0) > FrmStat->BlackBorder) ;
        FrmStat->MbTypes[Type = GetCodingTypeHEVC( lc, MvF, FrameType )]++ ;   // there is only one Type and QP within the CB, but we always count on min_cb_size base
        if( ((Type != SKIPPED) && (Type != DIRECT)) || ((x_pu + y_pu) == 0) )
          QPStatistics( FrmStat, lc->qp_y, Type, NoBorder ) ;

        if( (FrameType != AV_PICTURE_TYPE_I) )
          {
          MBs = 1 << (sps->log2_min_cb_size - sps->log2_min_pu_size) ;
          if( (Type != SKIPPED) && (Type < INTRA_BLK) )                // Not skipped includes merge_mode with non coded motion vectors
            MV_StatisticsHEVC( s, FrmStat, &FrmStat->S, Type, MvF, NormField, sps->min_pu_width, MBs ) ;
          else if(Type == SKIPPED)
            FrmStat->S.NumBlksSkip  += MBs * MBs ;
          else 
            FrmStat->S.NumBlksIntra += MBs * MBs ;
          } ;
        } ;
    FrmStat->S.MbCnt++ ;
    } ;
  }

/***************************************************************************************************/
/*********** SKIP=0   L0=1    L1=2     Bi=3   Dir:4    INTRA(direct)=5   INTRA(plane)=6  ***********/
/*********** pred_mode: 0: INTER   1: INTRA    2: SKIPP ********************************************/
/*********** pred_flag: 0: Non/INTRA   1: L0   2: L1   3: BI   *************************************/

int GetCodingTypeHEVC( HEVCLocalContext* lc, MvField* MvFld, int FrameType )
  {
  if( lc->cu.pred_mode == MODE_INTRA )
    return(5 + (lc->tu.intra_pred_mode < 2)) ;   // < 2 is DC prediction or planar mode. The Rest is directional modes
  else if( lc->cu.pred_mode == MODE_SKIP )
    return((FrameType == AV_PICTURE_TYPE_B)? 4 : 0) ;
  else
    return( ((FrameType == AV_PICTURE_TYPE_B) && (MvFld->pred_flag == 0))? 4 : MvFld->pred_flag ) ;
  }

/*****************************************************************************************************************/

int BlackBorderEstimationHEVC( uint8_t* cbf_luma, HEVCSPS* sps, int FrameType, int CurrBlackBorder )
  {
  int      tuy, tux, BB ;
  int*     BlackLine ;

  if( FrameType == AV_PICTURE_TYPE_I )
    {
    BlackLine = calloc( sps->min_tb_width, sizeof( int ) ) ;
    for( tuy = 0 ; tuy < sps->min_tb_height ; tuy++ )
      for( tux = 0 ; tux < sps->min_tb_width ; tux++ )
        if( ((tuy + tux) > 0) && (cbf_luma[tuy*sps->min_tb_width + tux] == 0) )
          BlackLine[tuy]++ ;

    BB = BlackborderDetect( BlackLine, sps->min_tb_height, (int)(0.72*sps->min_tb_width), sps->log2_min_tb_size) ;
    free( BlackLine ) ;
    return(BB) ;
    }

  return CurrBlackBorder;
  }

