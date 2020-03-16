#pragma once
#include "VideoStat.h"

#define SobelMin  -8000
#define SobelMax   8000

int BlackborderDetect( int* BlackLine, int rows, int threshold, int logBlkSize ) ;
void QPStatistics( VIDEO_STAT* FrmStat, int CurrQP, int CurrType, int NoBorder ) ;
void CoefsStatistics( VIDEO_STAT* FrmStat, double AvCoefs[5][1024], double AvCoefsSQR[5][1024], int AvCoefsCnt[5][1024], double TrShapes[8], int MaxSize, int FrameType ) ;
void FinishStatistics( VIDEO_STAT* FrmStat, int FrameType, int NumPel, int NumBlk ) ;
int GetResolution( int NumPix ) ;
