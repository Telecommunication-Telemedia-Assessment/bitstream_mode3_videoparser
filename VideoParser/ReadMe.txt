C:\seq\TV-Model\HEVC\Astra.SES.Demo.HEVC.ts
C:\seq\H264-Conformance\Testcases\CVWP2_TOSHIBA_E.264 // 2 frames before the first I-Frame
C:\seq\H264-Conformance\Testcases\CAWP5_TOSHIBA_E.264

C:\seq\TV-Model\HEVC\sintel_vsm_265.mp4

C:\seq\TV-Model\P.1203\SRC004_Q21_0000_0-10-VP9.mkv

C:\seq\tv-model\hevc\BlackBoarder.mp4    // HEVC

C:\seq\TV-Model\P.1203\bigbuck_bunny_8bit-vp9-1-7500kbps-60fps-1080p-2.mkv
C:\seq\VIR-Q\264-16Mb\SRC001_16M_HD264.mp4


C:\seq\vir-q\probleme\P2STR09_SRC00412_Q0118_0000_0-9.mp4


C:\seq\TV-Model\P.1203\SRC006_Q23_0000_0-10-VP9.mkv                  Blackborder 16 VP9
C:\seq\TV-Model\P.1203\blackborder\TR01_SRC115_Q3_00001.ts           Blackborder 16 H264
C:\seq\H264-Conformance\Testcases\NLMQ2_JVC_C.264                    H264  CAVLC
C:\seq\tv-model\P.1203\outputVP9.mkv


for H.265:
ffmpeg -i input.mp4 -c:v libx265 -x265-params bframes=0:ref=1:no-open-gop=1:keyint=-1:scenecut=0:weightp=1  -b:v 16000k   -vf scale=1920:1080  c:\seq\dst\output265.mp4

for H.264: 
ffmpeg -i input.mp4 -c:v libx264 -x264-params keyint_min=600:ref=1:no-open-gop=1:bframes=0:scenecut=0 -partitions p4x4+p8x8 -b:v 16000k  -vf scale=1920:1080   c:\seq\dst\output264.mp4

for VP9:
ffmpeg -i input.mp4 -c:v libvpx-vp9  -keyint_min=600  -b:v 16000k   -vf scale=1920:1080  c:\seq\dst\output2-265.mp4








HEVC:
if skip_flag == TRUE:   no more bits in CU, motion is predicted by using merge mode. 
   skip_flag == FALSE:  either Motion coded or merge mode applied
						 