# bitstream_mode3_videoparser
Authors: Peter List, Anton Schubert, Steve Göring, Werner Robitza

## Explanation

- ExternalLibs: external dependencies for ffmpeg **(only for windows)**
- ffmpeg: libav* libraries from FFmpeg project
  - Subdirectory ffmpg/SMP contains everything needed to compile ffmpeg under
    VS-2015. Everything else is the original ffmpeg, modified with // P.L. or //A.S. annotations
- TestMain:    test project for interfacing with the video parser
- VideoParser: code for a dll parsing any video with the help of ffmpeg and providing
               statistical information on a frame basis

## Building under Windows

Install the following requirements:

- Windows 10  (most probably not required)
- Visual Studio 2015 (Community Edition)
- Visual C++ Redistributable 2015
- C++ Tools for Visual Studio
- [YASM](http://yasm.tortall.net/Download.html)
  - Download the version for "64-Bit for VS2010 and above"
  - Copy the vsyasm.exe file to `C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\bin`
  - Copy the other files to `C:\Program Files (x86)\MSBuild\Microsoft.Cpp\v4.0\V140\BuildCustomizations`
  - In the `.props` file, replace the occurrence of `$(Platform)` with `win$(PlatformArchitecture)`

Building:

- Open the Visual Studio solution file `TestMain/TestMain.sln`
- Build the solution in 32-Bit or 64-Bit Debug mode
- Note that some error messages related to linking are normal

This will output a DLL that can be included in another program. The DLL file is here:

    \videoparser\TestMain\Debug\VideoParser.dll

The DLL will provide a per frame bitstream statistic of the parsed video.
It contains 3 external functions:

    OpenVideo( char* FileName, BYTE** Parser )
    ReadNextFrame( BYTE* Parser )
    GetFrameStatistics( BYTE* Parser, VIDEO_STAT* FrameStat)

In principle this is the way to use the dll:


      BYTE*   Parser ;
      VIDEO_STAT   FrameStatistics

      if( OpenVideo( FileName, &Parser ) )
        {
        do
          {
          MoreData  = ReadNextFrame( Parser ) ;
          GetFrameStatistics( Parser, &FrameStatistics ) ;
          }
        while( MoreData ) ;
        } ;


## Building under Linux

Requirements:
- Python 3
- SCons build system
- GCC

Ubuntu 18.04 (mostly FFmpeg dependencies):

```bash
sudo apt-get update -qq
sudo apt-get install -y -qq python3 python3-numpy python3-pip git scons
sudo apt-get -y install autoconf automake build-essential libass-dev libfreetype6-dev libsdl2-dev libtheora-dev libtool libva-dev libvdpau-dev libvorbis-dev libxcb1-dev libxcb-shm0-dev libxcb-xfixes0-dev pkg-config texinfo wget zlib1g-dev yasm
sudo pip3 install --upgrade pip
sudo pip3 install pandas
```

Building:

```bash
# Configure, Build and Install ffmpeg
cd ffmpeg && ./configure_ffmpeg.sh && make -j $(nproc) && make install

# Build videoparser Application and libvideoparser
cd ../VideoParser && scons
```
OR for a small test just run `build_and_test.sh`

## Building / Running with Docker

First, install Docker, then:

    docker build -t videoparser .

This will build the parser into a docker image called `videoparser`.

Then, you can run it on any file:

    docker run --rm -v $(pwd)/test_videos:/test_videos -t videoparser /test_videos/bigbuck_bunny_8bit-hevc-main-2000kbps-60fps-720p-2.mkv --output /test_videos/stats.json.bz2

Here, you have to mount the directory of the file into Docker.


