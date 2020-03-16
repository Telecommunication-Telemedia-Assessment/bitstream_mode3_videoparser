#include "stdafx.h"
#include "VideoStat.h"
#include "VideoStatCommon.h"
#include "libavcodec/avcodec.h"


/******************************************************************************/
/*********************** Common for all Codecs ********************************/
/******************************************************************************/

// Count consecutive zero blocklines with >threshold zero-coeff blocks
int BlackborderDetect( int* BlackLine, int rows, int threshold, int logBlkSize )
  {
  int BlackLines = 0, i;

  for( i = 0 ; i < rows>>1 ; i++ )
    BlackLine[i] = BlackLine[rows-i-1] = ((BlackLine[i] + BlackLine[rows-i-1]) >= (threshold << 1)) ? 1 : 0 ;

  for( i = 0 ; i < rows ; i++ )
    {
    if( BlackLine[i] != 0 )
      BlackLines++ ;
    else
      break ;
    } ;

  if( BlackLines >= (rows >> 1) )
    BlackLines = 0 ;

  return( BlackLines << logBlkSize ) ;
  }

/******************************************************************************/

// Adds QP of a single block to the QP-Sum
// - Before this method InitialQP, min_QP, max_QP need to be initialized
// - MbQP has to be counted seperately
void QPStatistics( VIDEO_STAT* FrmStat, int CurrQP, int CurrType, int NoBorder )
  {
  FRAME_SUMS* S = &(FrmStat->S) ;

  FrmStat->min_QP = min( CurrQP, FrmStat->min_QP );
  FrmStat->max_QP = max( CurrQP, FrmStat->max_QP );
  FrmStat->MbQPs[CurrType] += CurrQP ;

  S->QpSum += CurrQP ;                // add up QP-value of Mb for mean QP
  S->QpSumSQ += SQR( CurrQP ) ;
  S->QpCnt++ ;

  if( NoBorder )
    {
    S->QpSumBB += CurrQP ;            // add up QP-value of Mb for mean QP without black border
    S->QpSumSQBB += SQR( CurrQP ) ;
    S->QpCntBB++ ;
    } ;
  }

/******************************************************************************/



// Formats AvCoefs into 2D-Array, Calculates Coef-StdDev, Calculates HF-LF ratio
//   maxSize    max log2 tx-blocksize
//   frameType  0: I, 1: -, 2: P, 3: B
void CoefsStatistics( VIDEO_STAT* FrmStat, double AvCoefs[5][1024], double AvCoefsSQR[5][1024], int AvCoefsCnt[5][1024], double TrShapes[8], int maxSize, int frameType )
  {
  int      i, row, col, size, offset, HfLf, HfLf2, len, sqLen ;
  double   CoefCnt[2][6], Factor1, Factor2, Factor3, Cnt, QP,QP_Max  ;


  for( size = 0 ; size < maxSize ; size++ )
    {
    len   = 4 << size ;
    sqLen = len * len ;
    memset( CoefCnt, 0, sizeof( CoefCnt ) ) ;
    if( TrShapes[size + 2] )
      {
      Cnt  = TrShapes[size + 2] ;
      Cnt *= (FrmStat->S.QpCntBB != 0) ? FrmStat->S.QpSumBB / (double)FrmStat->S.QpCntBB : 1.0 ;
      for( row = 0 ; row < (4 << size) ; row++ ) // compute the averages for 4x4 blocks
        {
        offset = row * (4 << size) ;
        for( col = 0 ; col < (4 << size) ; col++ )
          {
          //Cnt   *= ( AvCoefsCnt[size][offset + col] )? (double)(AvCoefsCnt[size][offset + col]) : 1.0 ;

          HfLf = ((row + col) == 0) ? 2 : ((int)(col > ( len - row - 1 ))) ;
          Factor1 = (row << (3 - size)) + (col << (3 - size))  ;
          Factor2 = Factor1*Factor1 ;
          Factor3 = exp( Factor1/8.0 ) / 30.0 ;
          HfLf2 = ((row + col) == 0) ? 2 : ((int)(col + row >= (len >> 1))) ;

          FrmStat->Av_Coefs[size][row][col] = AvCoefs[size][offset + col] / Cnt ;
          FrmStat->StdDev_Coefs[size][row][col] = sqrt( AvCoefsSQR[size][offset + col] / TrShapes[size + 2] - SQR( AvCoefs[size][offset + col] / Cnt ) ) ;

          FrmStat->Blockiness[0][size] += FrmStat->Av_Coefs[size][row][col] * Factor1 ;
          FrmStat->Blockiness[1][size] += FrmStat->Av_Coefs[size][row][col] * Factor2 ;
          FrmStat->Blockiness[2][size] += FrmStat->Av_Coefs[size][row][col] * Factor3 ;
          FrmStat->HF_LF_ratio[0][size][HfLf]  += FrmStat->Av_Coefs[size][row][col] ;
          FrmStat->HF_LF_ratio[1][size][HfLf2] += FrmStat->Av_Coefs[size][row][col] ;
          CoefCnt[0][HfLf]  += (int)(FrmStat->Av_Coefs[size][row][col] != 0) ;
          CoefCnt[1][HfLf2] += (int)(FrmStat->Av_Coefs[size][row][col] != 0) ;
          } ;
        } ;
      } ;

    for( int i = 0 ; i < 3 ; i++ )
      {
      if( CoefCnt[0][i] )
        FrmStat->HF_LF_ratio[0][size][i] /= CoefCnt[0][i] ;  // LF, HF, DC
      if( CoefCnt[1][i] )
        FrmStat->HF_LF_ratio[1][size][i] /= CoefCnt[1][i] ;  // LF, HF, DC
      } ;

    if( (CoefCnt[0][0] + CoefCnt[0][1] + CoefCnt[0][2]) > 0 )
      {
      FrmStat->Blockiness[0][size] = ( FrmStat->Blockiness[0][size] * (CoefCnt[0][0] + CoefCnt[0][1] + CoefCnt[0][2])) /  sqLen ;
      FrmStat->Blockiness[1][size] = ( FrmStat->Blockiness[1][size] * (CoefCnt[0][0] + CoefCnt[0][1] + CoefCnt[0][2])) / (sqLen*sqLen)  ; 
      FrmStat->Blockiness[2][size] = ( FrmStat->Blockiness[2][size] * (CoefCnt[0][0] + CoefCnt[0][1] + CoefCnt[0][2])) / (sqLen*sqLen)  ; 
      } ;
    } ;

  for( i=0 ; i<3 ; i++ )
    {
    FrmStat->Blockiness[i][5] = ( FrmStat->Blockiness[i][0]*TrShapes[2] +  4.0*FrmStat->Blockiness[i][1]*TrShapes[3] 
                           + 16.0*FrmStat->Blockiness[i][2]*TrShapes[4] + 64.0*FrmStat->Blockiness[i][3]*TrShapes[5])
                           / (double)(TrShapes[0] + TrShapes[2] + 4*TrShapes[3] + 16*TrShapes[4] + 64*TrShapes[5]) ;


    QP     = (FrmStat->S.QpCntBB != 0) ? FrmStat->S.QpSumBB / (double)FrmStat->S.QpCntBB : FrmStat->InitialQP ;
    QP_Max = (FrmStat->Seq.Codec == CODEC_VP9) ? 255.0 : ((FrmStat->Seq.BitDepth > 8)? 63 : 51) ;

    FrmStat->Blockiness[i][4] = 1.0 / ((1.0 + FrmStat->Blockiness[i][5]) * exp( 5.0*((QP_Max - QP)/QP_Max) )) ;
    } ;

  }

/******************************************************************************/

#define SizeMultiplier    1.5 

// Calculates Avg and StdDeviation values for QP and MV

//                        ---               H.264             H.265             VP9
double TI_NORM[4][3] = { {0.0, 0.0, 0.0},  {0.03, 1.0, 1.0},  {0.1, 1.6, 1.0},  {0.2, 1.0, 0.4 } } ;
VIDEO_STAT PrevStat[2] ;

void FinishStatistics( VIDEO_STAT* FrmStat, int FrameType, int NumPel, int NumBlk )
  {
  int         i, NumMotion, NumDifs, FieldSize ;
  FRAME_SUMS* S=&FrmStat->S;
  double      TI_Curr, BpF ;
  //double      a1, b1, c1, d1, QP, QP_Sqr ;
  //double      Av_Motion, Av_MotionX, Av_MotionY, Av_MotionDif;


  FrmStat->AnalyzedMBs  = S->MbCnt ;
  FrmStat->NumBlksIntra = S->NumBlksIntra ;
  FrmStat->NumBlksMv    = S->NumBlksMv ;
  FrmStat->NumBlksSkip  = S->NumBlksSkip ;
  FrmStat->BitCntCoefs  = S->BitCntCoefs ;
  FrmStat->BitCntMotion = S->BitCntMotion ;
  FrmStat->CodedMv      = S->CodedMv ;

  if (S->QpCnt != 0) 
    {
    FrmStat->Av_QP       = S->QpSum   / (double)S->QpCnt ;
    FrmStat->Av_QPBB     = S->QpSumBB / (double)S->QpCntBB ;
    FrmStat->StdDev_QP   = sqrt( S->QpSumSQ   / (double)S->QpCnt   - SQR( S->QpSum   / (double)S->QpCnt ) ) ;
    FrmStat->StdDev_QPBB = sqrt( S->QpSumSQBB / (double)S->QpCntBB - SQR( S->QpSumBB / (double)S->QpCntBB ) ) ;
    } ;

  BpF    = (20.0*(double)FrmStat->FrameSize / (double)NumPel) ;
  //FrmStat->SpatialComplexety[1] = BpF * exp( 0.115524*FrmStat->Av_QPBB ) * pow( SizeMultiplier, (FrmStat->Seq.Resolution-1) ) ; 
  FrmStat->SpatialComplexety[0] = BpF * exp( 0.115524*FrmStat->Av_QPBB ) ;
  FrmStat->SpatialComplexety[1] = log( FrmStat->SpatialComplexety[0] ) ;

  if( (FrameType != AV_PICTURE_TYPE_I) )
    {
    NumMotion = S->NumBlksMv ;// +S->NumBlksSkip ; //+ ((FrmStat->Seq.Codec == CODEC_VP9)? 0 : S->NumBlksSkip) ;
    NumDifs  =  (FrmStat->Seq.Codec == CODEC_VP9) ? ((S->CodedMv > 0)? S->CodedMv:1) : NumMotion ;
    FrmStat->PredFrm_Intra = S->NumBlksIntra / (double)NumBlk ;
    FrmStat->SKIP_mb_ratio = S->NumBlksSkip  / (double)NumBlk ;

    NumDifs = NumMotion ;
    if( NumMotion > 0 ) 
      {
      FrmStat->Av_Motion        = S->MV_Length  / NumMotion ;
      FrmStat->Av_MotionDif     = S->MV_dLength / NumDifs ;
      FrmStat->Av_MotionX       = S->MV_LengthX / NumMotion ;
      FrmStat->Av_MotionY       = S->MV_LengthY / NumMotion ;
      FrmStat->StdDev_MotionX   = sqrt( 0.00001 + S->MV_XSQR      / NumMotion  - SQR( S->MV_LengthX / NumMotion ) ) ;
      FrmStat->StdDev_MotionY   = sqrt( 0.00001 + S->MV_YSQR      / NumMotion  - SQR( S->MV_LengthY / NumMotion ) ) ;
      FrmStat->StdDev_Motion    = sqrt( 0.00001 + S->MV_SumSQR    / NumMotion  - SQR( S->MV_Length  / NumMotion ) ) ;
      FrmStat->StdDev_MotionDif = sqrt( 0.00001 + S->MV_DifSumSQR / NumDifs    - SQR( S->MV_DifSum  / NumDifs) ) ;

      TI_Curr = log( TI_NORM[FrmStat->Seq.Codec][0] * FrmStat->Av_Motion + TI_NORM[FrmStat->Seq.Codec][1] ) * NumMotion * TI_NORM[FrmStat->Seq.Codec][2] * FrmStat->Av_MotionDif / NumBlk ;
      FrmStat->TemporalComplexety[1] = FrmStat->TemporalComplexety[2] = (5.0*TI_Curr + 2 * PrevStat[0].TemporalComplexety[1] + PrevStat[1].TemporalComplexety[1]) / 8.0 ;

      //if( (FrmStat->Seq.Codec == CODEC_VP9) && FrmStat->IsShrtFrame )  // motion for VP9 "short Frames" can not be measured therefore it is estimated
      //  {
      //  Av_Motion    = (2*PrevStat[0].Av_Motion    + PrevStat[1].Av_Motion) / 3.0 ;
      //  Av_MotionDif = (2*PrevStat[0].Av_MotionDif + PrevStat[1].Av_MotionDif) / 3.0 ;
      //  Av_MotionX   = (2*PrevStat[0].Av_MotionX   + PrevStat[1].Av_MotionX) / 3.0 ;
      //  Av_MotionY   = (2*PrevStat[0].Av_MotionY   + PrevStat[1].Av_MotionY) / 3.0 ;
      //  TI_Curr      = log( TI_NORM[CODEC_VP9][0]*Av_Motion + TI_NORM[CODEC_VP9][1]) * NumMotion * TI_NORM[CODEC_VP9][2]*Av_MotionDif / NumBlk ;
      //  FrmStat->TemporalComplexety[2]  = (5.0*TI_Curr + 2*PrevStat[0].TemporalComplexety[2] + PrevStat[1].TemporalComplexety[2]) / 8.0 ; 
      //  } ;

      memcpy( (void*)(&PrevStat[1]), (void*)(&PrevStat[0]), sizeof( VIDEO_STAT ) ) ;
      memcpy( (void*)(&PrevStat[0]), (void*)FrmStat,        sizeof( VIDEO_STAT ) ) ;
      } ;
    } ;
  for( i = 0 ; i < 7 ; i++ )
    {
    if( FrmStat->MbTypes[i] )
      FrmStat->MbQPs[i] = FrmStat->MbQPs[i]*4.0 / FrmStat->MbTypes[i] ;
    }  ;

  FieldSize = ((FrmStat->Seq.FrameHeight * FrmStat->Seq.FrameWidth) >> 4) ;
  //for( i=0 ; i<2 ; i++ )
  //  memcpy( FrmStat->MotionField[i], FrmStat->S.NormalizedField[i], FieldSize*sizeof( float )) ;

  FrmStat->FrameType     = FrameType ;
  }

/**************************************************************************************/

const int     NUM_PIX[4] = {101376, 414720, 2073600, 8294400} ;                // number of pixel in CIF / SD / 1080p /UHD

int GetResolution( int NumPix )
  {
  int   Dist, Res, i ;

  for( i=0, Dist=0x7fffffff ; i<4 ; i++ )
    if( abs( NumPix - NUM_PIX[i] ) < Dist )
      {
      Res  = i ;
      Dist = abs( NumPix - NUM_PIX[i] ) ;
      } ;
  return( Res ) ;
  }

/*******************************************************************************************************/

static uint8_t *DstData[2][4] = { NULL } ;
static int      DstLinesize[2][4], Curr=0 ;
 
void Sobel( int* dst, uint8_t* src, int width, int height )
  {
  int        i, j ;
  int        ValH, ValV, ValS ;
  uint8_t*   Pel ;

  for( i=1; i<height-1; i++) 
    {
    for( j=1; j<width-1; j++) 
      {
      Pel   =  src + (i-1) * width + j ;
      ValH  =  (int)(Pel[-1] + 2.0*Pel[0] + Pel[1]) ;                 // obere Zeile Sobel
      Pel  +=  width << 1 ;
      ValH += (int)(-Pel[-1] - 2.0*Pel[0] - Pel[1]) ;                 // untere Zeile Sobel

      Pel   = (src + (i-1) * width + width + j-1) ;
      ValV  = (int)( Pel[ -width] + 2.0*Pel[0] + Pel[  width]) ;      // linke Spalte Sobel
      ValV += (int)(-Pel[2-width] - 2.0*Pel[2] - Pel[2+width]) ;      // rechte Spalte Sobel

      ValS              = (int)(0.5 + sqrt( SQR( ValH ) + SQR( ValH ) )) ;
      dst[i*width + j]  = ICLIP( SobelMax, SobelMin, ValS ) ;
      }
    } ;
  }

/********************************************************************************************/


int GroundTruth_P910( AVFrame* frame, VIDEO_STAT* FrmStat, int width, int height, int pix_fmt )
  {
  int           BuffSize, i, LumSize ;
  int           Diff, Lum, LumPr, Sob ;
  double        SumSI, SqrSumSI, SumTI, SqrSumTI  ;
  int*          SBuf=NULL ;

  LumSize = width * height ;
  if( DstData[Curr][0] == NULL )
    if( (BuffSize =av_image_alloc( DstData[Curr], DstLinesize, width, height, pix_fmt, 1 )) < 0 )
      ErrorReturn( "Could not allocate raw video buffer\n", NULL, 0 );

  SBuf = calloc( LumSize, sizeof( int ) ) ;
  av_image_copy( DstData[Curr], DstLinesize, (const uint8_t **)(frame->data), frame->linesize, pix_fmt, width, height);
  Sobel( SBuf, DstData[Curr][0], width, height ) ;

  FrmStat->TI_910 = FrmStat->SI_910 = FrmStat->TI_Mot[0] = FrmStat->TI_Mot[1] = 0.0 ;
  SumTI    = SumSI    = 0.0 ;
  SqrSumTI = SqrSumSI = 0.0 ;
  for( i = 0 ; i < LumSize ; i++ )
    {
    Lum       = DstData[Curr][0][i] ;
    Sob       = SBuf[i] ;
    LumPr     = (FrmStat->FrameIdx > 1)? DstData[1 - Curr][0][i] : 0 ;
    Diff      = Lum - LumPr ;
    SumTI    += Diff ;
    SqrSumTI += Diff*Diff ;
    SumSI    += Sob ;
    SqrSumSI += Sob*Sob ;
    } ;

  FrmStat->SI_910 = sqrt( 0.000001 + SqrSumSI / LumSize - SQR( SumSI / LumSize ) ) ;
  if( (FrmStat->FrameIdx > 1) && (frame->pict_type != AV_PICTURE_TYPE_I) )
    FrmStat->TI_910 = sqrt( 0.000001 + SqrSumTI / LumSize - SQR( SumTI / LumSize ) ) ;

  free( SBuf ) ;
  Curr = 1 - Curr ;
  return(1) ;
  }

/*****************************************************************************************************************/

double GroundTruthTI( float* NF[2], int width, int height, double FilteredOutput[2] )

  {
  int      i, j ;
  double   ValxhS, ValyhS, ValxvS, ValyvS, ValS, ValXS, ValYS ;
  double   ValxhL, ValyhL, ValxvL, ValyvL, ValL, ValXL, ValYL ;
  int      NumBlk  = width * height ;
  double   SobelSum = 0.0, LaplaceSum=0.0 ;
  float    *MvX, *MvY ;

  for( i=1; i<height-1; i++) 
    {
    for( j=1; j<width-1; j++) 
      {
      MvX     =  NF[0] + (i-1) * width + j ;
      MvY     =  NF[1] + (i-1) * width + j ;
      ValxhS  =  MvX[-1] + 2.0*MvX[0] + MvX[1] ;                // obere Zeile Sobel
      ValyhS  =  MvY[-1] + 2.0*MvY[0] + MvY[1] ; 
      ValxhL  =  MvX[-1] +     MvX[0] + MvX[1] ;                // obere Zeile Laplace
      ValyhL  =  MvY[-1] +     MvY[0] + MvY[1] ;

      MvX    +=  width ;
      MvY    +=  width ;
      ValxhL +=  MvX[-1] - 8.0*MvX[0] + MvX[1] ;                 // mittlere Zeile Laplace
      ValyhL +=  MvY[-1] + 8.0*MvY[0] + MvY[1] ;

      MvX    +=  width ;
      MvY    +=  width ;
      ValxhS += -MvX[-1] - 2.0*MvX[0] - MvX[1] ;                 // untere Zeile Sobel
      ValyhS += -MvY[-1] - 2.0*MvY[0] - MvY[1] ;
      ValxhL +=  MvX[-1] +     MvX[0] + MvX[1] ;                 // obere Zeile Laplace
      ValyhL +=  MvY[-1] +     MvY[0] + MvY[1] ;

      MvX     = NF[0] + (i-1) * width + width + j-1 ;
      MvY     = NF[1] + (i-1) * width + width + j-1 ;
      ValxvS  =  MvX[-width] + 2.0*MvX[0] + MvX[width] ;         // linke Spalte Sobel
      ValyvS  =  MvY[-width] + 2.0*MvY[0] + MvY[width] ;
      ValxvL  =  MvX[-width] + MvX[0]     + MvX[width] ;         // linke Spalte Laplace
      ValyvL  =  MvY[-width] + MvY[0]     + MvY[width] ;

      MvX++ ;
      MvY++ ;
      ValxvL +=  MvX[-width] + 8.0*MvX[0] + MvX[width] ;         // mittlere Spalte Laplace
      ValyvL +=  MvY[-width] + 8.0*MvY[0] + MvY[width] ;

      MvX++ ;
      MvY++ ;
      ValxvS += -MvX[-width] - 2.0*MvX[0] - MvX[width] ;         // rechte Spalte Sobel
      ValyvS += -MvY[-width] - 2.0*MvY[0] - MvY[width] ;
      ValxvL +=  MvX[-width] + MvX[0]     + MvX[width] ;         // linke Spalte Laplace
      ValyvL +=  MvY[-width] + MvY[0]     + MvY[width] ;

      ValXS       = sqrt( SQR( ValxvS ) + SQR( ValxhS ) ) ;
      ValYS       = sqrt( SQR( ValyvS ) + SQR( ValyhS ) ) ;
      ValS        = sqrt( SQR( ValXS  ) + SQR( ValYS  ) ) ;
      SobelSum   += ICLIP( SobelMax, SobelMin, ValS ) ;

      ValXL       = sqrt( SQR( ValxvL ) + SQR( ValxhL ) ) ;
      ValYL       = sqrt( SQR( ValyvL ) + SQR( ValyhL ) ) ;
      ValL        = sqrt( SQR( ValXL  ) + SQR( ValYL  ) ) ;
      LaplaceSum += ICLIP( SobelMax, SobelMin, ValL ) ;
      }
    } ;

  FilteredOutput[0] = SobelSum   / NumBlk ;
  FilteredOutput[1] = LaplaceSum / (3.0*NumBlk) ;

  /* // SG: deactivated due to problems with some VP9 sequences that break here with a invalid pointer error
  SAFE_FREE( NF[0] ) ;
  SAFE_FREE( NF[1] ) ;
  */
  return( FilteredOutput[0] ) ;
  }