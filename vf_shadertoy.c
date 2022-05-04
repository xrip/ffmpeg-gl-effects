/**
 * Shadertoy ffmpeg filter
 * @Date    20210530
 * @Author  Canta <canta@canta.com.ar>
 *
 * @Comments: Check previous projects from other authors:
 *            - https://github.com/transitive-bullshit/ffmpeg-gl-transition
 *            - https://github.com/nervous-systems/ffmpeg-opengl
 *            - https://github.com/numberwolf/FFmpeg-Plus-OpenGL
 *
 */
#include "libavutil/opt.h"
#include "internal.h"

#ifndef __APPLE__
# define GL_TRANSITION_USING_EGL //remove this line if you don't want to use EGL
#endif

#include <GL/glew.h>
#ifdef GL_TRANSITION_USING_EGL
# include <EGL/egl.h>
# include <EGL/eglext.h>
#else
# include <GLFW/glfw3.h>
#endif

// #define TS2T(ts, tb) ((ts) == AV_NOPTS_VALUE ? NAN : (double)(ts)*av_q2d(tb))

static const float position[12] = {
  -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f};

static const GLchar *v_shader_source = "\
#version 130\n\
in vec2 position;\n\
void main()\n\
{\n\
  gl_Position = vec4(position, 0.0, 1.0);\n\
}\n";

static const GLchar f_shader_source_template[] ="\
#version 130 \n\
#ifdef GL_OES_standard_derivatives\n\
#extension GL_OES_standard_derivatives : enable\n\
#endif\n\
\n\
float round( float x ) { return floor(x+0.5); }\n\
vec2 round(vec2 x) { return floor(x + 0.5); }\n\
vec3 round(vec3 x) { return floor(x + 0.5); }\n\
vec4 round(vec4 x) { return floor(x + 0.5); }\n\
float trunc( float x, float n ) { return floor(x*n)/n; }\n\
mat3 transpose(mat3 m) { return mat3(m[0].x, m[1].x, m[2].x, m[0].y, m[1].y, m[2].y, m[0].z, m[1].z, m[2].z); }\n\
float determinant( in mat2 m ) { return m[0][0]*m[1][1] - m[0][1]*m[1][0]; }\n\
float determinant( mat4 m ) { float b00 = m[0][0] * m[1][1] - m[0][1] * m[1][0], b01 = m[0][0] * m[1][2] - m[0][2] * m[1][0], b02 = m[0][0] * m[1][3] - m[0][3] * m[1][0], b03 = m[0][1] * m[1][2] - m[0][2] * m[1][1], b04 = m[0][1] * m[1][3] - m[0][3] * m[1][1], b05 = m[0][2] * m[1][3] - m[0][3] * m[1][2], b06 = m[2][0] * m[3][1] - m[2][1] * m[3][0], b07 = m[2][0] * m[3][2] - m[2][2] * m[3][0], b08 = m[2][0] * m[3][3] - m[2][3] * m[3][0], b09 = m[2][1] * m[3][2] - m[2][2] * m[3][1], b10 = m[2][1] * m[3][3] - m[2][3] * m[3][1], b11 = m[2][2] * m[3][3] - m[2][3] * m[3][2];  return b00 * b11 - b01 * b10 + b02 * b09 + b03 * b08 - b04 * b07 + b05 * b06;}\n\
mat2 inverse(mat2 m) { float det = determinant(m); return mat2(m[1][1], -m[0][1], -m[1][0], m[0][0]) / det; }\n\
mat4 inverse(mat4 m ) { float inv0 = m[1].y*m[2].z*m[3].w - m[1].y*m[2].w*m[3].z - m[2].y*m[1].z*m[3].w + m[2].y*m[1].w*m[3].z + m[3].y*m[1].z*m[2].w - m[3].y*m[1].w*m[2].z; float inv4 = -m[1].x*m[2].z*m[3].w + m[1].x*m[2].w*m[3].z + m[2].x*m[1].z*m[3].w - m[2].x*m[1].w*m[3].z - m[3].x*m[1].z*m[2].w + m[3].x*m[1].w*m[2].z; float inv8 = m[1].x*m[2].y*m[3].w - m[1].x*m[2].w*m[3].y - m[2].x  * m[1].y * m[3].w + m[2].x  * m[1].w * m[3].y + m[3].x * m[1].y * m[2].w - m[3].x * m[1].w * m[2].y; float inv12 = -m[1].x  * m[2].y * m[3].z + m[1].x  * m[2].z * m[3].y +m[2].x  * m[1].y * m[3].z - m[2].x  * m[1].z * m[3].y - m[3].x * m[1].y * m[2].z + m[3].x * m[1].z * m[2].y; float inv1 = -m[0].y*m[2].z * m[3].w + m[0].y*m[2].w * m[3].z + m[2].y  * m[0].z * m[3].w - m[2].y  * m[0].w * m[3].z - m[3].y * m[0].z * m[2].w + m[3].y * m[0].w * m[2].z; float inv5 = m[0].x  * m[2].z * m[3].w - m[0].x  * m[2].w * m[3].z - m[2].x  * m[0].z * m[3].w + m[2].x  * m[0].w * m[3].z + m[3].x * m[0].z * m[2].w - m[3].x * m[0].w * m[2].z; float inv9 = -m[0].x  * m[2].y * m[3].w +  m[0].x  * m[2].w * m[3].y + m[2].x  * m[0].y * m[3].w - m[2].x  * m[0].w * m[3].y - m[3].x * m[0].y * m[2].w + m[3].x * m[0].w * m[2].y; float inv13 = m[0].x  * m[2].y * m[3].z - m[0].x  * m[2].z * m[3].y - m[2].x  * m[0].y * m[3].z + m[2].x  * m[0].z * m[3].y + m[3].x * m[0].y * m[2].z - m[3].x * m[0].z * m[2].y; float inv2 = m[0].y  * m[1].z * m[3].w - m[0].y  * m[1].w * m[3].z - m[1].y  * m[0].z * m[3].w + m[1].y  * m[0].w * m[3].z + m[3].y * m[0].z * m[1].w - m[3].y * m[0].w * m[1].z; float inv6 = -m[0].x  * m[1].z * m[3].w + m[0].x  * m[1].w * m[3].z + m[1].x  * m[0].z * m[3].w - m[1].x  * m[0].w * m[3].z - m[3].x * m[0].z * m[1].w + m[3].x * m[0].w * m[1].z; float inv10 = m[0].x  * m[1].y * m[3].w - m[0].x  * m[1].w * m[3].y - m[1].x  * m[0].y * m[3].w + m[1].x  * m[0].w * m[3].y + m[3].x * m[0].y * m[1].w - m[3].x * m[0].w * m[1].y; float inv14 = -m[0].x  * m[1].y * m[3].z + m[0].x  * m[1].z * m[3].y + m[1].x  * m[0].y * m[3].z - m[1].x  * m[0].z * m[3].y - m[3].x * m[0].y * m[1].z + m[3].x * m[0].z * m[1].y; float inv3 = -m[0].y * m[1].z * m[2].w + m[0].y * m[1].w * m[2].z + m[1].y * m[0].z * m[2].w - m[1].y * m[0].w * m[2].z - m[2].y * m[0].z * m[1].w + m[2].y * m[0].w * m[1].z; float inv7 = m[0].x * m[1].z * m[2].w - m[0].x * m[1].w * m[2].z - m[1].x * m[0].z * m[2].w + m[1].x * m[0].w * m[2].z + m[2].x * m[0].z * m[1].w - m[2].x * m[0].w * m[1].z; float inv11 = -m[0].x * m[1].y * m[2].w + m[0].x * m[1].w * m[2].y + m[1].x * m[0].y * m[2].w - m[1].x * m[0].w * m[2].y - m[2].x * m[0].y * m[1].w + m[2].x * m[0].w * m[1].y; float inv15 = m[0].x * m[1].y * m[2].z - m[0].x * m[1].z * m[2].y - m[1].x * m[0].y * m[2].z + m[1].x * m[0].z * m[2].y + m[2].x * m[0].y * m[1].z - m[2].x * m[0].z * m[1].y; float det = m[0].x * inv0 + m[0].y * inv4 + m[0].z * inv8 + m[0].w * inv12; det = 1.0 / det; return det*mat4( inv0, inv1, inv2, inv3,inv4, inv5, inv6, inv7,inv8, inv9, inv10, inv11,inv12, inv13, inv14, inv15);}\n\
float sinh(float x)  { return (exp(x)-exp(-x))/2.; }\n\
float cosh(float x)  { return (exp(x)+exp(-x))/2.; }\n\
float tanh(float x)  { return sinh(x)/cosh(x); }\n\
float coth(float x)  { return cosh(x)/sinh(x); }\n\
float sech(float x)  { return 1./cosh(x); }\n\
float csch(float x)  { return 1./sinh(x); }\n\
float asinh(float x) { return    log(x+sqrt(x*x+1.)); }\n\
float acosh(float x) { return    log(x+sqrt(x*x-1.)); }\n\
float atanh(float x) { return .5*log((1.+x)/(1.-x)); }\n\
float acoth(float x) { return .5*log((x+1.)/(x-1.)); }\n\
float asech(float x) { return    log((1.+sqrt(1.-x*x))/x); }\n\
float acsch(float x) { return    log((1.+sqrt(1.+x*x))/x); }\n\
\n\
uniform vec3      iResolution;           // viewport resolution (in pixels)\n\
uniform float     iTime;                 // shader playback time (in seconds)\n\
uniform float     iTimeDelta;            // render time (in seconds)\n\
uniform int       iFrame;                // shader playback frame\n\
uniform float     iChannelTime[4];       // channel playback time (in seconds)\n\
uniform vec3      iChannelResolution[4]; // channel resolution (in pixels)\n\
uniform vec4      iMouse;                // mouse pixel coords. xy: current (if MLB down), zw: click\n\
uniform sampler2D iChannel0;             // input channel. XX = 2D/Cube\n\
uniform sampler2D iChannel1;\n\
uniform sampler2D iChannel2;\n\
uniform sampler2D iChannel3;\n\
uniform vec4      iDate;                 // (year, month, day, time in seconds)\n\
uniform float     iSampleRate;           // sound sample rate (i.e., 44100)\n\
uniform vec2      iOffset;               // pixel offset for tiled rendering\n\
uniform vec2      iFrameRate;            // input frame rate\n\
\n\
\n\
// Fragment shader goes here\n\
\n\
%s\n\
\n\
// end fragment shader \n\
out vec4 outColor;\n\
void main()\n\
{\n\
    mainImage(outColor, gl_FragCoord.xy + iOffset);\n\
}\n";

static GLchar fragment_shader[65535];

#define PIXEL_FORMAT GL_RGB
#ifdef GL_TRANSITION_USING_EGL
static const EGLint configAttribs[] = {
    EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
    EGL_BLUE_SIZE, 8,
    EGL_GREEN_SIZE, 8,
    EGL_RED_SIZE, 8,
    EGL_DEPTH_SIZE, 8,
    EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
    EGL_NONE};
#endif

typedef struct {
    const AVClass *class;
    double          start_play_time;
    AVRational      timebase;
    char            *shadertoy_file;
    char            *vertex_file;
    int64_t         render_start_time;
    int64_t         render_start_time_tb;
    double          render_start_time_ft;
    int64_t         duration;
    int64_t         duration_tb;
    double          duration_ft;

    GLchar          *shadertoy_file_data;
    GLchar          *vertex_file_data;
    GLint           play_time;
    GLuint          program;
    GLuint          frame_tex;
#ifdef GL_TRANSITION_USING_EGL
  EGLDisplay eglDpy;
  EGLConfig eglCfg;
  EGLSurface eglSurf;
  EGLContext eglCtx;
#else
  GLFWwindow    *window;
#endif
    GLuint          pos_buf;

    // Shadertoy vars
    GLuint  resolution;
    GLfloat timedelta;
    GLuint  frame;
    GLuint  channeltime;
    GLuint  channelresolution;
    GLuint  mouse;
    GLuint  channel0;
    GLuint  channel1;
    GLuint  channel2;
    GLuint  channel3;
    GLuint  date;
    GLuint  samplerate;
    GLuint  offset;
    int     res[3];
} shadertoyContext;


#define OFFSET(x) offsetof(shadertoyContext, x)
#define FLAGS AV_OPT_FLAG_FILTERING_PARAM|AV_OPT_FLAG_VIDEO_PARAM
static const AVOption shadertoy_options[] = {
    {"shadertoy_file",  "Required. "
                        "Path to file containing a shadertoy's code.",
                        OFFSET(shadertoy_file),
                        AV_OPT_TYPE_STRING,
                        {.str = NULL},
                        CHAR_MIN,
                        CHAR_MAX,
                        FLAGS},
    {"vertex_file", "Optional. "
                    "Path to file with custom vertex shader for the shadertoy. "
                    "By default this filter uses shadertoy webside default vertex shader.",
                    OFFSET(vertex_file),
                    AV_OPT_TYPE_STRING,
                    {.str = NULL},
                    CHAR_MIN,
                    CHAR_MAX,
                    FLAGS},
    {"start", "Optional. "
              "Starting time for the shader render, in seconds.",
              OFFSET(render_start_time),
              AV_OPT_TYPE_DURATION,
              {.i64 = 0.},
              0,
              INT64_MAX,
              FLAGS},
    {"duration",  "Optional. Render duration.",
                  OFFSET(duration),
                  AV_OPT_TYPE_DURATION,
                  {.i64 = 0.},
                  0,
                  INT64_MAX,
                  FLAGS},
    {NULL}
};

AVFILTER_DEFINE_CLASS(shadertoy);

void glfw_onError(int error, const char* description);

static GLuint build_shader(AVFilterContext *ctx, const GLchar *shader_source, GLenum type) {
    av_log(ctx, AV_LOG_DEBUG, "vf_shadertoy: build_shader (%s)\n",
          type == GL_VERTEX_SHADER ? "vertex" : "fragment");

    GLuint shader = glCreateShader(type);
    if (!shader || !glIsShader(shader)) {
        av_log(ctx, AV_LOG_ERROR, "vf_shadertoy: build_shader glCreateShader glIsShader FAILED!\n");
        return 0;
    } else {
        av_log(ctx, AV_LOG_DEBUG, "vf_shadertoy: shader created.\n");
    }

    glShaderSource(shader, 1, &shader_source, 0);
    glCompileShader(shader);
    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);

    // error message
    int InfoLogLength = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &InfoLogLength);
    if (InfoLogLength > 0) {
        char *ShaderErrorMessage = (char *)malloc(InfoLogLength);

        glGetShaderInfoLog(shader, InfoLogLength, NULL, ShaderErrorMessage);
        av_log(ctx, AV_LOG_ERROR, "vf_shadertoy: build_shader ERROR: %s\n", ShaderErrorMessage);
    }

    GLuint ret = status == GL_TRUE ? shader : 0;
    return ret;
}

static void vbo_setup(shadertoyContext *gs) {
    glGenBuffers(1, &gs->pos_buf);
    glBindBuffer(GL_ARRAY_BUFFER, gs->pos_buf);
    glBufferData(GL_ARRAY_BUFFER, sizeof(position), position, GL_STATIC_DRAW);

    GLint loc = glGetAttribLocation(gs->program, "position");
    glEnableVertexAttribArray(loc);
    glVertexAttribPointer(loc, 2, GL_FLOAT, GL_FALSE, 0, 0);
}

static void tex_setup(AVFilterLink *inlink) {
    AVFilterContext   *ctx = inlink->dst;
    shadertoyContext  *gs = ctx->priv;

    glGenTextures(1, &gs->frame_tex);
    glActiveTexture(GL_TEXTURE0);

    glBindTexture(GL_TEXTURE_2D, gs->frame_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, inlink->w, inlink->h, 0, PIXEL_FORMAT, GL_UNSIGNED_BYTE, NULL);

    glUniform1i(glGetUniformLocation(gs->program, "iChannel0"), 0);
}

static int build_program(AVFilterContext *ctx) {
    av_log(ctx, AV_LOG_DEBUG, "start vf_shadertoy build_program action\n");
    GLuint v_shader, f_shader;
    shadertoyContext *gs = ctx->priv;
    gs->shadertoy_file_data = NULL;
    gs->vertex_file_data = NULL;

    /*
     * fragments shader
     */
    if (gs->shadertoy_file) {
        av_log(ctx, AV_LOG_DEBUG, "vf_shadertoy: build_program shader params: %s\n", gs->shadertoy_file);
        FILE *f = fopen(gs->shadertoy_file, "rb");
        if (!f) {
            av_log(ctx, AV_LOG_ERROR,
                "vf_shadertoy: build_program shader: invalid shader source file \"%s\"\n", gs->shadertoy_file);
            return -1;
        }

        // get file size
        fseek(f, 0, SEEK_END);
        unsigned long fsize = ftell(f);
        fseek(f, 0, SEEK_SET);

        gs->shadertoy_file_data = malloc(fsize + 1);
        fread(gs->shadertoy_file_data, fsize, 1, f);
        fclose(f);
        gs->shadertoy_file_data[fsize] = 0;

        //av_log(ctx, AV_LOG_DEBUG, "vf_shadertoy: loaded shader: %s\n", gs->shadertoy_file_data);
        //av_log(ctx, AV_LOG_DEBUG, "vf_shadertoy: will blend shader into: %s\n", f_shader_source_template);

        // mixing input shader with shadertoy base code
        av_log(ctx, AV_LOG_DEBUG, "vf_shadertoy: sizes: \nfragment_shader: %ld\nloaded shader: %ld\ntemplate: %ld",
          sizeof(fragment_shader),
          sizeof(gs->shadertoy_file_data),
          sizeof(f_shader_source_template)
        );

        snprintf(
          &fragment_shader,
          sizeof(f_shader_source_template) + fsize + 1,
          f_shader_source_template,
          gs->shadertoy_file_data
        );


        av_log(ctx, AV_LOG_DEBUG, "vf_shadertoy: blended shader: %s\n", fragment_shader);

    } else {
        av_log(ctx, AV_LOG_ERROR, "vf_shadertoy: no shadertoy code. Impossible to continue.\n");
        return -1;
    }

    /*
     * vertex shader
     */
    if (gs->vertex_file) {
        av_log(ctx, AV_LOG_DEBUG, "vf_shadertoy: build_program vertex params: %s\n", gs->vertex_file);
        FILE *f = fopen(gs->vertex_file, "rb");
        if (!f) {
            av_log(ctx, AV_LOG_ERROR,
                "vf_shadertoy: build_program shader: invalid shader source file \"%s\"\n", gs->vertex_file);
            return -1;
        }

        // get file size
        fseek(f, 0, SEEK_END);
        unsigned long fsize = ftell(f);
        fseek(f, 0, SEEK_SET);

        gs->vertex_file_data = malloc(fsize + 1);
        fread(gs->vertex_file_data, fsize, 1, f);
        fclose(f);
        gs->vertex_file_data[fsize] = 0;

    } else {
        av_log(ctx, AV_LOG_DEBUG, "vf_shadertoy: no custom vertex, using default.\n");
    }

    const char *gl_shadertoy_file_dst = fragment_shader;
    const char *gl_vertex_file_dst = gs->vertex_file_data ? gs->vertex_file_data : v_shader_source;

    av_log(ctx, AV_LOG_DEBUG,
        "vf_shadertoy: build_program build_shader debug shaders ===================================>\n");
    av_log(ctx, AV_LOG_DEBUG,
        "vf_shadertoy: build_program build_shader fragment shaders:\n%s\n", gl_shadertoy_file_dst);
    av_log(ctx, AV_LOG_DEBUG,
        "vf_shadertoy: build_program build_shader vertex shader:\n%s\n", gl_vertex_file_dst);

    if (gs->render_start_time > 0) {
        //gs->duration_tb = TS2T(gs->duration, gs->timebase);
        gs->render_start_time_tb = av_rescale_q(gs->render_start_time, AV_TIME_BASE_Q, gs->timebase);
        gs->render_start_time_ft = TS2T(gs->render_start_time_tb, gs->timebase);
        gs->duration += gs->render_start_time;
    } else {
        gs->duration_tb = 0;
        gs->duration_ft = 0;
    }
    av_log(ctx, AV_LOG_DEBUG, "vf_shadertoy: render_start_time:%ld, render_start_time_tb:%ld, render_start_time_ft:%f\n",
           gs->render_start_time, gs->render_start_time_tb, gs->render_start_time_ft);

    if (gs->duration > 0) {
        //gs->duration_tb = TS2T(gs->duration, gs->timebase);
        gs->duration_tb = av_rescale_q(gs->duration, AV_TIME_BASE_Q, gs->timebase);
        gs->duration_ft = TS2T(gs->duration_tb, gs->timebase);
    } else {
        gs->duration_tb = -1;
        gs->duration_ft = -1;
    }
    av_log(ctx, AV_LOG_DEBUG, "vf_shadertoy: duration:%ld, duration_tb:%ld, duration_ft:%f\n",
           gs->duration, gs->duration_tb, gs->duration_ft);

    if (!((v_shader = build_shader(ctx, gl_vertex_file_dst, GL_VERTEX_SHADER)) &&
        (f_shader = build_shader(ctx, gl_shadertoy_file_dst, GL_FRAGMENT_SHADER)))) {
        av_log(ctx, AV_LOG_ERROR, "vf_shadertoy: build_program failed!\n");
        return -1;
    }

    // render shader object
    av_log(ctx, AV_LOG_DEBUG, "vf_shadertoy: build_program create program\n");
    gs->program = glCreateProgram();
    glAttachShader(gs->program, v_shader);
    glAttachShader(gs->program, f_shader);
    glLinkProgram(gs->program);

    GLint status;
    glGetProgramiv(gs->program, GL_LINK_STATUS, &status);
    if (gs->shadertoy_file_data) {
        free(gs->shadertoy_file_data);
        gs->shadertoy_file_data = NULL;
    }
    if (gs->vertex_file_data) {
        free(gs->vertex_file_data);
        gs->vertex_file_data = NULL;
    }

    av_log(ctx, AV_LOG_DEBUG, "vf_shadertoy: build_program finished\n");
    return status == GL_TRUE ? 0 : -1;
}

static void uni_setup(AVFilterLink *inLink) {
    AVFilterContext   *ctx = inLink->dst;
    shadertoyContext  *c = ctx->priv;

    // Shadertoy vars
    /*
    uniform vec3      iResolution;           // viewport resolution (in pixels)
    uniform float     iTime;                 // shader playback time (in seconds)
    uniform float     iTimeDelta;            // render time (in seconds)
    uniform int       iFrame;                // shader playback frame
    uniform float     iChannelTime[4];       // channel playback time (in seconds)
    uniform vec3      iChannelResolution[4]; // channel resolution (in pixels)
    uniform vec4      iMouse;                // mouse pixel coords. xy: current (if MLB down), zw: click
    uniform sampler2D iChannel0;             // input channel. XX = 2D/Cube
    uniform sampler2D iChannel1;
    uniform sampler2D iChannel2;
    uniform sampler2D iChannel3;
    uniform vec4      iDate;                 // (year, month, day, time in seconds)
    uniform float     iSampleRate;           // sound sample rate (i.e., 44100)
    uniform vec2      iOffset;               // pixel offset for tiled rendering
    */
    c->play_time = glGetUniformLocation(c->program, "iTime");
    glUniform1f(c->play_time, 0.0f);

    c->resolution = glGetUniformLocation(c->program, "iResolution");
    glUniform3f(c->resolution, inLink->w, inLink->h, 1.0);

    c->timedelta = glGetUniformLocation(c->program, "iTimeDelta");
    c->frame = glGetUniformLocation(c->program, "iFrame");
    c->channeltime = glGetUniformLocation(c->program, "iChannelTime");
    c->channelresolution = glGetUniformLocation(c->program, "iChannelResolution");
    c->mouse = glGetUniformLocation(c->program, "iMouse");

    c->channel0 = glGetUniformLocation(c->program, "iChannel0");
    c->channel1 = glGetUniformLocation(c->program, "iChannel1");
    c->channel2 = glGetUniformLocation(c->program, "iChannel2");
    c->channel3 = glGetUniformLocation(c->program, "iChannel3");

    c->date = glGetUniformLocation(c->program, "iDate");
    c->samplerate = glGetUniformLocation(c->program, "iSampleRate");
    c->offset = glGetUniformLocation(c->program, "iOffset");
}

static av_cold int init(AVFilterContext *ctx) {

#ifndef GL_TRANSITION_USING_EGL
  glfwSetErrorCallback(glfw_onError);
#endif
  return 0;
}

void glfw_onError(int error, const char* description)
{
    av_log(0, AV_LOG_ERROR,
           "vf_shadertoy: glfw error #%d:\n%s\n",
           error,
           description);
}


static int config_props(AVFilterLink *inlink) {
  AVFilterContext  *ctx = inlink->dst;
  shadertoyContext *gs  = ctx->priv;
  gs->start_play_time   = -1;

#ifdef GL_TRANSITION_USING_EGL
  //init EGL
  // 1. Initialize EGL
  // c->eglDpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);

  #define MAX_DEVICES 4
  EGLDeviceEXT eglDevs[MAX_DEVICES];
  EGLint numDevices;

  PFNEGLQUERYDEVICESEXTPROC eglQueryDevicesEXT =(PFNEGLQUERYDEVICESEXTPROC)
  eglGetProcAddress("eglQueryDevicesEXT");

  eglQueryDevicesEXT(MAX_DEVICES, eglDevs, &numDevices);

  PFNEGLGETPLATFORMDISPLAYEXTPROC eglGetPlatformDisplayEXT =  (PFNEGLGETPLATFORMDISPLAYEXTPROC)
  eglGetProcAddress("eglGetPlatformDisplayEXT");

  gs->eglDpy = eglGetPlatformDisplayEXT(EGL_PLATFORM_DEVICE_EXT, eglDevs[0], 0);

  EGLint major, minor;
  eglInitialize(gs->eglDpy, &major, &minor);
  av_log(ctx, AV_LOG_DEBUG, "%d%d", major, minor);
  // 2. Select an appropriate configuration
  EGLint numConfigs;
  EGLint pbufferAttribs[] = {
      EGL_WIDTH,
      inlink->w,
      EGL_HEIGHT,
      inlink->h,
      EGL_NONE,
  };
  eglChooseConfig(gs->eglDpy, configAttribs, &gs->eglCfg, 1, &numConfigs);
  // 3. Create a surface
  gs->eglSurf = eglCreatePbufferSurface(gs->eglDpy, gs->eglCfg,
                                       pbufferAttribs);
  // 4. Bind the API
  eglBindAPI(EGL_OPENGL_API);
  // 5. Create a context and make it current
  gs->eglCtx = eglCreateContext(gs->eglDpy, gs->eglCfg, EGL_NO_CONTEXT, NULL);
  eglMakeCurrent(gs->eglDpy, gs->eglSurf, gs->eglSurf, gs->eglCtx);
#else
  if (glfwInit() != GLFW_TRUE) {
    av_log(ctx, AV_LOG_ERROR, "vf_shadertoy: GLFW initialization failed\n");
    return -1;
  }

  glfwWindowHint(GLFW_VISIBLE, 0);
  gs->window = glfwCreateWindow(inlink->w, inlink->h, "", NULL, NULL);

  glfwMakeContextCurrent(gs->window);
#endif

#ifndef __APPLE__
  glewExperimental = GL_TRUE;
  glewInit();
#endif

  glViewport(0, 0, inlink->w, inlink->h);
  gs->timebase = inlink->time_base;

  int ret;
  if((ret = build_program(ctx)) < 0) {
      return ret;
  }

  glUseProgram(gs->program);
  vbo_setup(gs);
  tex_setup(inlink);
  uni_setup(inlink);
    return 0;
}

static int filter_frame(AVFilterLink *inlink, AVFrame *in) {
    AVFilterContext *ctx    = inlink->dst;
    AVFilterLink *outlink   = ctx->outputs[0];
    shadertoyContext *gs = ctx->priv;

    double play_time = TS2T(in->pts, gs->timebase);
    // check start time
    if (gs->start_play_time < 0) {
        gs->start_play_time = play_time;
    }
    play_time -= gs->start_play_time;
    av_log(ctx, AV_LOG_DEBUG,
           "vf_shadertoy: filter_frame get pts:%ld ,time->%f, duration:%f\n",
           in->pts,
           play_time,
           gs->duration_ft);

    AVFrame *out = ff_get_video_buffer(outlink, outlink->w, outlink->h);
    if (!out) {
        av_frame_free(&in);
        return AVERROR(ENOMEM);
    }

    int copy_props_ret = av_frame_copy_props(out, in);
    if (copy_props_ret < 0) {
        av_frame_free(&out);
        return -1;
    }

    #ifdef GL_TRANSITION_USING_EGL
      eglMakeCurrent(gs->eglDpy, gs->eglSurf, gs->eglSurf, gs->eglCtx);
    #else
      glfwMakeContextCurrent(gs->window);
    #endif

    glUseProgram(gs->program);

    if ( // check if render
            (gs->duration_ft < 0 || (gs->duration_ft > 0 && play_time <= gs->duration_ft))
            && play_time >= gs->render_start_time_ft)
    {
        av_log(ctx, AV_LOG_DEBUG,
               "vf_shadertoy: filter_frame gl render pts:%ld ,time->%f, duration:%f\n", in->pts, play_time, gs->duration_ft);

        glUniform1f(gs->play_time, play_time);
        glUniform1f(gs->frame, in->display_picture_number);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, inlink->w, inlink->h, 0, PIXEL_FORMAT, GL_UNSIGNED_BYTE, in->data[0]);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glReadPixels(0, 0, outlink->w, outlink->h, PIXEL_FORMAT, GL_UNSIGNED_BYTE, (GLvoid *) out->data[0]);

    } else {
        av_log(ctx, AV_LOG_DEBUG,
               "vf_shadertoy: filter_frame copy pts:%ld ,time->%f, duration:%f\n", in->pts, play_time, gs->duration_ft);
        av_frame_copy(out, in);
    }

    av_frame_free(&in);
    return ff_filter_frame(outlink, out);
}

static av_cold void uninit(AVFilterContext *ctx) {
    av_log(ctx, AV_LOG_DEBUG, "vf_shadertoy: starting uninit action\n");

    shadertoyContext *c = ctx->priv;

    av_log(ctx, AV_LOG_DEBUG, "vf_shadertoy: check window\n");
    #ifdef GL_TRANSITION_USING_EGL
      if (c->eglDpy) {
        av_log(ctx, AV_LOG_DEBUG, "vf_shadertoy: gldelete operations\n");
        glDeleteTextures(1, &c->frame_tex);
        glDeleteBuffers(1, &c->pos_buf);
        glDeleteProgram(c->program);
        eglTerminate(c->eglDpy);
      } else {
               av_log(ctx, AV_LOG_DEBUG, "vf_shadertoy: no window, don't need gldelete operations\n");
           }
    #else
    if (c->window) { // @new
        av_log(ctx, AV_LOG_DEBUG, "vf_shadertoy: gldelete operations\n");
        glDeleteTextures(1, &c->frame_tex);
        glDeleteBuffers(1, &c->pos_buf);
        glDeleteProgram(c->program);
        glfwDestroyWindow(c->window);
    } else {
        av_log(ctx, AV_LOG_DEBUG, "vf_shadertoy: no window, don't need gldelete operations\n");
    }
    #endif

    av_log(ctx, AV_LOG_DEBUG, "vf_shadertoy: finished\n");

}


static int query_formats(AVFilterContext *ctx) {
  static const enum AVPixelFormat formats[] = {AV_PIX_FMT_RGB24, AV_PIX_FMT_NONE};
  return ff_set_common_formats(ctx, ff_make_format_list(formats));
}

static const AVFilterPad shadertoy_inputs[] = {
    {
        .name = "default",
        .type = AVMEDIA_TYPE_VIDEO,
        .config_props = config_props,
        .filter_frame = filter_frame
    }
};

static const AVFilterPad shadertoy_outputs[] = {
  {
    .name = "default",
    .type = AVMEDIA_TYPE_VIDEO
  }
};

AVFilter ff_vf_shadertoy = {
    .name          = "shadertoy",
    .description   = NULL_IF_CONFIG_SMALL("Applies a shadertoy using ffmpeg filter"),
    .priv_size     = sizeof(shadertoyContext),
    .init          = init,
    .uninit        = uninit,
    .priv_class    = &shadertoy_class,
    .flags         = AVFILTER_FLAG_SUPPORT_TIMELINE_GENERIC,
    FILTER_QUERY_FUNC(query_formats),
    FILTER_INPUTS(shadertoy_inputs),
    FILTER_OUTPUTS(shadertoy_outputs),
};
