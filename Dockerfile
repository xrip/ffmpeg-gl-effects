# This is a contributed example of how to build ffmpeg-gl-transions using Docker
# If you use Docker, this should get the job done
# if you don't use Docker, you could still run the commands
# manually and get the same result


FROM ubuntu:20.04
# should also work for
#FROM ubuntu:18.04

# everything is relative to /build
WORKDIR /build

# enable contrib/non-free
#RUN sed -E -e 's/\s+main\s?$/ main contrib non-free /' /etc/apt/sources.list >/etc/apt/sources.list.new && mv /etc/apt/sources.list.new /etc/apt/sources.list

# update anything needed & install git
RUN apt-get -y update -y && apt-get -y upgrade && apt-get -y install git
# dependencies needed for ffmpeg compile
RUN DEBIAN_FRONTEND=noninteractive TZ=Europe/Moscow apt-get -y install gcc g++ make pkg-config \
    nasm yasm libglew2.1 libglew-dev libx264-dev libglu1-mesa libmp3lame-dev
 \
    # get ffmpeg sources
RUN git clone --depth 1 http://git.videolan.org/git/ffmpeg.git/ ffmpeg && git clone --depth 1 https://github.com/xrip/ffmpeg-gl-effects.git && cp /build/ffmpeg-gl-effects/*.c ffmpeg/libavfilter/

# apply patch, configure/compile/install ffmpeg
RUN (cd ffmpeg; git apply /build/ffmpeg-gl-effects/ffmpeg.diff; ./configure --enable-libx264 --enable-libmp3lame --enable-nonfree --enable-gpl --enable-opengl --enable-filter=gltransition --enable-filter=shadertoy --extra-libs='-lGLEW -lEGL -ldl'; make -j && make install )


ENTRYPOINT /bin/bash

