#include "stdafx.h"
#include "VideoStat.h"
#include <iostream>

using namespace std;

extern "C"
  {
  int   OpenVideo( char*  FileName, BYTE**  ParserStore ) ;
  int   ReadNextFrame( BYTE* Parser ) ;
  int   GetFrameStatistics( BYTE* Parser, VIDEO_STAT* FrmStat, VIDEO_STAT SeqStat[][4] ) ;
  void  GetSeqStatistic( BYTE* Parser, VIDEO_STAT* SeqStat ) ;
  }


template<typename T> void print_array_1d(const char* label, T array, int64_t r) {
    cout << label << " : ";
    for(int64_t i = 0; i < r - 1; i++) {
        cout << array[i] << ", ";
    }
    cout << array[r - 1] << endl;
}


template<typename T> void print_array_2d(const char* label, T array, int64_t r, int64_t c) {
    cout << label << " : " << endl;
    for(int64_t i = 0; i < r - 1; i++) {
        print_array_1d("\t", array[i], c);
    }
    print_array_1d("\t", array[r - 1], c);
    cout << endl;
}


template<typename T> void print_array_3d(const char* label, T array, int64_t r, int64_t c, int64_t d) {
    cout << label << " : " << endl;
    for(int64_t i = 0; i < r - 1; i++) {
        print_array_2d(" ", array[i], c, d);
    }
    print_array_2d(" ", array[r - 1], c, d);
    cout << endl;
}


void print_framesum(FRAME_SUMS& framesum) {
    printf("S: \n");
    printf("\tNumBlksMv: %d\n", framesum.NumBlksMv);
    printf("\tNumBlksMv: %d\n", framesum.NumBlksSkip);
    printf("\tNumBlksMv: %d\n", framesum.NumBlksIntra);
}

void print_framestat(VIDEO_STAT& FrameStat) {
    printf("------------------------------------------------------------\n");
    print_framesum(FrameStat.S);

    printf("AnalyzedMBs : %f\n", FrameStat.AnalyzedMBs);
    printf("Av_QP : %f\n", FrameStat.Av_QP);
    printf("StdDev_QP : %f\n", FrameStat.StdDev_QP);
    printf("Av_QPBB : %f\n", FrameStat.Av_QPBB);
    printf("StdDev_QPBB : %f\n", FrameStat.StdDev_QPBB);
    printf("min_QP : %f\n", FrameStat.min_QP);
    printf("max_QP : %f\n", FrameStat.max_QP);
    printf("InitialQP : %f\n", FrameStat.InitialQP);

    print_array_1d("MbQPs", FrameStat.MbQPs, 7);

    printf("Av_Motion : %f\n", FrameStat.Av_Motion);
    printf("StdDev_Motion : %f\n", FrameStat.StdDev_Motion);
    printf("Av_MotionX : %f\n", FrameStat.Av_MotionX);
    printf("Av_MotionY : %f\n", FrameStat.Av_MotionY);
    printf("StdDev_MotionX : %f\n", FrameStat.StdDev_MotionX);
    printf("StdDev_MotionY : %f\n", FrameStat.StdDev_MotionY);
    printf("Av_MotionDif : %f\n", FrameStat.Av_MotionDif);
    printf("StdDev_MotionDif : %f\n", FrameStat.StdDev_MotionDif);

    print_array_3d("Av_Coefs", FrameStat.Av_Coefs, 4, 32, 32);
    print_array_3d("StdDev_Coefs", FrameStat.StdDev_Coefs, 4, 32, 32);
    print_array_3d("HF_LF_ratio", FrameStat.HF_LF_ratio, 3, 4, 3);
    print_array_1d("MbTypes", FrameStat.MbTypes, 7);
    print_array_1d("BlkShapes", FrameStat.BlkShapes, 9);
    print_array_1d("TrShapes", FrameStat.TrShapes, 7);
    print_array_1d("FarFWDRef", FrameStat.FarFWDRef, 2);

    printf("FrameSize : %d\n", FrameStat.FrameSize);
    printf("FrameType : %d\n", FrameStat.FrameType);
    printf("BlackBorder : %d\n", FrameStat.BlackBorder);

    printf("DTS : %ld\n", FrameStat.DTS);
    printf("PTS : %ld\n", FrameStat.PTS);

    printf("------------------------------------------------------------\n");
    printf("\n\n");
}

VIDEO_STAT    SeqStat[16][4] = { 0 } ;
VIDEO_STAT    FrameStat ;

int main( int argc, char *argv[] )
  {
  int           ValidData, CurrGOB=-1 ;
  BYTE*         Parser ;
  char*         DotPos ;

  if( argc != 2 )
    {
    fprintf( stderr, "usage: Inputfile-name \n" );
    exit( 1 );
    } ;

  if( (DotPos = strrchr( argv[1], (int)'.' )) == NULL)
    {
    fprintf( stderr, "No valid Inputfile \n" );
    exit( 1 );
    } ;


  if( OpenVideo( argv[1], &Parser ) )
    {
    do
      {
      if( (ValidData = ReadNextFrame( Parser )) > 0 )
        {
        CurrGOB = GetFrameStatistics( Parser, &FrameStat, SeqStat ) ;
        //print_framestat( FrameStat ) ;
        //printf("NumBlksSkip: %i\n", FrameStat.NumBlksSkip);
        //printf("NumBlksMv: %i\n", FrameStat.NumBlksMv);
        //printf("NumBlksMerge: %i\n", FrameStat.NumBlksMerge);
        //printf("NumBlksIntra: %i\n", FrameStat.NumBlksIntra);

        } ;
      }
    while( ValidData >= 0 ) ;
    } ;

  for( int i=0 ; i<=CurrGOB ; i++ )
    GetSeqStatistic( Parser, SeqStat[i] ) ;
  }


/******************************************************************************/

