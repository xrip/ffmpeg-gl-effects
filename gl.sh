#!/bin/bash
# Example of concatenating 3 mp4s together with 1-second transitions between them.

ffmpeg -i ./media/2.mp4 -vf "shadertoy=./shader.glsl" -c:v libx264 output.mp4