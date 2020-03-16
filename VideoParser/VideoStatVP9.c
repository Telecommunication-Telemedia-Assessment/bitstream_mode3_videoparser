#include "stdafx.h"
#include "VideoStat.h"
#include "VideoStatCommon.h"
#include "libavcodec/vp9common.h"
#include "libavcodec/vp9.h"

extern int DebugCnt, Tmp=0 ;
VIDEO_STAT VP9Hidden_FrmStat ;

extern const uint8_t ff_vp56_norm_shift[256];

#if !defined WIN32
// only include if you build under linux, otherwise you will get a linking error
const uint8_t ff_vp56_norm_shift[256]= {
	 8,7,6,6,5,5,5,5,4,4,4,4,4,4,4,4,
	 3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
	 2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
	 2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
	 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
};
#endif

static void FrameQPStatsVP9( VIDEO_STAT* FrmStat, VP9Context* ctx ) ;

// Table for Blocksize-lookup
static const uint8_t bwh_tab[2][N_BS_SIZES][2] = {
		{
				{ 16, 16 }, { 16, 8 }, { 8, 16 }, { 8, 8 }, { 8, 4 }, { 4, 8 },
				{ 4, 4 }, { 4, 2 }, { 2, 4 }, { 2, 2 }, { 2, 1 }, { 1, 2 }, { 1, 1 },
		}, {
				{ 8, 8 }, { 8, 4 }, { 4, 8 }, { 4, 4 }, { 4, 2 }, { 2, 4 },
				{ 2, 2 }, { 2, 1 }, { 1, 2 }, { 1, 1 }, { 1, 1 }, { 1, 1 }, { 1, 1 },
		}
};


/******************************************************************************/

int GetVideoStatisticsVP9( AVCodecContext* av_ctx, AVFrame* frame, VIDEO_STAT* FrameStat )
  {
  VP9Context* ctx = (VP9Context*)av_ctx->priv_data ;
  VIDEO_STAT* FrmStat = &(frame->FrmStat) ;
  FRAME_SUMS* S       = &FrmStat->S;
  int i ;

  if( !Started && (frame->pict_type != AV_PICTURE_TYPE_I) )
    return(0) ;
  Started = 1 ;

  if( FrmStat->Seq.BitDepth == 0 )
    {
    FrmStat->Seq.ChromaFormat = ((av_ctx->pix_fmt >= 33)? 4 : ((av_ctx->pix_fmt == 0)? 1 : ((av_ctx->pix_fmt == 4)? 2 : 3))) ;
    FrmStat->Seq.BitDepth     = ctx->bpp ;
    FrmStat->Seq.Profile      = av_ctx->profile ;
    FrmStat->Seq.Level        = av_ctx->level ;
    FrmStat->Seq.FrameWidth   = frame->width  ;
    FrmStat->Seq.FrameHeight  = frame->height ;
    FrmStat->Seq.ArithmCoding = 1 ;
    FrmStat->Seq.Codec        = 3 ;
    }

  FrmStat->BlackBorder        = CurrBlackBorder ;
  if( !FrmStat->IsShrtFrame )
    FrmStat->FrameSize        = frame->pkt_size ;
  FrmStat->FrameIdx           = FrameStat->FrameIdx ;

	// Collect stat results from VP9Context
  FrmStat->IsIDR     = ctx->s.h.keyframe ;
  FrmStat->FrameType = ctx->s.h.keyframe ? 1 : 2 ;
  GroundTruth_P910( frame, FrmStat, frame->width, frame->height, ctx->pix_fmt ) ;

  if( (FrameStat->FrameIdx > 1) && (FrmStat->FrameType != AV_PICTURE_TYPE_I) && (!FrmStat->IsShrtFrame)) {
    // SG: we observed problems with some videos where the second P frame has a framesize of 0
    // checking the code, than this kind of frame is a !FrmStat->IsShrtFrame
    // for those frames, there are no allocated S.NormalizedField values
    GroundTruthTI( FrmStat->S.NormalizedField, ctx->w>>2, ctx->h>>2, FrmStat->TI_Mot )  ;
  }


	// Calculate Averages
	FrameQPStatsVP9( FrmStat, ctx );
	CoefsStatistics( FrmStat, FrmStat->S.AverageCoefs, FrmStat->S.AverageCoefsSQR,  FrmStat->S.AverageCoefsBlkCnt, FrmStat->TrShapes, 4, ctx->s.h.keyframe ? 2 : 0 ) ;
	FinishStatistics( FrmStat, ctx->s.h.keyframe ? 1 : 2, frame->width*frame->height, frame->width*frame->height/16 ) ;
  memcpy( FrameStat, FrmStat, sizeof( VIDEO_STAT ) ) ;

	return(1) ;
	}

/******************************************************************************/

// Calculates QP Statistics
// for VP9 QP are only set on frame and optionally segment-level
static void FrameQPStatsVP9( VIDEO_STAT* FrmStat, VP9Context* ctx )
	{
	int defaultQP = ctx->s.h.yac_qi; // y-ac qp, (y-dc qp and uv qp are disregarded)

	FrmStat->min_QP = 255 ; FrmStat->max_QP = 0 ;
	FrmStat->InitialQP = FrmStat->Av_QP = FrmStat->Av_QPBB = defaultQP ;

	// Check whether segmentation is used
	if( ctx->s.h.segmentation.enabled )
		{
		uint8_t *segmap = ctx->s.frames[CUR_FRAME].segmentation_map ;
		uint32_t col, row ;

		// Iterate over 8x8 blocks
		for (row = 0; row < ctx->rows; row++ )
			{
			for (col = 0; col < ctx->cols; col++ )
				{
				int segment = segmap[row * 8 * ctx->sb_cols + col] ;
				int currQP = defaultQP ;
				int NoBorder = ((row << 3) >= FrmStat->BlackBorder) && ((row << 3) < (ctx->h - FrmStat->BlackBorder)) ;

				// Use segment QP if available
				if( ctx->s.h.segmentation.feat[segment].q_enabled )
					{
					if( ctx->s.h.segmentation.absolute_vals )
						currQP = ctx->s.h.segmentation.feat[segment].q_val ;
					else
						currQP += ctx->s.h.segmentation.feat[segment].q_val ;
					}

				QPStatistics(FrmStat, currQP, 0, NoBorder) ;
				}
			}
		}
	else
		{
		QPStatistics(FrmStat, defaultQP, 0, 1) ;
		}
	}

/******************************************************************************/

// Calculates the Y-AC QP of the segment a VP9Block belongs to
static int GetBlockQPVP9( VP9Context* s, VP9Block* b )
	{
	if( s->s.h.segmentation.enabled && s->s.h.segmentation.feat[b->seg_id].q_enabled )
		{
		if( s->s.h.segmentation.absolute_vals )
			return s->s.h.segmentation.feat[b->seg_id].q_val ;
		else
			return s->s.h.yac_qi + s->s.h.segmentation.feat[b->seg_id].q_val ;
		}

	return s->s.h.yac_qi;
	}

/******************************************************************************/

// Maps VP9-block to MbType
static int GetMbTypeIdxVP9( VP9Block* b )
	{
	// Skip Block
	if( b->skip )
		return( SKIPPED ) ;

	// Intra Block
	else if( b->intra )
		return( INTRA_PLN ) ;

	// Compound Prediction (2 Ref Blocks)
	else if( b->comp == PRED_COMPREF )
		return( BIDIRECT ) ;

	// Single Prediction (1 Ref Block)
	else
		return( FORWARD ) ;
	}

/******************************************************************************/

// Maps VP9 BlockSize to SHAPE
// Can also be used as log2 factor for Blocksize relative to 4x4
static int GetShapeIdxVP9( enum BlockSize bs )
	{
	if( bs % 3 == 0 )               // quare shapes
		return 8 - (bs * 2) / 3 ; // 0 -> 8; 3 -> 6; 6 -> 4; ...
	else
		return 8 - (bs * 2) / 3 -  (bs + 1) % 3 / 2 ; // 1,2 -> 7; 4,5 -> 5; ...
	}

/******************************************************************************/

// Process a single MV pair
// TODO: evaluate whether special treatment for 1/4pel mvs or scaled mvs is necessary
static void ProcessMV( VIDEO_STAT* FrmStat, VP9Block* b, float* NF[2], int idx, int count, int stride, double Dist, int pecision )
	{
  int    i, j, scount, CntD ;
  int    AvMot=1, AvDif=1 ;
	double MV_LengthXY, MV_LengthdXY ;
  double mvX,  mvY ;
  double mvdX, mvdY ;

  CntD = sqrt( count ) ;
  mvX  = (double) abs(b->mv [idx][0].x) ;
  mvY  = (double) abs(b->mv [idx][0].y) ;
  mvdX = (double) abs(b->mvd[idx][0].x) ;
  mvdY = (double) abs(b->mvd[idx][0].y) ;
  //if( b->comp )
		//{
  //  mvX  += (double) abs(b->mv [idx][1].x) ;
  //  mvY  += (double) abs(b->mv [idx][1].y) ;
  //  mvdX += (double) abs(b->mvd[idx][1].x) ;
  //  mvdY += (double) abs(b->mvd[idx][1].y) ;
  //  cnt += 8 ;
		//}

  mvX  /=  (8*Dist) ;
  mvY  /=  (8*Dist) ;
  mvdX /=  (8*Dist) ;
  mvdY /=  (8*Dist) ;
  for( i=0 ; i<CntD ; i++ )
    for( j = 0 ; j < CntD ; j++ )
      {
      NF[0][i*stride + j] = (float)mvX ;
      NF[1][i*stride + j] = (float)mvY ;
      } ;

  if( FrmStat->FrameIdx )
      mvdX = mvdX ;

  if( mvX || mvY )
    {
    MV_LengthXY  = 4.0 * sqrt(  mvX*mvX  +  mvY*mvY ) ;
    MV_LengthdXY = 4.0 * sqrt( mvdX*mvdX + mvdY*mvdY ) ;
    }
  else
    MV_LengthXY  = MV_LengthdXY = 0 ;

  if( FrmStat->S.CodedMv )
    AvMot = FrmStat->S.MV_Length  / FrmStat->S.CodedMv ;
  if( FrmStat->S.CodedMv )
    AvDif = FrmStat->S.MV_dLength / FrmStat->S.CodedMv ;

  if( !((abs( mvX ) + abs( mvY ) > 20 * AvMot) || (abs( mvdX ) + abs( mvdY ) > 20 * AvDif)) )
    {
    if( b->mode[idx] == NEWMV )
      {
      scount = count*count ;
      FrmStat->S.CodedMv      += b->CodedMv[idx] * count ;
      FrmStat->S.MV_Length    += MV_LengthXY * count ;         // Add up of motion vector length for mean value
      FrmStat->S.MV_LengthX   += mvX * count ;
      FrmStat->S.MV_LengthY   += mvY * count ;
      FrmStat->S.MV_SumSQR    += SQR( MV_LengthXY ) * scount ;  // Add up square of motion vector length for variance value
      FrmStat->S.MV_XSQR      += SQR( mvX ) * scount ;
      FrmStat->S.MV_YSQR      += SQR( mvY ) * scount ;
      FrmStat->S.MV_DifSumSQR += SQR( MV_LengthdXY ) * scount ;
      FrmStat->S.MV_dLength   += MV_LengthdXY * count ;    // Add up of motion vector length for mean value
      } ;
    } ;

  if( b->skip )
    FrmStat->S.NumBlksSkip += count ;
  else
    FrmStat->S.NumBlksMv   += count ;
  }

// Collect MV Statistics for the current block
static void BlockMVStatsVP9( VP9Context* s, VIDEO_STAT* FrmStat, int count, float* NfX, float* NfY )
	{
	VP9Block*  b = s->b ;
  double     FrmDist=1 ;
  float      *NormField[2] ;

  NormField[0] = NfX ; NormField[1] = NfY ;
  if( s->s.h.keyframe || s->s.h.intraonly || b->intra )
    {
    if( b->skip )
      FrmStat->S.NumBlksSkip  += count ;
    else
      FrmStat->S.NumBlksIntra += count ;
    return ;
    } ;

  //if( b->ref[0] == 2 )
  //  return ;

  //if( s->s.refs[b->ref[0]].f->FrmStat.IsHidden )   // is the ref as a hidden frame, the pts, which is actually the dts, is meaningless
  //  {
  //  if( FrmStat->IsShrtFrame )
  //    FrmDist = (FrmStat->S.FrameDistance + 1) >> 1 ;
  //  else
  //    return ;
  //  }
  //else
  // FrmDist = (FrmStat->PTS - s->s.refs[ b->ref[0] ].f->pts) / s->s.frames[0].tf.f->pkt_duration ;

  // the following line:
  // FrmDist = max( 1, (FrmStat->PTS - s->s.refs[ b->ref[0] ].f->pts) / s->s.frames[0].tf.f->pkt_duration ) ;
  // was changed to the A, B thing, because B can be zero, and then everything breaks
  float B = s->s.frames[0].tf.f->pkt_duration;
  if (B == 0) {
      FrmDist = 1;  // not sure about this value
  } else {
      float A = FrmStat->PTS - s->s.refs[ b->ref[0] ].f->pts;
      FrmDist = max( 1, (A) / B ) ;
  }

  if (b->bs > BS_8x8)
		{
		ProcessMV(    FrmStat, b, NormField, 0,     1, s->cols, FrmDist, s->s.h.highprecisionmvs) ;
		if (b->bs != BS_8x4)
			ProcessMV  (FrmStat, b, NormField, 1,     1, s->cols, FrmDist, s->s.h.highprecisionmvs) ;
		if (b->bs != BS_4x8)
			{
			ProcessMV(  FrmStat, b, NormField, 2,     1, s->cols, FrmDist, s->s.h.highprecisionmvs) ;
			if (b->bs != BS_8x4)
				ProcessMV(FrmStat, b, NormField, 3,     1, s->cols, FrmDist, s->s.h.highprecisionmvs) ;
      } ;
		}
	else
		ProcessMV(    FrmStat, b, NormField, 0, count, 2*s->cols, FrmDist, s->s.h.highprecisionmvs) ;
	}

/******************************************************************************/

// Collect Coefficient and Zero-Coef Statistics for the current block
static void BlockCoefStatsVP9(VP9Context* s, VIDEO_STAT* FrmStat, int bytesperpixel )
	{
	VP9Block* b ;
	int row, col, w4, h4, end_x, end_y,
			n, x, y, i, j, step ;

	b = s->b ;
	row = s->row, col = s->col ;
	w4 = bwh_tab[1][b->bs][0] << 1 ;
	h4 = bwh_tab[1][b->bs][1] << 1 ;
	end_x = FFMIN(2 * (s->cols - col), w4) ;
	end_y = FFMIN(2 * (s->rows - row), h4) ;

	// tx    -> step
	// 4x4   -> 1
	// 8x8   -> 2
	// 16x16 -> 4
	// 32x32 -> 8
	step = 1 << b->tx ;

	for (n = 0, y = 0; y < end_y; y += step) // tx-block row
		{
		for (x = 0; x < end_x; x += step, n += step * step) // tx-block col
			{
			int n_coeffs = 16 * step * step ;
			uint32_t sum = 0 ;

      if( bytesperpixel == 1 )
        {
        int16_t *block_coef = s->block + 16 * n  ;
        for( i = 0; i < n_coeffs; i++ )
          {
          sum += block_coef[i] ;
          FrmStat->S.AverageCoefs[b->tx][i]       += (double)abs( block_coef[i] ) ;
          FrmStat->S.AverageCoefsSQR[b->tx][i]    += (double)block_coef[i] * (double)block_coef[i] ;
          FrmStat->S.AverageCoefsBlkCnt[b->tx][i] += (double)(block_coef[i] != 0) ;
          } ;
        }
      else
        {
        int32_t *block_coef = s->block + 32 * n ;
        for( i = 0; i < n_coeffs; i++ )
          {
          sum += block_coef[i] ;
          FrmStat->S.AverageCoefs[b->tx][i]       += (double)abs( block_coef[i] ) ;
          FrmStat->S.AverageCoefsSQR[b->tx][i]    += (double)block_coef[i] * (double)block_coef[i] ;
          FrmStat->S.AverageCoefsBlkCnt[b->tx][i] += (double)(block_coef[i] != 0) ;
          } ;
        }


			// Count 4x4 columns with zero-tx
			if (sum == 0)
        for( j = y; j < y + step ; j++ )
          s->BlackLine[2 * row + j] += step ;
			}
		}
	}


/******************************************************************************/

void InitFrameStatisticsVP9( AVCodecContext* ctx, VIDEO_STAT* FrmStat, VIDEO_STAT* FrmStatRef, AVPacket* pkt )
  {
  VP9Context *s = ctx->priv_data ;
  int        FieldSize ;

  Tmp          = 0 ;
  FrmStat->PTS = FrmStatRef->PTS = pkt->pts ;
  s->BlackLine = (int*)calloc( ctx->width >> 2, sizeof( int ) ) ;   // P.L.
  FrmStat->BlackBorder =  FrmStatRef->BlackBorder = CurrBlackBorder ;

  SAFE_FREE( FrmStat->S.NormalizedField[0] ) ;
  SAFE_FREE( FrmStat->S.NormalizedField[1] ) ;
  memset( &(FrmStat->S),    0, sizeof( FRAME_SUMS ) );
  memset( &(FrmStatRef->S), 0, sizeof( FRAME_SUMS ) );

  FrmStat->IsHidden = FrmStatRef->IsHidden = s->s.h.invisible ;
  FrmStat->S.FrameDistance = FrmStatRef->S.FrameDistance = VP9Hidden_FrmStat.S.FrameDistance ;
  if( pkt->size < 100 )
    {
    FrmStat->IsShrtFrame  = FrmStatRef->IsShrtFrame  = 1 ;
    VP9Hidden_FrmStat.PTS = FrmStat->PTS ;
    } ;

  FieldSize = (s->w*s->h) >> 4  ;
  FrmStat->S.NormalizedField[0] = calloc( FieldSize, sizeof( float )) ;
  FrmStat->S.NormalizedField[1] = calloc( FieldSize, sizeof( float ))  ;
  }

/******************************************************************************/

// Called from vp9.c in decode_mode()
void ModeStatistics( VP9Context* s, VIDEO_STAT* FrmStat, int row, int col, int bytesperpixel  )         // row/col in an 8x8 grid
	{
		VP9Block* b = s->b ;

                EMMS() ;
		int shape = GetShapeIdxVP9(b->bs) ;
		int mbType = GetMbTypeIdxVP9(b) ;
		int count = 1 << shape;      // not correct, since shape can be 4x4, but then it is 4 of theses 4x4 blocks
    int Offset = 2 * row * s->cols + 2*col ;                                    // array-offset for a 4x4 grid

    count     = FrmStat->S.BlkCnt4x4 ;  // number of 4x4 blocks

    //count = (1 << shape) * (((bl == 3) && (b->bp == 3))? 4:1) ;
		// All block statistics scaled to 4x4px blocksize-equivalent
		FrmStat->MbQPs[mbType]    += GetBlockQPVP9(s, b) * count ;
		FrmStat->MbTypes[mbType]  += count ;
		FrmStat->S.MbCnt          += count ;
		FrmStat->BlkShapes[shape] += count ;
		FrmStat->TrShapes[b->skip ? 0 : b->tx + 2] += count ;

		// Calculate MV and Coef Blockstats
		BlockMVStatsVP9(  s, FrmStat, count, FrmStat->S.NormalizedField[0]+Offset, FrmStat->S.NormalizedField[1]+Offset ) ;
		BlockCoefStatsVP9(s, FrmStat, bytesperpixel ) ;
    FrmStat->S.BlkCnt4x4  = 0 ;
	}




//****************** Called from vp9.c at the end of decode_frame()  *************************************//

void FrameStatistics( AVCodecContext* ctx, VIDEO_STAT* FrmStat, VIDEO_STAT* FrmStatRef, AVPacket* pkt )
	{
  VP9Context *s = ctx->priv_data ;
  FRAME_SUMS *S = &VP9Hidden_FrmStat.S ;

  if( pkt->size < 100 )
    {
    VP9Hidden_FrmStat.IsShrtFrame  = 1 ;
    VP9Hidden_FrmStat.S.FrameDistance = max(1, VP9Hidden_FrmStat.S.FrameDistance-2) ;

    S->MV_DifSum    /= VP9Hidden_FrmStat.S.FrameDistance ;
    S->MV_DifSumSQR /= VP9Hidden_FrmStat.S.FrameDistance ;
    S->MV_Length    /= VP9Hidden_FrmStat.S.FrameDistance ;
    S->MV_dLength   /= VP9Hidden_FrmStat.S.FrameDistance ;
    S->MV_LengthX   /= VP9Hidden_FrmStat.S.FrameDistance ;
    S->MV_LengthY   /= VP9Hidden_FrmStat.S.FrameDistance ;
    S->MV_SumSQR    /= VP9Hidden_FrmStat.S.FrameDistance ;
    S->MV_XSQR      /= VP9Hidden_FrmStat.S.FrameDistance ;
    S->MV_YSQR      /= VP9Hidden_FrmStat.S.FrameDistance ;

    memcpy( FrmStat,    &VP9Hidden_FrmStat, sizeof( VIDEO_STAT ) ) ;
    memcpy( FrmStatRef, &VP9Hidden_FrmStat, sizeof( VIDEO_STAT ) ) ;
    }
  else
    {
    FrmStat->BlackBorder = (s->s.h.keyframe) ? (CurrBlackBorder = BlackborderDetect( s->BlackLine, s->rows * 2, (int)(1.2 * s->cols), 2 )) : CurrBlackBorder ;
    FrmStat->FrameSize = pkt->size;
    if( s->s.h.invisible )
      {
      memcpy( &VP9Hidden_FrmStat, &s->s.frames[0].tf.f->FrmStat,   sizeof( VIDEO_STAT ) ) ;
      VP9Hidden_FrmStat.S.FrameDistance = 0 ;
      VP9Hidden_FrmStat.IsHidden = 1 ;
      }
    else
      VP9Hidden_FrmStat.S.FrameDistance++ ;
    } ;

  free( s->BlackLine ) ;
	}

/*****************************************************************************************************************/