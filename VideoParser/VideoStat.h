#pragma once

extern int Started, Opened ;

#define      SQR(_x_)  (_x_)*(_x_)
#define      SAFE_FREE( _x_ ) { if( _x_ ) free(_x_); _x_=0 ; }


// some defines as used in the old statistic
#define      SKIPPED              0
#define      FORWARD              1
#define      BACKWARD             2
#define      BIDIRECT             3
#define      DIRECT               4
#define      INTRA_BLK            5
#define      INTRA_PLN            6

#define      TRANSF_4X4           0
#define      TRANSF_8X8           1
#define      TRANSF_16X16         2
#define      TRANSF_32X32         3

#define      SHAPE_64x64          8
#define      SHAPE_64x32          7
#define      SHAPE_32x32          6
#define      SHAPE_32x16          5
#define      SHAPE_16x16          4
#define      SHAPE_16x8           3
#define      SHAPE_8x8            2
#define      SHAPE_8x4            1
#define      SHAPE_4x4            0

#define      CODEC_H264           1
#define      CODEC_HEVC           2
#define      CODEC_VP9            3


typedef struct FRAME_SUMS
  {
  int      QpSum, QpSumSQ, QpCnt ;
  int      QpSumBB, QpSumSQBB, QpCntBB ;
  int      NumBlksMv, NumMvs,NumMerges, MbCnt, BlkCnt4x4 ;       // NumMvs, NumMerges   not used
  int      CodedMv, NumBlksSkip, NumBlksIntra ;
  double   BitCntMotion, BitCntCoefs ;
  double   MV_Length, MV_dLength,  MV_SumSQR ;
  double   MV_LengthX,   MV_LengthY, MV_XSQR, MV_YSQR ;
  double   MV_DifSum, MV_DifSumSQR ;
  int32_t  PU_Stat[7][6] ;
  int32_t  TreeStat ;
  double   AverageCoefs[5][1024] ;    
  double   AverageCoefsSQR[5][1024] ;  
  uint32_t AverageCoefsBlkCnt[5][1024] ; //  [0]:No Transf.   [2]: 4x4    [3]: 8x8    [4]: 16x16    [5]: 32*32
  float*   NormalizedField[2] ;
  int      FrameDistance ;           // to count the number of frames between a "hidden frame" and a "golden fame"
  } FRAME_SUMS ;

typedef struct SEQ_DATA
  {
  int      ChromaFormat ;            // 0=Graylevel   1=4:2:0    2=4:2:2     3:4:4:4      4:RGB
  int      BitDepth ;                // in bit 8...12
  int      Profile ;                 // HEVC:  1=Main    2=Main10     3=StilPicture     4=R-EXT
                                     // H.264: 88=Baseline  77=Main  88=Extended  100=High  110:High10   122=High422 .... and many more
                                     // VP9:   0:8bit 4:2:0     1:8bit 4:2:2/4:4:4     2:10-12bit 4:2:0      3:10-12bit 4:2:2/4:4:4
  int      Level ;                   // for instance: 120 -> level 1.20   VP9 has no levels(?)
  int      Bitrate ;
  double   FramesPerSec ;
  double   BitsPertPel ;
  int      Resolution ;              // 0:CIF   1:SD    2:HD    3:UHD
  int      ArithmCoding ;            // Has arithmetic coding?  (always true for H.265 and VP9)
  int      Codec ;                   // 1:H.264     2:HEVC      3:VP9
  int      FrameWidth, FrameHeight ;
  } SEQ_DATA ;


typedef struct VIDEO_STAT
  {
  FRAME_SUMS S ;                     // for internal use only
  SEQ_DATA   Seq ;                   // Seqeunce base properties (see above)
  //float      MotionField[2][3840*2160/16] ;     // the complete motion vector field

  int      IsHidden, IsShrtFrame ;   // only valid for VP9, internal use
  double   AnalyzedMBs ;             // Number of Blockstructures in frame (H.264: Macroblocks  H.265: CodingBlocks with min_cb_Size  VP9: Blocks)
  double   SKIP_mb_ratio ;           // NumberOfSkippedBlocks / AnalyzedMBs
  double   Av_QP ;                   // Average QP of whole frame
  double   StdDev_QP ;               // Standard diviation of Av_QP
  double   Av_QPBB ;                 // If the frame has a black border at the top and bottom (wide screen movie), the QP of the black
                                     // part usually distorts the average QP. Therefore Av_QPBB is the average QP WITHOUT the black regions
  double   StdDev_QPBB ;             // Standard diviation of Av_QPBB
  double   min_QP, max_QP;           // Minimum and maximum QP values accountered in this frame
  double   InitialQP ;               // QP Value the frame is starting with (to be found in the slice- or frame-header)
  double   MbQPs[7] ;                // Average QP dependant on MB-Types (SKIP=0   L0=1    L1=2     Bi=3   INTRA(directional)=5  INTRA(plane)=6
  //------------------------------------
  double   Av_Motion ;               // average length ( sqrt(x*x + y*y) ) of the vectors in the motion field.
                                     // Grid for H.264: 4x4 (a 16*16 vector is counted 16 times)
                                     // Grid for H.265: MinPredictionUnit size, usually also 4x4 but in principle variable
  double   StdDev_Motion  ;          // Standard Deviation of Av_Motion
  double   Av_MotionX, Av_MotionY ;  // Same as Av_Motion but split into x and y direction. ( Average of abs(MotX) and abs(MotY) )
  double   StdDev_MotionX, StdDev_MotionY ;   // Standard deviation of Av_MotionX and Av_MotionY
  double   Av_MotionDif,   StdDev_MotionDif ; // Same as Av_Motion, but not with the motion itself, but the difference of the motion
                                              // with its prediction, which is the effectively transmitted value. Idea behind this: If the motion
                                              // is well predictable (rather small values compared to Av_Motion), it also is regular and smooth.
                                              // non predictable usually means complex or caotic motion resulting in a complex frame
  //-----------------------------------
  double   Av_Coefs[4][32][32] ;       // [NoTrans,-,4x4, 8x8, 16x16, 32x32][coef-y][codef-x]. Represents the 2-dim arrays of the average
                                       // size of the transform coefficients in their block-positions. H.264 only has 4x4 and 16x16 transforms,
                                       // whereas H.265 has 4x4, 8x8, 16x16 and 32x32 transforms. There are different averages for P-frames[0]
                                       // I-frames[2] and b-frames[3]
  double   StdDev_Coefs[4][32][32] ;   //  standard deviation of Av_Coef
  double   HF_LF_ratio[3][4][3];	     // HF_LF_ratio[4x4 - 8x8 - 16x16 - 32x32][HF - LF - DC] the Ratio between high-frequency and low-frequency
                                       // components in Av_Coef arrays. Decision whether a coef is LF or HF is done by a diagonal separation:
                                       // HfLf = ((row+col) == 0)? 2 : (int)(col > (size - row - 1)). The idea behind this: The better the prediction
                                       // or the easier the content, the smaller are the high frequency components and the more effective is the
                                       // compression.
  //------------------------------------
  double   MbTypes[7] ;                // Count for 0=Skipped   1=Forward (L0)   2=Backward (L1)   3=Bidirect    4=Direct    5=Intra(directional)
                                       // 6=Intra(planar)  ( H.264:  [5]=always 4x4   /  [6]=always 16x16 )

  double   BlkShapes[10] ;             // count for Shapes: 8=64x64    7=64x32,32x64  6=32x32  5=16x32,32x16   4=16x16  3=16x8,8x16  2=8x8
                                       // 1=8x4,4x8  0=4x4    (H.264: only 0,1,2,3,4 )  [9] = Block-Size Measure 
  double   TrShapes[8] ;               // count for Transform Sizes    6=64x64  5=32x32  4=16x16  3=8x8   2=4x4    0=NO_TRANSFORM (on 4x4 basis)   (H.264: only 0,2,3)  [7]=Tr.Size Measure
  double   FarFWDRef[2] ;              // 0: BLocks, which use the nearest reference frame for FORWARD-prediction    1: Block uses Refs. more far away

  double   PredFrm_Intra ;             // Amount of I-blk area in a predicted frame
  int      FrameSize ;                 // Number of actual bytes for this frame
  int      FrameType ;                 // Type of this frame ( 1=I   2=P    3=B )
  int      IsIDR  ;                    // 0: any frame   1: IDR-Intra frame
  int      FrameIdx ;
  int      FirstFrame, NumFrames ;     // only used for Sequence Statistics
  int      BlackBorder ;               // Number of black scanlines at top and bottom of frame.
  int64_t  DTS ;                       // The frames Decoding Timestamp (if transportstream)
  int64_t  PTS ;                       // The frames Presentation Timestamp (if transportstream)
  int      CurrPOC, POC_DIF ;
  double   FrameCnt ;
  double   SpatialComplexety[3] ;
  double   TemporalComplexety[3] ;
  double   TI_Mot[2], TI_910, SI_910 ;
  double   Blockiness[3][6] ;

  double   BitCntMotion, BitCntCoefs ;
  int      NumBlksSkip, NumBlksMv, NumBlksMerge, NumBlksIntra ;
  int      CodedMv ;
  } VIDEO_STAT ;

extern int  CurrBlackBorder ;
