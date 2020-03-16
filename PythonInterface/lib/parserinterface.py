import os
import sys
import ctypes
from ctypes import *
import numpy as np


class ParserInterface:
    """
    Class for interfacing with DLL on a low level
    """
    def __init__(self, input_file, dll_file):
        self.input_file = input_file
        self.dll_file = dll_file

        if not os.path.isfile(self.dll_file):
            raise IOError("DLL file not found")

        self.dll = CDLL(self.dll_file)
        self.decoded_frames = 0

    def init(self):
        """
        Open the input file
        returns True on success

        Signature: int OpenVideo(char* inputFile,unsigned char** Parser)
        """
        self.stats = VIDEO_STAT()
        self.parser_ptr = POINTER(c_ubyte)()

        OpenVideo_fun = getattr(self.dll, "OpenVideo")
        if sys.version_info.major == 3:
            ret = OpenVideo_fun(c_char_p(bytes(self.input_file, "utf-8")), byref(self.parser_ptr))
        else:
            ret = OpenVideo_fun(c_char_p(self.input_file, "utf-8"), byref(self.parser_ptr))
        return ret == 1

    @staticmethod
    def get_dict_from_struct(struct):
        """
        Convert a ctypes struct to a Python dictionary
        http://stackoverflow.com/q/3789372/
        """
        result = {}
        for field, _ in struct._fields_:
            value = getattr(struct, field)
            # if the type is not a primitive and it evaluates to False ...
            if (type(value) not in [int, float, bool]) and not bool(value):
                # it's a null pointer
                value = None
            elif hasattr(value, "_length_") and hasattr(value, "_type_"):
                # Probably an array
                value = np.ctypeslib.as_array(value).tolist()
            elif hasattr(value, "_fields_"):
                # Probably another struct
                value = ParserInterface.get_dict_from_struct(value)
            else:
                # do nothing to the data; take as-is
                pass
            result[field] = value
        return result

    def parse_next_frame(self):
        """
        Parse the next frame. Returns data on success or False on error.

        int ReadNextFrame(unsigned char* Parser)
        int GetFrameStatistics(unsigned char* Parser, struct DECODER_STATISTICS *)
        """
        ReadNextFrame_fun = getattr(self.dll, "ReadNextFrame")
        GetFrameStatistics_fun = getattr(self.dll, "GetFrameStatistics")

        SeqStat = ((VIDEO_STAT *4) * 16)()

        res = ReadNextFrame_fun(self.parser_ptr)
        if res > 0:
            #print(ParserInterface.get_dict_from_struct(self.stats))
            GetFrameStatistics_fun(self.parser_ptr, pointer(self.stats), pointer(SeqStat))
            return ParserInterface.get_dict_from_struct(self.stats)
        if res == 0:
            return None

        return False

# use clang2py VideoStat.h  to get a starting point of c header definitions

class FRAME_SUMS(Structure):
    """ copy of definition from VideoStat.h
    """
    _fields_ = [
            ("QpSum",c_int),
            ("QpSumSQ",c_int),
            ("QpCnt",c_int),
            ("QpSumBB",c_int),
            ("QpSumSQBB",c_int),
            ("QpCntBB",c_int),
            ("NumBlksMv",c_int),
            ("NumMvs",c_int),
            ("NumMerges",c_int),
            ("MbCnt",c_int),
            ("BlkCnt4x4", c_int),
            ("CodedMv",c_int),
            ("NumBlksSkip",c_int),
            ("NumBlksIntra",c_int),
            ("BitCntMotion",c_double),
            ("BitCntCoefs",c_double),
            ("MV_Length",c_double),
            ("MV_dLength",c_double),
            ("MV_SumSQR",c_double),
            ("MV_LengthX",c_double),
            ("MV_LengthY",c_double),
            ("MV_XSQR",c_double),
            ("MV_YSQR",c_double),
            ("MV_DifSum",c_double),
            ("MV_DifSumSQR",c_double),
            ("PU_Stat",((c_int32*6)*7)),
            ("TreeStat",c_int32),
            ("AverageCoefs",((c_double*1024)*5)),
            ("AverageCoefsSQR",((c_double*1024)*5)),
            ("AverageCoefsBlkCnt",(c_uint32*1024)*5),
            ("NormalizedField",(c_double*2)),
            ('FrameDistance', c_int)
    ]


class SEQ_DATA(Structure):
    """ copy of definition from VideoStat.h
    """
    _fields_ = [
            ("ChromaFormat",c_int),
            ("BitDepth",c_int),
            ("Profile",c_int),
            ("Level",c_int),
            ("Bitrate",c_int),
            ("FramesPerSec",c_double),
            ("BitsPertPel",c_double),
            ("Resolution",c_int),
            ("ArithmCoding", c_int),
            ("Codec",c_int),
            ("FrameWidth", c_int),
            ("FrameHeight", c_int)
    ]


class VIDEO_STAT(Structure):
    """
    Copy of the VideoStat.h field structure
    """
    _fields_ = [
            ("S",FRAME_SUMS),
            ("Seq",SEQ_DATA),
            ('IsHidden',c_int),
            ('IsShrtFrame', c_int),
            ("AnalyzedMBs",c_double),
            ("SKIP_mb_ratio",c_double),
            ("Av_QP",c_double),
            ("StdDev_QP",c_double),
            ("Av_QPBB",c_double),
            ("StdDev_QPBB",c_double),
            ("min_QP",c_double),
            ("max_QP",c_double),
            ("InitialQP",c_double),
            ("MbQPs",(c_double*7)),
            ("Av_Motion",c_double),
            ("StdDev_Motion",c_double),
            ("Av_MotionX",c_double),
            ("Av_MotionY",c_double),
            ("StdDev_MotionX",c_double),
            ("StdDev_MotionY",c_double),
            ("Av_MotionDif",c_double),
            ("StdDev_MotionDif",c_double),
            ("Av_Coefs",(((c_double*32)*32)*4)),
            ("StdDev_Coefs",(((c_double*32)*32)*4)),
            ("HF_LF_ratio",(((c_double*3)*4)*3)),
            ("MbTypes",(c_double*7)),
            ("BlkShapes",(c_double*10)),
            ("TrShapes",(c_double*8)),
            ("FarFWDRef",(c_double*2)),
            ("PredFrm_Intra",c_double),
            ("FrameSize",c_int),
            ("FrameType",c_int),
            ("IsIDR",c_int),
            ("FrameIdx",c_int),
            ("FirstFrame",c_int),
            ("NumFrames",c_int),
            ("BlackBorder",c_int),
            ("DTS",c_int64),
            ("PTS",c_int64),
            ("CurrPOC",c_int),
            ("POC_DIF",c_int),
            ("FrameCnt",c_double),
            ("SpatialComplexety",(c_double*3)),
            ("TemporalComplexety",(c_double*3)),
            ('TI_Mot', (c_double*2)),
            ('TI_910', c_double),
            ('SI_910', c_double),
            ("Blockiness", ((c_double*6)*3)),
            ('BitCntMotion', c_double),
            ('BitCntCoefs', c_double),
            ('NumBlksSkip', c_int),
            ('NumBlksMv', c_int),
            ('NumBlksMerge', c_int),
            ('NumBlksIntra', c_int),
            ('CodedMv', c_int),
    ]
