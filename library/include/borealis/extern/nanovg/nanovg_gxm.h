//
// Copyright (c) 2024 xfangfang xfangfang@126.com
//
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//
#ifndef NANOVG_GXM_H
#define NANOVG_GXM_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "nanovg_gxm_utils.h"

    // Create flags
    enum NVGcreateFlags
    {
        // Flag indicating if geometry based anti-aliasing is used (may not be needed when using MSAA).
        NVG_ANTIALIAS = 1 << 0,
        // Flag indicating if strokes should be drawn using stencil buffer. The rendering will be a little
        // slower, but path overlaps (i.e. self-intersecting or sharp turns) will be drawn just once.
        NVG_STENCIL_STROKES = 1 << 1,
        // Flag indicating that additional debug checks are done.
        NVG_DEBUG = 1 << 2,
    };

    NVGcontext* nvgCreateGXM(NVGXMframebuffer* fb, int flags);

    void nvgDeleteGXM(NVGcontext* ctx);

    // These are additional flags on top of NVGimageFlags.
    enum NVGimageFlagsGXM
    {
        NVG_IMAGE_NODELETE = 1 << 16, // Do not delete GXM texture handle.
    };

    int __attribute__((weak)) nvg_gxm_vertex_buffer_size = 1024 * 1024;

#ifdef __cplusplus
}
#endif

#endif /* NANOVG_GXM_H */

#ifdef NANOVG_GXM_IMPLEMENTATION

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nanovg.h"

#ifdef USE_VITA_SHARK

#include <vitashark.h>

static void shark_log_cb(const char* msg, shark_log_level msg_level, int line)
{
    switch (msg_level)
    {
        case SHARK_LOG_INFO:
            sceClibPrintf("\033[0;34m[GXP #%d]\033[0m %s\n", line, msg);
            break;
        case SHARK_LOG_WARNING:
            sceClibPrintf("\033[0;33m[GXP #%d]\033[0m %s\n", line, msg);
            break;
        case SHARK_LOG_ERROR:
            sceClibPrintf("\033[0;31m[GXP #%d]\033[0m %s\n", line, msg);
            break;
    }
}

#endif

enum GXMNVGuniformLoc
{
    GXMNVG_LOC_VIEWSIZE,
    GXMNVG_LOC_FRAG,
    GXMNVG_MAX_LOCS
};

enum GXMNVGshaderType
{
    NSVG_SHADER_FILLGRAD,
    NSVG_SHADER_FILLIMG,
    NSVG_SHADER_SIMPLE,
    NSVG_SHADER_IMG
};

struct GXMNVGshader
{
    NVGXMshaderProgram prog;
    const SceGxmProgramParameter* loc[GXMNVG_MAX_LOCS];
};
typedef struct GXMNVGshader GXMNVGshader;

struct GXMNVGtexture
{
    int id;
    int width, height;
    int type;
    int flags;

    int stride;
    int unused;
    SceGxmTexture gxm_tex;
    uint8_t* tex_data;
    SceUID data_UID;
};
typedef struct GXMNVGtexture GXMNVGtexture;

struct GXMNVGblend
{
    SceGxmBlendFactor srcRGB;
    SceGxmBlendFactor dstRGB;
    SceGxmBlendFactor srcAlpha;
    SceGxmBlendFactor dstAlpha;
};
typedef struct GXMNVGblend GXMNVGblend;

enum GXMNVGcallType
{
    GXMNVG_NONE = 0,
    GXMNVG_FILL,
    GXMNVG_CONVEXFILL,
    GXMNVG_STROKE,
    GXMNVG_TRIANGLES,
};

struct GXMNVGcall
{
    int type;
    int image;
    int pathOffset;
    int pathCount;
    int triangleOffset;
    int triangleCount;
    int uniformOffset;
    GXMNVGblend blendFunc;
};
typedef struct GXMNVGcall GXMNVGcall;

struct GXMNVGpath
{
    int fillOffset;
    int fillCount;
    int strokeOffset;
    int strokeCount;
};
typedef struct GXMNVGpath GXMNVGpath;

struct GXMNVGfragUniforms
{
// note: after modifying layout or size of uniform array,
// don't forget to also update the fragment shader source!
#define NANOVG_GXM_UNIFORMARRAY_SIZE 11
    union
    {
        struct
        {
            float scissorMat[12]; // matrices are actually float3x4
            float paintMat[12];
            struct NVGcolor innerCol;
            struct NVGcolor outerCol;
            float scissorExt[2];
            float scissorScale[2];
            float extent[2];
            float radius;
            float feather;
            float strokeMult;
            float strokeThr;
            float texType;
            float type;
        };
        float uniformArray[NANOVG_GXM_UNIFORMARRAY_SIZE][4];
    };
};
typedef struct GXMNVGfragUniforms GXMNVGfragUniforms;

struct GXMNVGcontext
{
    SceGxmContext* context;
    SceGxmShaderPatcher* shader_patcher;
    SceGxmMultisampleMode msaa;

    GXMNVGshader shader;
    SceUID verticesUid;
    struct NVGvertex* vertBuf;

    GXMNVGshader depth_shader;

    GXMNVGtexture* textures;
    float view[2];
    int ntextures;
    int ctextures;
    int textureId;
    int fragSize;
    int flags;

    // Per frame buffers
    GXMNVGcall* calls;
    int ccalls;
    int ncalls;
    GXMNVGpath* paths;
    int cpaths;
    int npaths;
    struct NVGvertex* verts;
    int cverts;
    int nverts;
    unsigned char* uniforms;
    int cuniforms;
    int nuniforms;

    int dummyTex;
};
typedef struct GXMNVGcontext GXMNVGcontext;

static void gxmDrawArrays(GXMNVGcontext* gxm, SceGxmPrimitiveType type, int fillOffset, int fillCount)
{
    if (fillCount > UINT16_MAX)
    {
        return;
    }

    static int index = 0;
    if (index + fillCount > nvg_gxm_vertex_buffer_size)
    {
        index = 0;
    }

    memcpy(&gxm->vertBuf[index], &gxm->verts[fillOffset], sizeof(NVGvertex) * fillCount);
    GXM_CHECK_VOID(sceGxmSetVertexStream(gxm->context, 0, &gxm->vertBuf[index]));
    GXM_CHECK_VOID(sceGxmDraw(gxm->context, type, SCE_GXM_INDEX_FORMAT_U16, gxmGetSharedIndices(), fillCount));

    index += fillCount;
}

static int gxmnvg__maxi(int a, int b) { return a > b ? a : b; }

static unsigned int gxmnvg__nearestPow2(unsigned int num)
{
    unsigned n = num > 0 ? num - 1 : 0;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n++;
    return n;
}

static void
gxmnvg__stencilFunc(GXMNVGcontext* gxm, SceGxmStencilFunc func, SceGxmStencilOp stencilFail, SceGxmStencilOp depthFail,
    SceGxmStencilOp depthPass)
{
    sceGxmSetFrontStencilFunc(gxm->context, func, stencilFail, depthFail, depthPass, 0xff, 0xff);
    sceGxmSetBackStencilFunc(gxm->context, func, stencilFail, depthFail, depthPass, 0xff, 0xff);
}

static void gxmnvg__disableStencilTest(GXMNVGcontext* gxm)
{
    gxmnvg__stencilFunc(gxm, SCE_GXM_STENCIL_FUNC_ALWAYS,
        SCE_GXM_STENCIL_OP_KEEP, SCE_GXM_STENCIL_OP_KEEP, SCE_GXM_STENCIL_OP_KEEP);
}

// static void gxmnvg__blendFuncSeparate(GXMNVGcontext *gxm, const GXMNVGblend *blend) {}

static GXMNVGtexture* gxmnvg__allocTexture(GXMNVGcontext* gxm)
{
    GXMNVGtexture* tex = NULL;
    int i;

    for (i = 0; i < gxm->ntextures; i++)
    {
        if (gxm->textures[i].id == 0)
        {
            tex = &gxm->textures[i];
            break;
        }
    }
    if (tex == NULL)
    {
        if (gxm->ntextures + 1 > gxm->ctextures)
        {
            GXMNVGtexture* textures;
            int ctextures = gxmnvg__maxi(gxm->ntextures + 1, 4) + gxm->ctextures / 2; // 1.5x Overallocate
            textures      = (GXMNVGtexture*)realloc(gxm->textures, sizeof(GXMNVGtexture) * ctextures);
            if (textures == NULL)
                return NULL;
            gxm->textures  = textures;
            gxm->ctextures = ctextures;
        }
        tex = &gxm->textures[gxm->ntextures++];
    }

    memset(tex, 0, sizeof(*tex));
    tex->id = ++gxm->textureId;

    return tex;
}

static GXMNVGtexture* gxmnvg__findTexture(GXMNVGcontext* gxm, int id)
{
    int i;
    for (i = 0; i < gxm->ntextures; i++)
        if (gxm->textures[i].id == id)
            return &gxm->textures[i];
    return NULL;
}

static int gxmnvg__deleteTexture(GXMNVGcontext* gxm, int id)
{
    int i;
    for (i = 0; i < gxm->ntextures; i++)
    {
        if (gxm->textures[i].id == id)
        {
            if (gxm->textures[i].data_UID != 0 && (gxm->textures[i].flags & NVG_IMAGE_NODELETE) == 0)
            {
                gxm->textures[i].unused = 1;
            }
            return 1;
        }
    }
    return 0;
}

static int gxmnvg__garbageCollector(GXMNVGcontext* gxm)
{
    int i;
    for (i = 0; i < gxm->ntextures; i++)
    {
        if (gxm->textures[i].unused == 0)
            continue;
        gpu_unmap_free(gxm->textures[i].data_UID);
        memset(&gxm->textures[i], 0, sizeof(gxm->textures[i]));
    }
    return 0;
}

static int gxmnvg__createShader(GXMNVGshader* shader, const char* name, const char* vshader, const char* fshader)
{
    return gxmCreateShader(&shader->prog, name, vshader, fshader);
}

static void gxmnvg__deleteShader(GXMNVGshader* shader)
{
    gxmDeleteShader(&shader->prog);
}

static void gxmnvg__getUniforms(GXMNVGshader* shader)
{
    shader->loc[GXMNVG_LOC_VIEWSIZE] = sceGxmProgramFindParameterByName(shader->prog.vert_gxp, "viewSize");
    shader->loc[GXMNVG_LOC_FRAG]     = sceGxmProgramFindParameterByName(shader->prog.frag_gxp, "frag");
}

static int gxmnvg__renderCreateTexture(void* uptr, int type, int w, int h, int imageFlags, const unsigned char* data);

static int gxmnvg__renderCreate(void* uptr)
{
    GXMNVGcontext* gxm = (GXMNVGcontext*)uptr;
    int align          = 4;

#if USE_VITA_SHARK
    char fillVertShader[500] = "struct VS_OUTPUT\n"
                               "{\n"
                               "    float4 position   : POSITION;\n"
                               "    float2 ftcoord    : TEXCOORD0;\n"
                               "    float2 fpos       : TEXCOORD1;\n"
                               "};\n"
                               "void main(\n"
                               "   float2 vertex : POSITION,\n"
                               "   float2 tcoord : TEXCOORD0,\n"
                               "   uniform float2 viewSize,\n"
                               "   out VS_OUTPUT output\n"
                               ")\n"
                               "{\n"
                               "   output.ftcoord = tcoord;\n"
                               "   output.fpos = vertex;\n"
                               "   output.position = float4(2.0 * vertex.x / viewSize.x - 1.0, 1.0 - 2.0 * vertex.y / viewSize.y, 1.0f, 1.0f);\n"
                               "}\n";

    char fillFragShader[2500] = "#define EDGE_AA 0\n"
                                "#define UNIFORMARRAY_SIZE 11\n"
                                "uniform float4 frag[UNIFORMARRAY_SIZE];\n"
                                "#define scissorMat float3x3(frag[0].xyz, frag[1].xyz, frag[2].xyz)\n"
                                "#define paintMat float3x3(frag[3].xyz, frag[4].xyz, frag[5].xyz)\n"
                                "#define innerCol frag[6]\n"
                                "#define outerCol frag[7]\n"
                                "#define scissorExt frag[8].xy\n"
                                "#define scissorScale frag[8].zw\n"
                                "#define extent frag[9].xy\n"
                                "#define radius frag[9].z\n"
                                "#define feather frag[9].w\n"
                                "#define strokeMult frag[10].x\n"
                                "#define strokeThr frag[10].y\n"
                                "#define texType frag[10].z\n"
                                "#define type frag[10].w\n"
                                "float sdroundrect(float2 pt, float2 ext, float rad)\n"
                                "{\n"
                                "    float2 ext2 = ext - float2(rad,rad);\n"
                                "    float2 d = abs(pt) - ext2;\n"
                                "    return min(max(d.x,d.y),0.0) + length(max(d,0.0)) - rad;\n"
                                "}\n"
                                "float scissorMask(float2 p) {\n"
                                "   float2 sc = (abs((mul(scissorMat, float3(p,1.0))).xy) - scissorExt);\n"
                                "   sc = float2(0.5,0.5) - sc * scissorScale;\n"
                                "   return clamp(sc.x,0.0,1.0) * clamp(sc.y,0.0,1.0);\n"
                                "}\n"
                                "#if EDGE_AA\n" // Stroke - from [0..1] to clipped pyramid, where the slope is 1px.
                                "float strokeMask(float2 ftcoord)\n"
                                "{\n"
                                "    return min(1.0, (1.0 - abs(ftcoord.x*2.0 - 1.0))*strokeMult) * min(1.0f, ftcoord.y);\n"
                                "}\n"
                                "#endif\n"
                                "float4 main(\n"
                                "   uniform sampler2D tex : TEXUNIT0,\n"
                                "   float2 ftcoord: TEXCOORD0,\n"
                                "   float2 fpos: TEXCOORD1\n"
                                ") : COLOR\n"
                                "{\n"
                                "   float4 result;\n"
                                "   float scissor = scissorMask(fpos);\n"
                                "#if EDGE_AA\n"
                                "    float strokeAlpha = strokeMask(ftcoord);\n"
                                "    if (strokeAlpha < strokeThr) discard;\n"
                                "#else\n"
                                "    float strokeAlpha = 1.0f;\n"
                                "#endif\n"
                                "   if (type == 0.0f) {\n" // Gradient
                                "       float2 pt = (mul(paintMat, float3(fpos,1.0))).xy;\n"
                                "       float d = clamp((sdroundrect(pt, extent, radius) + feather*0.5) / feather, 0.0, 1.0);\n"
                                "       float4 color = lerp(innerCol, outerCol, d);\n"
                                "       color *= strokeAlpha * scissor;\n" // Combine alpha
                                "       result = color;\n"
                                "   } else if (type == 1.0f) {\n" // Image
                                "       float2 pt = (mul(paintMat, float3(fpos,1.0))).xy / extent.xy;\n"
                                "       float4 color = tex2D(tex, pt);\n"
                                "       if (texType == 1.0f) color = float4(color.xyz*color.w, color.w);\n"
                                "       if (texType == 2.0f) color = float4(color.x, color.x, color.x, color.x);\n"
                                "       color *= innerCol;\n"
                                "       color *= strokeAlpha * scissor;\n"
                                "       result = color;\n"
                                "   } else if (type == 2.0f) {\n" // Stencil fill
                                "       result = float4(1.0f, 1.0f, 1.0f, 1.0f);\n"
                                "   } else {\n" // Textured tris
                                "       float4 color = tex2D(tex, ftcoord);\n"
                                "       if (texType == 1.0f) color = float4(color.xyz*color.w, color.w);\n"
                                "       if (texType == 2.0f) color = float4(color.x, color.x, color.x, color.x);\n"
                                "       color *= scissor;\n"
                                "       result = (color * innerCol);\n"
                                "   }\n"
                                "   return result;\n"
                                "}\n";

    char depthFragShader[20] = "void main() {}";

    if (gxm->flags & NVG_ANTIALIAS)
    {
        fillFragShader[16] = '1'; // #define EDGE_AA 1
        if (gxmnvg__createShader(&gxm->shader, "fillAA", fillVertShader, fillFragShader) == 0)
            return 0;
    }
    else
    {
        if (gxmnvg__createShader(&gxm->shader, "fill", fillVertShader, fillFragShader) == 0)
            return 0;
    }
#else
    static const unsigned char fillVertShader[384] = {
        0x47, 0x58, 0x50, 0x00, 0x01, 0x05, 0x00, 0x03, 0x7f, 0x01, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x19, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x10, 0x01, 0x00, 0x00,
        0x70, 0x00, 0x00, 0x00, 0x08, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x01, 0x00, 0x09, 0x00, 0x00, 0x00, 0x90, 0x00,
        0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x74, 0x00, 0x00, 0x00, 0x80,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x02, 0x00, 0x00, 0x00, 0xc0, 0x00, 0x00, 0x00, 0x90, 0x3a,
        0x03, 0x00, 0x02, 0x00, 0x00, 0x00, 0xa4, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0xbc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xa4, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x9c, 0x00, 0x00,
        0x00, 0x02, 0x00, 0x00, 0x00, 0x94, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x33, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x08,
        0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x02, 0x00, 0x00, 0x80, 0x0a, 0x00, 0x80, 0x30, 0x01, 0x00,
        0x00, 0x80, 0x02, 0x00, 0x84, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x07, 0x44, 0xfa, 0x80, 0x00, 0x08, 0x83,
        0x21, 0x05, 0x80, 0x38, 0x40, 0x80, 0x64, 0xe0, 0x82, 0x41, 0x84,
        0x08, 0x01, 0x10, 0x40, 0xf0, 0x8e, 0x00, 0x80, 0x00, 0x01, 0x10,
        0x54, 0xf0, 0x26, 0x01, 0x80, 0x00, 0x41, 0x00, 0x04, 0x90, 0x85,
        0x11, 0xa5, 0x08, 0x01, 0x80, 0x56, 0x90, 0x81, 0x11, 0x83, 0x08,
        0x00, 0x00, 0x0c, 0x83, 0x21, 0x05, 0x80, 0x38, 0x00, 0x00, 0x20,
        0xa0, 0x00, 0x50, 0x27, 0xfb, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x80, 0x3f, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x0e,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x13, 0x00, 0x00, 0x00,
        0x02, 0x00, 0x02, 0x00, 0x30, 0x00, 0x00, 0x00, 0x00, 0x04, 0x0b,
        0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x27, 0x00,
        0x00, 0x00, 0x00, 0x04, 0x0e, 0x00, 0x01, 0x00, 0x00, 0x00, 0x04,
        0x00, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00, 0x01, 0xe2, 0x00, 0x00,
        0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x76, 0x65, 0x72,
        0x74, 0x65, 0x78, 0x00, 0x74, 0x63, 0x6f, 0x6f, 0x72, 0x64, 0x00,
        0x76, 0x69, 0x65, 0x77, 0x53, 0x69, 0x7a, 0x65, 0x00, 0x00
    };

    static const unsigned char fillFragShader[1184] = {
        0x47, 0x58, 0x50, 0x00, 0x01, 0x05, 0x00, 0x03, 0x9d, 0x04, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x10,
        0x18, 0x00, 0x00, 0x00, 0x00, 0x10, 0x02, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x2c, 0x04, 0x00, 0x00,
        0x70, 0x00, 0x00, 0x00, 0x04, 0x00, 0x36, 0x00, 0x05, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x01, 0x00, 0x5f, 0x00, 0x00, 0x00, 0xdc, 0x00,
        0x00, 0x00, 0x09, 0x00, 0x00, 0x00, 0x88, 0x00, 0x00, 0x00, 0xcc,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x2c, 0x00, 0x00, 0x00, 0xcc, 0x03, 0x00, 0x00, 0x90, 0x3a,
        0x03, 0x00, 0x04, 0x00, 0x00, 0x00, 0xa0, 0x03, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0xd8, 0x03, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
        0xb0, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xb8, 0x03, 0x00,
        0x00, 0x02, 0x00, 0x00, 0x00, 0xb0, 0x03, 0x00, 0x00, 0xdc, 0x03,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x01, 0x04, 0x02, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
        0x0f, 0x00, 0x40, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x10, 0x40, 0x0e, 0x00, 0x00,
        0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40,
        0x00, 0x68, 0x43, 0x0a, 0x05, 0x82, 0x38, 0x01, 0x09, 0x80, 0x82,
        0x02, 0x00, 0x80, 0x30, 0x02, 0x09, 0x80, 0x82, 0x0a, 0x00, 0x80,
        0x30, 0xd2, 0x14, 0xc0, 0xa2, 0xa2, 0x41, 0x80, 0x08, 0xd9, 0x84,
        0xa4, 0xa2, 0x82, 0x00, 0x84, 0x08, 0x82, 0x09, 0x20, 0x80, 0x0a,
        0x00, 0x80, 0x30, 0x0e, 0x13, 0x04, 0xa1, 0xa6, 0x41, 0xa4, 0x08,
        0x4f, 0x13, 0x44, 0xa1, 0xaa, 0x41, 0xc0, 0x08, 0x00, 0x00, 0x00,
        0x00, 0x40, 0x01, 0x04, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x07, 0x44, 0xfa, 0x80, 0x8a, 0x03, 0xd0, 0x91,
        0xc1, 0xc9, 0x48, 0x02, 0x80, 0x81, 0xff, 0x9c, 0x0d, 0x80, 0x40,
        0x01, 0xd2, 0x11, 0x80, 0x80, 0x88, 0x81, 0x18, 0x06, 0x82, 0xa1,
        0xff, 0x9c, 0x0d, 0x80, 0x40, 0x01, 0xd2, 0x11, 0x90, 0x00, 0x89,
        0x81, 0x18, 0x22, 0x90, 0xb9, 0xff, 0xbc, 0x0d, 0xc0, 0x40, 0x00,
        0x11, 0x1d, 0x00, 0xb4, 0x81, 0xa1, 0x18, 0x0c, 0xd0, 0x0f, 0x10,
        0xa0, 0x11, 0xa1, 0x00, 0x40, 0xd6, 0x24, 0xc0, 0x84, 0x41, 0xc0,
        0x08, 0x00, 0x60, 0x00, 0x40, 0x80, 0x41, 0x82, 0x08, 0x00, 0x00,
        0x80, 0x00, 0x80, 0x10, 0x80, 0x08, 0x80, 0x18, 0x00, 0xe0, 0x00,
        0x10, 0x81, 0x91, 0x00, 0x18, 0x00, 0xe0, 0x08, 0x00, 0x81, 0x55,
        0x30, 0x00, 0x0a, 0x30, 0x81, 0x02, 0xa8, 0x48, 0x39, 0x00, 0x00,
        0x00, 0x40, 0x00, 0x00, 0xf9, 0x82, 0x8a, 0x03, 0xd0, 0x91, 0xc1,
        0x89, 0x48, 0x80, 0x18, 0x00, 0xe0, 0x00, 0x10, 0x81, 0x91, 0x00,
        0x18, 0x00, 0xe0, 0x08, 0x00, 0x81, 0x55, 0x30, 0x00, 0x0a, 0x30,
        0x81, 0x02, 0xa8, 0x48, 0x1c, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00,
        0xf9, 0x84, 0x8a, 0x03, 0xd0, 0x91, 0xc1, 0x89, 0x48, 0x80, 0x18,
        0x40, 0xe0, 0x02, 0x10, 0x81, 0x91, 0x00, 0x18, 0x40, 0xe0, 0x0a,
        0x00, 0x81, 0x55, 0x30, 0x01, 0x0a, 0xb0, 0x81, 0x02, 0xa8, 0x48,
        0x14, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0xf9, 0x82, 0x8a, 0x03,
        0xd0, 0x81, 0x81, 0x89, 0x48, 0x00, 0x0b, 0x00, 0xe0, 0x04, 0xc4,
        0x01, 0xe0, 0x80, 0x18, 0x00, 0xe0, 0x02, 0x10, 0x81, 0x91, 0x00,
        0x18, 0x00, 0xe0, 0x0a, 0x00, 0x81, 0x55, 0x30, 0x00, 0x0a, 0xb0,
        0x81, 0x02, 0xa8, 0x48, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20,
        0xf9, 0x03, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0xfd, 0x40, 0x80,
        0x24, 0x00, 0x80, 0x41, 0x84, 0x08, 0x41, 0x00, 0x44, 0x00, 0x88,
        0x10, 0xc0, 0x08, 0x84, 0x8a, 0x03, 0xd0, 0x81, 0x81, 0x89, 0x48,
        0x80, 0x18, 0x00, 0xe0, 0x02, 0x10, 0x81, 0x91, 0x00, 0x18, 0x00,
        0xe0, 0x0a, 0x00, 0x81, 0x55, 0x30, 0x00, 0x0a, 0xb0, 0x81, 0x02,
        0x88, 0x48, 0x00, 0x00, 0x00, 0x03, 0x00, 0x05, 0x80, 0x39, 0x00,
        0x00, 0x04, 0x03, 0x20, 0x0d, 0x80, 0x39, 0x02, 0x00, 0x04, 0x0f,
        0x84, 0x0f, 0xa4, 0x08, 0x0c, 0x81, 0x11, 0xc0, 0x82, 0x81, 0xe1,
        0x18, 0x0d, 0x0f, 0x4d, 0x30, 0x82, 0x41, 0x80, 0x08, 0x32, 0x00,
        0x00, 0x00, 0x40, 0x00, 0x00, 0xf8, 0x34, 0x1a, 0x03, 0xf0, 0x8e,
        0x0d, 0x80, 0x40, 0x34, 0x1a, 0x43, 0xf0, 0x8e, 0x0d, 0x80, 0x40,
        0x2f, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0xf8, 0x82, 0x8a, 0x03,
        0xd0, 0x81, 0x81, 0xc9, 0x48, 0x0e, 0x86, 0x81, 0xff, 0x9c, 0x0d,
        0x00, 0x40, 0x01, 0xd2, 0x11, 0x80, 0x82, 0x88, 0x01, 0x18, 0x12,
        0x88, 0xa1, 0xff, 0x9c, 0x0d, 0x00, 0x40, 0x01, 0xd2, 0x11, 0x90,
        0x02, 0x81, 0x01, 0x18, 0x14, 0x00, 0x04, 0xb0, 0x86, 0x41, 0x24,
        0x08, 0x00, 0x0b, 0x00, 0xe0, 0x04, 0xc4, 0x01, 0xe0, 0x80, 0x18,
        0x00, 0xe0, 0x02, 0x10, 0x81, 0x91, 0x00, 0x18, 0x00, 0xe0, 0x0a,
        0x00, 0x81, 0x55, 0x30, 0x00, 0x0a, 0xb0, 0x81, 0x02, 0xa8, 0x48,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0xf9, 0x03, 0x00, 0x00,
        0x00, 0x40, 0x00, 0x00, 0xfd, 0x40, 0x80, 0x24, 0x00, 0x80, 0x41,
        0x84, 0x08, 0x41, 0x00, 0x44, 0x00, 0x88, 0x10, 0xc0, 0x08, 0x84,
        0x8a, 0x03, 0xd0, 0x81, 0x81, 0x89, 0x48, 0x80, 0x18, 0x00, 0xe0,
        0x02, 0x10, 0x81, 0x91, 0x00, 0x18, 0x00, 0xe0, 0x0a, 0x00, 0x81,
        0x55, 0x30, 0x00, 0x0a, 0xb0, 0x81, 0x02, 0xc8, 0x48, 0x02, 0x80,
        0x99, 0x0f, 0xbc, 0x0d, 0xc0, 0x40, 0x00, 0x0f, 0xf0, 0x0f, 0x00,
        0x0d, 0x80, 0x39, 0x3c, 0x03, 0x04, 0xcf, 0x84, 0x4f, 0xa4, 0x08,
        0x02, 0x80, 0x11, 0x00, 0x82, 0x81, 0xe1, 0x18, 0x02, 0x0f, 0x4d,
        0x00, 0x82, 0x01, 0x80, 0x08, 0x17, 0x00, 0x00, 0x00, 0x40, 0x00,
        0x00, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x40, 0x09, 0x00, 0xf8, 0x0e,
        0x86, 0x81, 0xff, 0x9c, 0x0d, 0x80, 0x40, 0x01, 0xd2, 0x11, 0x80,
        0x82, 0x88, 0x81, 0x18, 0x12, 0x88, 0xa1, 0xff, 0x9c, 0x0d, 0x80,
        0x40, 0x01, 0xd2, 0x11, 0x90, 0x02, 0x81, 0x81, 0x18, 0xc0, 0x12,
        0x04, 0xe0, 0xb6, 0x49, 0xa4, 0x08, 0x00, 0x60, 0x04, 0xaf, 0x84,
        0x18, 0xa4, 0x08, 0x00, 0x5f, 0x44, 0x1f, 0x84, 0x08, 0xa5, 0x08,
        0x00, 0x60, 0x04, 0x9f, 0x84, 0x09, 0xa5, 0x08, 0x3c, 0x42, 0x3e,
        0x0f, 0x80, 0x88, 0x81, 0x18, 0x01, 0x3e, 0x80, 0x0f, 0x00, 0x0a,
        0x80, 0x30, 0x01, 0x3e, 0x80, 0x0f, 0x00, 0x08, 0x80, 0x30, 0x3d,
        0x00, 0x1c, 0x0f, 0x84, 0x88, 0xa1, 0x18, 0xfc, 0x14, 0x10, 0xcf,
        0xa8, 0x08, 0xc0, 0x08, 0x0a, 0x00, 0x1c, 0xcf, 0x84, 0x88, 0xa1,
        0x18, 0x41, 0x80, 0x01, 0xcf, 0x80, 0x88, 0xe1, 0x18, 0x7c, 0xd6,
        0x28, 0xcf, 0x84, 0x08, 0xc0, 0x08, 0x3c, 0x60, 0x00, 0x4f, 0x80,
        0x08, 0x82, 0x08, 0x1a, 0x8c, 0xb9, 0xff, 0xbc, 0x0d, 0xc0, 0x40,
        0x04, 0x11, 0x01, 0xcf, 0x80, 0x8f, 0xb1, 0x18, 0x02, 0x80, 0x11,
        0x00, 0x82, 0x81, 0xe1, 0x18, 0x02, 0x0f, 0x4d, 0x00, 0x82, 0x01,
        0x80, 0x08, 0x02, 0x80, 0x19, 0xa0, 0x7e, 0x0d, 0x80, 0x40, 0x04,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00,
        0x01, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x3f, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3f, 0x00, 0x00,
        0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x02, 0x00, 0x02, 0x00, 0x03,
        0x00, 0x03, 0x00, 0x0e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2c, 0x00,
        0x13, 0x00, 0x00, 0x00, 0x2c, 0x00, 0x08, 0x00, 0x40, 0x00, 0x00,
        0x00, 0x01, 0xe4, 0x00, 0x00, 0x0b, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x35, 0x00, 0x00, 0x00, 0x02, 0x04, 0x01, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x03, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x66, 0x72, 0x61, 0x67, 0x00,
        0x74, 0x65, 0x78, 0x00, 0x00, 0x00, 0x00
    };

    static const unsigned char fillAAFragShader[1324] = {
        0x47, 0x58, 0x50, 0x00, 0x01, 0x05, 0x00, 0x03, 0x29, 0x05, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x09, 0x10,
        0x18, 0x00, 0x00, 0x00, 0x00, 0x10, 0x02, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0xb8, 0x04, 0x00, 0x00,
        0x70, 0x00, 0x00, 0x00, 0x04, 0x00, 0x3a, 0x00, 0x07, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x02, 0x00, 0x6e, 0x00, 0x00, 0x00, 0xe0, 0x00,
        0x00, 0x00, 0x09, 0x00, 0x00, 0x00, 0x88, 0x00, 0x00, 0x00, 0xcc,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x2c, 0x00, 0x00, 0x00, 0x58, 0x04, 0x00, 0x00, 0x90, 0x3a,
        0x03, 0x00, 0x06, 0x00, 0x00, 0x00, 0x1c, 0x04, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x64, 0x04, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
        0x3c, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x44, 0x04, 0x00,
        0x00, 0x02, 0x00, 0x00, 0x00, 0x3c, 0x04, 0x00, 0x00, 0x68, 0x04,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x01, 0x04, 0x02, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
        0x0f, 0x00, 0x40, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x10, 0x40, 0x0e, 0x00, 0x00,
        0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40,
        0x00, 0x70, 0x43, 0x0a, 0x05, 0x82, 0x38, 0x01, 0x09, 0x60, 0x83,
        0x02, 0x00, 0x80, 0x30, 0x02, 0x09, 0x60, 0x83, 0x0a, 0x00, 0x80,
        0x30, 0xd2, 0x14, 0xc0, 0xa2, 0xa2, 0x41, 0x80, 0x08, 0xd9, 0x84,
        0xa4, 0xa2, 0x82, 0x10, 0x84, 0x08, 0x82, 0x09, 0x20, 0x80, 0x0a,
        0x00, 0x80, 0x30, 0x0e, 0x13, 0x04, 0xa1, 0xa6, 0x41, 0xa4, 0x08,
        0x4f, 0x13, 0x44, 0xa1, 0xaa, 0x41, 0xc0, 0x08, 0x00, 0x00, 0x00,
        0x00, 0x40, 0x01, 0x04, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x0d, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x44, 0xfa, 0x9a,
        0x06, 0x00, 0xf0, 0x8c, 0x28, 0x80, 0x00, 0x1a, 0x10, 0x04, 0x3f,
        0xe4, 0x10, 0xa4, 0x08, 0x14, 0x80, 0x01, 0xc0, 0x80, 0x80, 0xe1,
        0x18, 0x80, 0x00, 0x20, 0xa0, 0x08, 0x00, 0x81, 0x50, 0x80, 0xd6,
        0x24, 0xc0, 0x84, 0x41, 0xc0, 0x08, 0x00, 0x00, 0xc0, 0x00, 0x80,
        0x10, 0x80, 0x08, 0x03, 0x8a, 0x00, 0xc0, 0x91, 0xc6, 0x8c, 0x48,
        0x00, 0x19, 0x00, 0xe0, 0x00, 0x10, 0x81, 0x91, 0x80, 0x16, 0x00,
        0xe0, 0x08, 0x00, 0x81, 0x55, 0x2d, 0x00, 0x0a, 0x30, 0x85, 0x01,
        0x88, 0x48, 0x2c, 0x16, 0x00, 0xf0, 0x06, 0x04, 0x30, 0xf9, 0x00,
        0x00, 0x00, 0x00, 0x40, 0x01, 0x04, 0xf8, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x07, 0x44, 0xfa, 0x80, 0x8a, 0x03, 0xd0, 0x91, 0xc1, 0xc9,
        0x48, 0x02, 0x80, 0x81, 0xff, 0x9c, 0x0d, 0x80, 0x40, 0x01, 0xd2,
        0x11, 0x80, 0x80, 0x88, 0x81, 0x18, 0x06, 0x82, 0xa1, 0xff, 0x9c,
        0x0d, 0x80, 0x40, 0x01, 0xd2, 0x11, 0x90, 0x00, 0x89, 0x81, 0x18,
        0x22, 0x90, 0xb9, 0xff, 0xbc, 0x0d, 0xc0, 0x40, 0x00, 0x11, 0x1d,
        0x00, 0xb4, 0x81, 0xa1, 0x18, 0x0c, 0xd0, 0x0f, 0x10, 0xa0, 0x11,
        0xa1, 0x00, 0x80, 0xd6, 0x24, 0xc0, 0x84, 0x41, 0xc0, 0x08, 0x00,
        0x60, 0x00, 0x40, 0x80, 0x41, 0x82, 0x08, 0x00, 0x00, 0x80, 0x00,
        0x80, 0x10, 0x80, 0x08, 0x00, 0x19, 0x00, 0xe0, 0x00, 0x10, 0x81,
        0x91, 0x80, 0x16, 0x00, 0xe0, 0x08, 0x00, 0x81, 0x55, 0x2d, 0x00,
        0x0a, 0x30, 0x81, 0x02, 0xa8, 0x48, 0x3a, 0x00, 0x00, 0x00, 0x40,
        0x00, 0x00, 0xf9, 0x82, 0x8a, 0x03, 0xd0, 0x91, 0xc1, 0x89, 0x48,
        0x00, 0x19, 0x00, 0xe0, 0x00, 0x10, 0x81, 0x91, 0x80, 0x16, 0x00,
        0xe0, 0x08, 0x00, 0x81, 0x55, 0x2d, 0x00, 0x0a, 0x30, 0x81, 0x02,
        0xa8, 0x48, 0x1c, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0xf9, 0x84,
        0x8a, 0x03, 0xd0, 0x91, 0xc1, 0x89, 0x48, 0x00, 0x19, 0x40, 0xe0,
        0x02, 0x10, 0x81, 0x91, 0x80, 0x16, 0x40, 0xe0, 0x0a, 0x00, 0x81,
        0x55, 0x2d, 0x01, 0x0a, 0xb0, 0x81, 0x02, 0xa8, 0x48, 0x14, 0x00,
        0x00, 0x00, 0x40, 0x00, 0x00, 0xf9, 0x82, 0x8a, 0x03, 0xd0, 0x81,
        0x81, 0x89, 0x48, 0x80, 0x0b, 0x00, 0xe0, 0x04, 0xc4, 0x01, 0xe0,
        0x00, 0x19, 0x00, 0xe0, 0x02, 0x10, 0x81, 0x91, 0x80, 0x16, 0x00,
        0xe0, 0x0a, 0x00, 0x81, 0x55, 0x2d, 0x00, 0x0a, 0xb0, 0x81, 0x02,
        0xa8, 0x48, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0xf9, 0x03,
        0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0xfd, 0x40, 0x80, 0x24, 0x00,
        0x80, 0x41, 0x84, 0x08, 0x41, 0x00, 0x44, 0x00, 0x88, 0x10, 0xc0,
        0x08, 0x84, 0x8a, 0x03, 0xd0, 0x81, 0x81, 0x89, 0x48, 0x00, 0x19,
        0x00, 0xe0, 0x02, 0x10, 0x81, 0x91, 0x80, 0x16, 0x00, 0xe0, 0x0a,
        0x00, 0x81, 0x55, 0x2d, 0x00, 0x0a, 0xb0, 0x81, 0x02, 0x88, 0x48,
        0x00, 0x00, 0x00, 0x03, 0x00, 0x05, 0x80, 0x39, 0x00, 0x00, 0x04,
        0x03, 0x20, 0x0d, 0x80, 0x39, 0x02, 0x00, 0x04, 0x0f, 0x84, 0x0f,
        0xa4, 0x08, 0x0c, 0x81, 0x11, 0xc0, 0x82, 0x81, 0xe1, 0x18, 0x0d,
        0x0f, 0x4d, 0x30, 0x82, 0x41, 0x80, 0x08, 0x34, 0x00, 0x00, 0x00,
        0x40, 0x00, 0x00, 0xf8, 0x38, 0x1c, 0x03, 0xf0, 0x8e, 0x0d, 0x80,
        0x40, 0x38, 0x1c, 0x43, 0xf0, 0x8e, 0x0d, 0x80, 0x40, 0x31, 0x00,
        0x00, 0x00, 0x40, 0x00, 0x00, 0xf8, 0x82, 0x8a, 0x03, 0xd0, 0x81,
        0x81, 0xc9, 0x48, 0x0e, 0x86, 0x81, 0xff, 0x9c, 0x0d, 0x00, 0x40,
        0x01, 0xd2, 0x11, 0x80, 0x82, 0x88, 0x01, 0x18, 0x12, 0x88, 0xa1,
        0xff, 0x9c, 0x0d, 0x00, 0x40, 0x01, 0xd2, 0x11, 0x90, 0x02, 0x81,
        0x01, 0x18, 0x1b, 0x00, 0x04, 0xb0, 0x86, 0x41, 0x24, 0x08, 0x80,
        0x0b, 0x00, 0xe0, 0x04, 0xc4, 0x01, 0xe0, 0x00, 0x19, 0x00, 0xe0,
        0x02, 0x10, 0x81, 0x91, 0x80, 0x16, 0x00, 0xe0, 0x0a, 0x00, 0x81,
        0x55, 0x2d, 0x00, 0x0a, 0xb0, 0x81, 0x02, 0xa8, 0x48, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x20, 0xf9, 0x03, 0x00, 0x00, 0x00, 0x40,
        0x00, 0x00, 0xfd, 0x40, 0x80, 0x24, 0x00, 0x80, 0x41, 0x84, 0x08,
        0x41, 0x00, 0x44, 0x00, 0x88, 0x10, 0xc0, 0x08, 0x84, 0x8a, 0x03,
        0xd0, 0x81, 0x81, 0x89, 0x48, 0x00, 0x19, 0x00, 0xe0, 0x02, 0x10,
        0x81, 0x91, 0x80, 0x16, 0x00, 0xe0, 0x0a, 0x00, 0x81, 0x55, 0x2d,
        0x00, 0x0a, 0xb0, 0x81, 0x02, 0xc8, 0x48, 0x02, 0x80, 0x99, 0x0f,
        0xbc, 0x0d, 0xc0, 0x40, 0x00, 0x0f, 0xf0, 0x0f, 0x00, 0x0d, 0x80,
        0x39, 0x3c, 0x03, 0x04, 0xcf, 0x84, 0x4f, 0xa4, 0x08, 0xc2, 0x00,
        0x40, 0x0f, 0x80, 0x08, 0x80, 0x08, 0x3c, 0x81, 0x01, 0x10, 0x82,
        0x81, 0xe1, 0x18, 0x3d, 0x0f, 0x4d, 0x00, 0x82, 0x01, 0x80, 0x08,
        0x18, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0xf8, 0x00, 0x00, 0x00,
        0x00, 0x40, 0x09, 0x00, 0xf8, 0x0e, 0x86, 0x81, 0xff, 0x9c, 0x0d,
        0x80, 0x40, 0x01, 0xd2, 0x11, 0x80, 0x82, 0x88, 0x81, 0x18, 0x12,
        0x88, 0xa1, 0xff, 0x9c, 0x0d, 0x80, 0x40, 0x01, 0xd2, 0x11, 0x90,
        0x02, 0x81, 0x81, 0x18, 0xc0, 0x12, 0x04, 0xe0, 0xb6, 0x49, 0xa4,
        0x08, 0x00, 0x60, 0x04, 0xaf, 0x84, 0x18, 0xa4, 0x08, 0x00, 0x5f,
        0x44, 0x1f, 0x84, 0x08, 0xa5, 0x08, 0x00, 0x60, 0x04, 0x9f, 0x84,
        0x09, 0xa5, 0x08, 0x3c, 0x42, 0x3e, 0x0f, 0x80, 0x88, 0x81, 0x18,
        0x01, 0x3e, 0x80, 0x0f, 0x00, 0x0a, 0x80, 0x30, 0x01, 0x3e, 0x80,
        0x0f, 0x00, 0x08, 0x80, 0x30, 0x3d, 0x00, 0x1c, 0x0f, 0x84, 0x88,
        0xa1, 0x18, 0xfc, 0x14, 0x10, 0xcf, 0xa8, 0x08, 0xc0, 0x08, 0x0a,
        0x00, 0x1c, 0xcf, 0x84, 0x88, 0xa1, 0x18, 0x41, 0x80, 0x01, 0xcf,
        0x80, 0x88, 0xe1, 0x18, 0xbc, 0xd6, 0x28, 0xcf, 0x84, 0x08, 0xc0,
        0x08, 0x3c, 0x60, 0x00, 0x4f, 0x80, 0x08, 0x82, 0x08, 0x1a, 0x8c,
        0xb9, 0xff, 0xbc, 0x0d, 0xc0, 0x40, 0x04, 0x11, 0x01, 0xcf, 0x80,
        0x8f, 0xb1, 0x18, 0xc2, 0x00, 0x40, 0x0f, 0x80, 0x08, 0x80, 0x08,
        0x3c, 0x81, 0x01, 0x10, 0x82, 0x81, 0xe1, 0x18, 0x3d, 0x0f, 0x4d,
        0x00, 0x82, 0x01, 0x80, 0x08, 0x02, 0x80, 0x19, 0xa0, 0x7e, 0x0d,
        0x80, 0x40, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x06,
        0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x3f, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x40, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x3f, 0x00, 0x00,
        0x00, 0x00, 0x00, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x01,
        0x00, 0x03, 0x00, 0x02, 0x00, 0x04, 0x00, 0x03, 0x00, 0x05, 0x00,
        0x0e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2c, 0x00, 0x13, 0x00, 0x00,
        0x00, 0x2c, 0x00, 0x0a, 0x00, 0x40, 0x00, 0x00, 0x00, 0x01, 0xe4,
        0x00, 0x00, 0x0b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x35,
        0x00, 0x00, 0x00, 0x02, 0x04, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x01, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x66, 0x72, 0x61, 0x67, 0x00, 0x74, 0x65, 0x78,
        0x00, 0x00, 0x00, 0x00
    };

    static const unsigned char depthFragShader[188] = {
        0x47, 0x58, 0x50, 0x00, 0x01, 0x05, 0x00, 0x03, 0xbc, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00,
        0x19, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x94, 0x00, 0x00, 0x00,
        0x70, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x74, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x68, 0x00, 0x00, 0x00, 0x64,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x54, 0x00, 0x00, 0x00, 0x90, 0x3a,
        0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x48, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x07, 0x04, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x44,
        0xfa
    };

    if (gxm->flags & NVG_ANTIALIAS)
    {
        if (gxmnvg__createShader(&gxm->shader, "fillAA", (const char*)fillVertShader, (const char*)fillAAFragShader) == 0)
            return 0;
    }
    else
    {
        if (gxmnvg__createShader(&gxm->shader, "fill", (const char*)fillVertShader, (const char*)fillFragShader) == 0)
            return 0;
    }
#endif

    if (gxmnvg__createShader(&gxm->depth_shader, "depth", NULL, (const char*)depthFragShader) == 0)
        return 0;

    gxm->vertBuf = (struct NVGvertex*)gpu_alloc_map(
        SCE_KERNEL_MEMBLOCK_TYPE_USER_RW_UNCACHE,
        SCE_GXM_MEMORY_ATTRIB_READ,
        sizeof(struct NVGvertex) * nvg_gxm_vertex_buffer_size,
        &gxm->verticesUid);

    const SceGxmProgramParameter* basic_vertex_param = sceGxmProgramFindParameterByName(gxm->shader.prog.vert_gxp,
        "vertex");
    const SceGxmProgramParameter* basic_tcoord_param = sceGxmProgramFindParameterByName(gxm->shader.prog.vert_gxp,
        "tcoord");
    SceGxmVertexAttribute basic_vertex_attributes[2];
    basic_vertex_attributes[0].streamIndex    = 0;
    basic_vertex_attributes[0].offset         = 0;
    basic_vertex_attributes[0].format         = SCE_GXM_ATTRIBUTE_FORMAT_F32;
    basic_vertex_attributes[0].componentCount = 2;
    basic_vertex_attributes[0].regIndex       = sceGxmProgramParameterGetResourceIndex(basic_vertex_param);
    basic_vertex_attributes[1].streamIndex    = 0;
    basic_vertex_attributes[1].offset         = 2 * sizeof(float);
    basic_vertex_attributes[1].format         = SCE_GXM_ATTRIBUTE_FORMAT_F32;
    basic_vertex_attributes[1].componentCount = 2;
    basic_vertex_attributes[1].regIndex       = sceGxmProgramParameterGetResourceIndex(basic_tcoord_param);
    SceGxmVertexStream basic_vertex_stream[1];
    basic_vertex_stream[0].stride      = sizeof(struct NVGvertex);
    basic_vertex_stream[0].indexSource = SCE_GXM_INDEX_SOURCE_INDEX_16BIT;

    GXM_CHECK(sceGxmShaderPatcherCreateVertexProgram(gxm->shader_patcher, gxm->shader.prog.vert_id,
        basic_vertex_attributes,
        sizeof(basic_vertex_attributes) / sizeof(SceGxmVertexAttribute),
        basic_vertex_stream,
        sizeof(basic_vertex_stream) / sizeof(SceGxmVertexStream),
        &gxm->shader.prog.vert));

    /**
     * TODO: Custom blend function
     * Currently, these functions in nanovg.h are not supported:
     * nvgGlobalCompositeOperation, nvgGlobalCompositeBlendFunc, nvgGlobalCompositeBlendFuncSeparate
     * The default behavior is equivalent to: nvgGlobalCompositeOperation(NVG_SOURCE_OVER)
     */
    SceGxmBlendInfo blendInfo;
    blendInfo.colorMask = SCE_GXM_COLOR_MASK_ALL;
    blendInfo.colorFunc = SCE_GXM_BLEND_FUNC_ADD;
    blendInfo.alphaFunc = SCE_GXM_BLEND_FUNC_ADD;
    blendInfo.colorSrc  = SCE_GXM_BLEND_FACTOR_ONE;
    blendInfo.colorDst  = SCE_GXM_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blendInfo.alphaSrc  = SCE_GXM_BLEND_FACTOR_ONE;
    blendInfo.alphaDst  = SCE_GXM_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;

    GXM_CHECK(sceGxmShaderPatcherCreateFragmentProgram(gxm->shader_patcher,
        gxm->shader.prog.frag_id, SCE_GXM_OUTPUT_REGISTER_FORMAT_UCHAR4,
        gxm->msaa, &blendInfo, gxm->shader.prog.vert_gxp,
        &gxm->shader.prog.frag));

    gxmnvg__getUniforms(&gxm->shader);

    GXM_CHECK(sceGxmShaderPatcherCreateFragmentProgram(gxm->shader_patcher,
        gxm->depth_shader.prog.frag_id,
        SCE_GXM_OUTPUT_REGISTER_FORMAT_UCHAR4,
        gxm->msaa, NULL, gxm->shader.prog.vert_gxp,
        &gxm->depth_shader.prog.frag));

    gxm->fragSize = ALIGN(sizeof(GXMNVGfragUniforms), align);

    // Some platforms does not allow to have samples to unset textures.
    // Create empty one which is bound when there's no texture specified.
    gxm->dummyTex = gxmnvg__renderCreateTexture(gxm, NVG_TEXTURE_ALPHA, 1, 1, 0, NULL);

    return 1;
}

static int gxmnvg__renderCreateTexture(void* uptr, int type, int w, int h, int imageFlags, const unsigned char* data)
{
    GXMNVGcontext* gxm = (GXMNVGcontext*)uptr;
    GXMNVGtexture* tex = gxmnvg__allocTexture(gxm);

    if (tex == NULL)
        return 0;

    SceGxmTextureFormat format = type == NVG_TEXTURE_RGBA ? SCE_GXM_TEXTURE_FORMAT_U8U8U8U8_ABGR : SCE_GXM_TEXTURE_FORMAT_U8_R;
    int aligned_w              = ALIGN(w, 8);
    int texture_w              = w;
    int spp                    = type == NVG_TEXTURE_RGBA ? 4 : 1;
    int tex_size               = aligned_w * h * spp;
    int ret;

    tex->stride   = aligned_w * spp;
    tex->tex_data = (uint8_t*)gpu_alloc_map(SCE_KERNEL_MEMBLOCK_TYPE_USER_RW_UNCACHE, SCE_GXM_MEMORY_ATTRIB_RW,
        tex_size, &tex->data_UID);
    if (tex->tex_data == NULL)
    {
        return 0;
    }

    /* Clear the texture */
    if (data == NULL)
    {
        memset(tex->tex_data, 0, tex_size);
    }
    else
    {
        for (int i = 0; i < h; i++)
        {
            memcpy(tex->tex_data + i * tex->stride, data + i * w * spp, w * spp);
        }
    }

    // TODO: Support mipmap
    imageFlags &= ~NVG_IMAGE_GENERATE_MIPMAPS;

    /* Create the gxm texture */
    ret = sceGxmTextureInitLinear(&tex->gxm_tex, tex->tex_data, format, texture_w, h, 0);
    if (ret < 0)
    {
        GXM_PRINT_ERROR(ret);
        gpu_unmap_free(tex->data_UID);
        tex->data_UID = 0;
        return 0;
    }

    if (imageFlags & NVG_IMAGE_GENERATE_MIPMAPS)
    {
        if (imageFlags & NVG_IMAGE_NEAREST)
        {
            sceGxmTextureSetMinFilter(&tex->gxm_tex, SCE_GXM_TEXTURE_FILTER_MIPMAP_POINT);
        }
        else
        {
            sceGxmTextureSetMinFilter(&tex->gxm_tex, SCE_GXM_TEXTURE_FILTER_MIPMAP_LINEAR);
        }
    }
    else
    {
        if (imageFlags & NVG_IMAGE_NEAREST)
        {
            sceGxmTextureSetMinFilter(&tex->gxm_tex, SCE_GXM_TEXTURE_FILTER_POINT);
        }
        else
        {
            sceGxmTextureSetMinFilter(&tex->gxm_tex, SCE_GXM_TEXTURE_FILTER_LINEAR);
        }
    }

    if (imageFlags & NVG_IMAGE_NEAREST)
        sceGxmTextureSetMagFilter(&tex->gxm_tex, SCE_GXM_TEXTURE_FILTER_POINT);
    else
        sceGxmTextureSetMagFilter(&tex->gxm_tex, SCE_GXM_TEXTURE_FILTER_LINEAR);

    if (imageFlags & NVG_IMAGE_REPEATX)
        sceGxmTextureSetUAddrMode(&tex->gxm_tex, SCE_GXM_TEXTURE_ADDR_REPEAT);
    else
        sceGxmTextureSetUAddrMode(&tex->gxm_tex, SCE_GXM_TEXTURE_ADDR_CLAMP);

    if (imageFlags & NVG_IMAGE_REPEATY)
        sceGxmTextureSetVAddrMode(&tex->gxm_tex, SCE_GXM_TEXTURE_ADDR_REPEAT);
    else
        sceGxmTextureSetVAddrMode(&tex->gxm_tex, SCE_GXM_TEXTURE_ADDR_CLAMP);

    tex->width  = w;
    tex->height = h;
    tex->type   = type;
    tex->flags  = imageFlags;

    return tex->id;
}

static int gxmnvg__renderDeleteTexture(void* uptr, int image)
{
    GXMNVGcontext* gxm = (GXMNVGcontext*)uptr;
    return gxmnvg__deleteTexture(gxm, image);
}

static int gxmnvg__renderUpdateTexture(void* uptr, int image, int x, int y, int w, int h, const unsigned char* data)
{
    GXMNVGcontext* gxm = (GXMNVGcontext*)uptr;
    GXMNVGtexture* tex = gxmnvg__findTexture(gxm, image);

    if (tex == NULL)
        return 0;

    int spp = tex->type == NVG_TEXTURE_RGBA ? 4 : 1;

    for (int i = 0; i < h; i++)
    {
        int start = ((i + y) * tex->stride + x) * spp;
        memcpy(tex->tex_data + start, data + start, w * spp);
    }

    return 1;
}

static int gxmnvg__renderGetTextureSize(void* uptr, int image, int* w, int* h)
{
    GXMNVGcontext* gxm = (GXMNVGcontext*)uptr;
    GXMNVGtexture* tex = gxmnvg__findTexture(gxm, image);
    if (tex == NULL)
        return 0;
    *w = tex->width;
    *h = tex->height;
    return 1;
}

static void gxmnvg__xformToMat3x4(float* m3, float* t)
{
    // transpose
    m3[0]  = t[0];
    m3[1]  = t[2];
    m3[2]  = t[4];
    m3[3]  = 0.0f;
    m3[4]  = t[1];
    m3[5]  = t[3];
    m3[6]  = t[5];
    m3[7]  = 0.0f;
    m3[8]  = 0.0f;
    m3[9]  = 0.0f;
    m3[10] = 1.0f;
    m3[11] = 0.0f;
}

static NVGcolor gxmnvg__premulColor(NVGcolor c)
{
    c.r *= c.a;
    c.g *= c.a;
    c.b *= c.a;
    return c;
}

static int gxmnvg__convertPaint(GXMNVGcontext* gxm, GXMNVGfragUniforms* frag, NVGpaint* paint,
    NVGscissor* scissor, float width, float fringe, float strokeThr)
{
    GXMNVGtexture* tex = NULL;
    float invxform[6];

    memset(frag, 0, sizeof(*frag));

    frag->innerCol = gxmnvg__premulColor(paint->innerColor);
    frag->outerCol = gxmnvg__premulColor(paint->outerColor);

    if (scissor->extent[0] < -0.5f || scissor->extent[1] < -0.5f)
    {
        memset(frag->scissorMat, 0, sizeof(frag->scissorMat));
        frag->scissorExt[0]   = 1.0f;
        frag->scissorExt[1]   = 1.0f;
        frag->scissorScale[0] = 1.0f;
        frag->scissorScale[1] = 1.0f;
    }
    else
    {
        nvgTransformInverse(invxform, scissor->xform);
        gxmnvg__xformToMat3x4(frag->scissorMat, invxform);
        frag->scissorExt[0]   = scissor->extent[0];
        frag->scissorExt[1]   = scissor->extent[1];
        frag->scissorScale[0] = sqrtf(scissor->xform[0] * scissor->xform[0] + scissor->xform[2] * scissor->xform[2]) / fringe;
        frag->scissorScale[1] = sqrtf(scissor->xform[1] * scissor->xform[1] + scissor->xform[3] * scissor->xform[3]) / fringe;
    }

    memcpy(frag->extent, paint->extent, sizeof(frag->extent));
    frag->strokeMult = (width * 0.5f + fringe * 0.5f) / fringe;
    frag->strokeThr  = strokeThr;

    if (paint->image != 0)
    {
        tex = gxmnvg__findTexture(gxm, paint->image);
        if (tex == NULL)
            return 0;
        if ((tex->flags & NVG_IMAGE_FLIPY) != 0)
        {
            float m1[6], m2[6];
            nvgTransformTranslate(m1, 0.0f, frag->extent[1] * 0.5f);
            nvgTransformMultiply(m1, paint->xform);
            nvgTransformScale(m2, 1.0f, -1.0f);
            nvgTransformMultiply(m2, m1);
            nvgTransformTranslate(m1, 0.0f, -frag->extent[1] * 0.5f);
            nvgTransformMultiply(m1, m2);
            nvgTransformInverse(invxform, m1);
        }
        else
        {
            nvgTransformInverse(invxform, paint->xform);
        }
        frag->type = NSVG_SHADER_FILLIMG;

        if (tex->type == NVG_TEXTURE_RGBA)
            frag->texType = (tex->flags & NVG_IMAGE_PREMULTIPLIED) ? 0.0f : 1.0f;
        else
            frag->texType = 2.0f;
    }
    else
    {
        frag->type    = NSVG_SHADER_FILLGRAD;
        frag->radius  = paint->radius;
        frag->feather = paint->feather;
        nvgTransformInverse(invxform, paint->xform);
    }

    gxmnvg__xformToMat3x4(frag->paintMat, invxform);
    return 1;
}

static GXMNVGfragUniforms* nvg__fragUniformPtr(GXMNVGcontext* gxm, int i);

static void gxmnvg__setUniforms(GXMNVGcontext* gxm, int uniformOffset, int image)
{
    GXMNVGtexture* tex       = NULL;
    GXMNVGfragUniforms* frag = nvg__fragUniformPtr(gxm, uniformOffset);

    void* buffer;
    sceGxmReserveFragmentDefaultUniformBuffer(gxm->context, &buffer);
    sceGxmSetUniformDataF(buffer, gxm->shader.loc[GXMNVG_LOC_FRAG], 0, sizeof(float) * NANOVG_GXM_UNIFORMARRAY_SIZE,
        (const float*)frag->uniformArray);

    if (image != 0)
    {
        tex = gxmnvg__findTexture(gxm, image);
    }
    // If no image is set, use empty texture
    if (tex == NULL)
    {
        tex = gxmnvg__findTexture(gxm, gxm->dummyTex);
    }
    if (tex != NULL)
    {
        GXM_CHECK_VOID(sceGxmSetFragmentTexture(gxm->context, 0, &tex->gxm_tex));
    }
}

static void gxmnvg__renderViewport(void* uptr, float width, float height, float devicePixelRatio)
{
    NVG_NOTUSED(devicePixelRatio);
    GXMNVGcontext* gxm = (GXMNVGcontext*)uptr;
    gxm->view[0]       = width;
    gxm->view[1]       = height;
}

static void gxmnvg__fill(GXMNVGcontext* gxm, GXMNVGcall* call)
{
    GXMNVGpath* paths = &gxm->paths[call->pathOffset];
    int i, npaths = call->pathCount;

    // set bindpoint for solid loc
    gxmnvg__setUniforms(gxm, call->uniformOffset, 0);

    // Draw shapes
    {
        // Disable color output
        sceGxmSetFragmentProgram(gxm->context, gxm->depth_shader.prog.frag);

        sceGxmSetTwoSidedEnable(gxm->context, SCE_GXM_TWO_SIDED_ENABLED);
        sceGxmSetCullMode(gxm->context, SCE_GXM_CULL_NONE);
        sceGxmSetFrontStencilFunc(gxm->context,
            SCE_GXM_STENCIL_FUNC_ALWAYS,
            SCE_GXM_STENCIL_OP_KEEP, SCE_GXM_STENCIL_OP_KEEP, SCE_GXM_STENCIL_OP_INCR_WRAP,
            0xff, 0xff);
        sceGxmSetBackStencilFunc(gxm->context,
            SCE_GXM_STENCIL_FUNC_ALWAYS,
            SCE_GXM_STENCIL_OP_KEEP, SCE_GXM_STENCIL_OP_KEEP, SCE_GXM_STENCIL_OP_DECR_WRAP,
            0xff, 0xff);

        for (i = 0; i < npaths; i++)
            gxmDrawArrays(gxm, SCE_GXM_PRIMITIVE_TRIANGLE_FAN, paths[i].fillOffset, paths[i].fillCount);

        sceGxmSetCullMode(gxm->context, SCE_GXM_CULL_CW);
        sceGxmSetTwoSidedEnable(gxm->context, SCE_GXM_TWO_SIDED_DISABLED);

        // Enable color output
        sceGxmSetFragmentProgram(gxm->context, gxm->shader.prog.frag);
    }

    // Draw anti-aliased pixels
    gxmnvg__setUniforms(gxm, call->uniformOffset + gxm->fragSize, call->image);

    if (gxm->flags & NVG_ANTIALIAS)
    {
        gxmnvg__stencilFunc(gxm, SCE_GXM_STENCIL_FUNC_EQUAL,
            SCE_GXM_STENCIL_OP_KEEP, SCE_GXM_STENCIL_OP_KEEP, SCE_GXM_STENCIL_OP_KEEP);

        // Draw fringes
        for (i = 0; i < npaths; i++)
            gxmDrawArrays(gxm, SCE_GXM_PRIMITIVE_TRIANGLE_STRIP, paths[i].strokeOffset, paths[i].strokeCount);
    }

    // Draw fill
    gxmnvg__stencilFunc(gxm, SCE_GXM_STENCIL_FUNC_NOT_EQUAL,
        SCE_GXM_STENCIL_OP_ZERO, SCE_GXM_STENCIL_OP_ZERO, SCE_GXM_STENCIL_OP_ZERO);

    gxmDrawArrays(gxm, SCE_GXM_PRIMITIVE_TRIANGLE_STRIP, call->triangleOffset, call->triangleCount);

    gxmnvg__disableStencilTest(gxm);
}

static void gxmnvg__convexFill(GXMNVGcontext* gxm, GXMNVGcall* call)
{
    GXMNVGpath* paths = &gxm->paths[call->pathOffset];
    int i, npaths = call->pathCount;

    gxmnvg__setUniforms(gxm, call->uniformOffset, call->image);

    for (i = 0; i < npaths; i++)
    {
        gxmDrawArrays(gxm, SCE_GXM_PRIMITIVE_TRIANGLE_FAN, paths[i].fillOffset, paths[i].fillCount);

        // Draw fringes
        if (paths[i].strokeCount > 0)
        {
            gxmDrawArrays(gxm, SCE_GXM_PRIMITIVE_TRIANGLE_STRIP, paths[i].strokeOffset, paths[i].strokeCount);
        }
    }
}

static void gxmnvg__stroke(GXMNVGcontext* gxm, GXMNVGcall* call)
{
    GXMNVGpath* paths = &gxm->paths[call->pathOffset];
    int npaths        = call->pathCount, i;

    if (gxm->flags & NVG_STENCIL_STROKES)
    {
        gxmnvg__stencilFunc(gxm, SCE_GXM_STENCIL_FUNC_EQUAL,
            SCE_GXM_STENCIL_OP_KEEP, SCE_GXM_STENCIL_OP_KEEP, SCE_GXM_STENCIL_OP_INCR);

        gxmnvg__setUniforms(gxm, call->uniformOffset + gxm->fragSize, call->image);

        for (i = 0; i < npaths; i++)
            gxmDrawArrays(gxm, SCE_GXM_PRIMITIVE_TRIANGLE_STRIP, paths[i].strokeOffset, paths[i].strokeCount);

        // Draw anti-aliased pixels.
        gxmnvg__setUniforms(gxm, call->uniformOffset, call->image);

        sceGxmSetFrontStencilFunc(gxm->context,
            SCE_GXM_STENCIL_FUNC_EQUAL,
            SCE_GXM_STENCIL_OP_KEEP, SCE_GXM_STENCIL_OP_KEEP, SCE_GXM_STENCIL_OP_KEEP,
            0xff, 0xff);
        sceGxmSetBackStencilFunc(gxm->context,
            SCE_GXM_STENCIL_FUNC_EQUAL,
            SCE_GXM_STENCIL_OP_KEEP, SCE_GXM_STENCIL_OP_KEEP, SCE_GXM_STENCIL_OP_KEEP,
            0xff, 0xff);

        for (i = 0; i < npaths; i++)
            gxmDrawArrays(gxm, SCE_GXM_PRIMITIVE_TRIANGLE_STRIP, paths[i].strokeOffset, paths[i].strokeCount);

        // Clear stencil buffer.
        {
            gxmnvg__stencilFunc(gxm, SCE_GXM_STENCIL_FUNC_ALWAYS,
                SCE_GXM_STENCIL_OP_ZERO, SCE_GXM_STENCIL_OP_ZERO, SCE_GXM_STENCIL_OP_ZERO);

            // Disable color output
            sceGxmSetFragmentProgram(gxm->context, gxm->depth_shader.prog.frag);

            for (i = 0; i < npaths; i++)
                gxmDrawArrays(gxm, SCE_GXM_PRIMITIVE_TRIANGLE_STRIP, paths[i].strokeOffset, paths[i].strokeCount);

            // Enable color output
            sceGxmSetFragmentProgram(gxm->context, gxm->shader.prog.frag);

            gxmnvg__disableStencilTest(gxm);
        }

        //		gxmnvg__convertPaint(gxm, nvg__fragUniformPtr(gxm, call->uniformOffset + gl->fragSize), paint, scissor, strokeWidth, fringe, 1.0f - 0.5f/255.0f);
    }
    else
    {
        gxmnvg__setUniforms(gxm, call->uniformOffset, call->image);
        // Draw Strokes
        for (i = 0; i < npaths; i++)
        {
            gxmDrawArrays(gxm, SCE_GXM_PRIMITIVE_TRIANGLE_STRIP, paths[i].strokeOffset, paths[i].strokeCount);
        }
    }
}

static void gxmnvg__triangles(GXMNVGcontext* gxm, GXMNVGcall* call)
{
    gxmnvg__setUniforms(gxm, call->uniformOffset, call->image);
    gxmDrawArrays(gxm, SCE_GXM_PRIMITIVE_TRIANGLES, call->triangleOffset, call->triangleCount);
}

static void gxmnvg__renderCancel(void* uptr)
{
    GXMNVGcontext* gxm = (GXMNVGcontext*)uptr;
    gxm->nverts        = 0;
    gxm->npaths        = 0;
    gxm->ncalls        = 0;
    gxm->nuniforms     = 0;
}

static SceGxmBlendFactor gxmnvg_convertBlendFuncFactor(int factor)
{
    switch (factor)
    {
        case NVG_ZERO:
            return SCE_GXM_BLEND_FACTOR_ZERO;
        case NVG_ONE:
            return SCE_GXM_BLEND_FACTOR_ONE;
        case NVG_SRC_COLOR:
            return SCE_GXM_BLEND_FACTOR_SRC_COLOR;
        case NVG_ONE_MINUS_SRC_COLOR:
            return SCE_GXM_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
        case NVG_DST_COLOR:
            return SCE_GXM_BLEND_FACTOR_DST_COLOR;
        case NVG_ONE_MINUS_DST_COLOR:
            return SCE_GXM_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
        case NVG_SRC_ALPHA:
            return SCE_GXM_BLEND_FACTOR_SRC_ALPHA;
        case NVG_ONE_MINUS_SRC_ALPHA:
            return SCE_GXM_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        case NVG_DST_ALPHA:
            return SCE_GXM_BLEND_FACTOR_DST_ALPHA;
        case NVG_ONE_MINUS_DST_ALPHA:
            return SCE_GXM_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
        case NVG_SRC_ALPHA_SATURATE:
            return SCE_GXM_BLEND_FACTOR_SRC_ALPHA_SATURATE;
    }

    // act like invalid
    return SCE_GXM_BLEND_FACTOR_DST_ALPHA_SATURATE;
}

static GXMNVGblend gxmnvg__blendCompositeOperation(NVGcompositeOperationState op)
{
    GXMNVGblend blend;
    blend.srcRGB   = gxmnvg_convertBlendFuncFactor(op.srcRGB);
    blend.dstRGB   = gxmnvg_convertBlendFuncFactor(op.dstRGB);
    blend.srcAlpha = gxmnvg_convertBlendFuncFactor(op.srcAlpha);
    blend.dstAlpha = gxmnvg_convertBlendFuncFactor(op.dstAlpha);
    // act like invalid
    if (blend.srcRGB == SCE_GXM_BLEND_FACTOR_DST_ALPHA_SATURATE || blend.dstRGB == SCE_GXM_BLEND_FACTOR_DST_ALPHA_SATURATE || blend.srcAlpha == SCE_GXM_BLEND_FACTOR_DST_ALPHA_SATURATE || blend.dstAlpha == SCE_GXM_BLEND_FACTOR_DST_ALPHA_SATURATE)
    {
        blend.srcRGB   = SCE_GXM_BLEND_FACTOR_ONE;
        blend.dstRGB   = SCE_GXM_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        blend.srcAlpha = SCE_GXM_BLEND_FACTOR_ONE;
        blend.dstAlpha = SCE_GXM_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
    }
    return blend;
}

static void gxmnvg__renderFlush(void* uptr)
{
    GXMNVGcontext* gxm = (GXMNVGcontext*)uptr;

    int i;
    if (gxm->ncalls > 0)
    {
        // Setup require GXM state.
        sceGxmSetVertexProgram(gxm->context, gxm->shader.prog.vert);
        sceGxmSetFragmentProgram(gxm->context, gxm->shader.prog.frag);

        sceGxmSetCullMode(gxm->context, SCE_GXM_CULL_CW);
        sceGxmSetTwoSidedEnable(gxm->context, SCE_GXM_TWO_SIDED_DISABLED);
        sceGxmSetFrontStencilRef(gxm->context, 0);
        sceGxmSetBackStencilRef(gxm->context, 0);

        gxmnvg__disableStencilTest(gxm);

        // Set view just once per frame.
        {
            void* uniform_buffer;
            sceGxmReserveVertexDefaultUniformBuffer(gxm->context, &uniform_buffer);
            sceGxmSetUniformDataF(uniform_buffer, gxm->shader.loc[GXMNVG_LOC_VIEWSIZE], 0, 2, gxm->view);
        }

        for (i = 0; i < gxm->ncalls; i++)
        {
            GXMNVGcall* call = &gxm->calls[i];
            //            gxmnvg__blendFuncSeparate(gxm, &call->blendFunc);
            if (call->type == GXMNVG_FILL)
                gxmnvg__fill(gxm, call);
            else if (call->type == GXMNVG_CONVEXFILL)
                gxmnvg__convexFill(gxm, call);
            else if (call->type == GXMNVG_STROKE)
                gxmnvg__stroke(gxm, call);
            else if (call->type == GXMNVG_TRIANGLES)
                gxmnvg__triangles(gxm, call);
        }
    }

    // Reset calls
    gxm->nverts    = 0;
    gxm->npaths    = 0;
    gxm->ncalls    = 0;
    gxm->nuniforms = 0;

    // texture gc
    gxmnvg__garbageCollector(gxm);
}

static int gxmnvg__maxVertCount(const NVGpath* paths, int npaths)
{
    int i, count = 0;
    for (i = 0; i < npaths; i++)
    {
        count += paths[i].nfill;
        count += paths[i].nstroke;
    }
    return count;
}

static GXMNVGcall* gxmnvg__allocCall(GXMNVGcontext* gxm)
{
    GXMNVGcall* ret = NULL;
    if (gxm->ncalls + 1 > gxm->ccalls)
    {
        GXMNVGcall* calls;
        int ccalls = gxmnvg__maxi(gxm->ncalls + 1, 128) + gxm->ccalls / 2; // 1.5x Overallocate
        calls      = (GXMNVGcall*)realloc(gxm->calls, sizeof(GXMNVGcall) * ccalls);
        if (calls == NULL)
            return NULL;
        gxm->calls  = calls;
        gxm->ccalls = ccalls;
    }
    ret = &gxm->calls[gxm->ncalls++];
    memset(ret, 0, sizeof(GXMNVGcall));
    return ret;
}

static int gxmnvg__allocPaths(GXMNVGcontext* gxm, int n)
{
    int ret = 0;
    if (gxm->npaths + n > gxm->cpaths)
    {
        GXMNVGpath* paths;
        int cpaths = gxmnvg__maxi(gxm->npaths + n, 128) + gxm->cpaths / 2; // 1.5x Overallocate
        paths      = (GXMNVGpath*)realloc(gxm->paths, sizeof(GXMNVGpath) * cpaths);
        if (paths == NULL)
            return -1;
        gxm->paths  = paths;
        gxm->cpaths = cpaths;
    }
    ret = gxm->npaths;
    gxm->npaths += n;
    return ret;
}

static int gxmnvg__allocVerts(GXMNVGcontext* gxm, int n)
{
    int ret = 0;
    if (gxm->nverts + n > gxm->cverts)
    {
        NVGvertex* verts;
        int cverts = gxmnvg__maxi(gxm->nverts + n, 4096) + gxm->cverts / 2; // 1.5x Overallocate
        verts      = (NVGvertex*)realloc(gxm->verts, sizeof(NVGvertex) * cverts);
        if (verts == NULL)
            return -1;
        gxm->verts  = verts;
        gxm->cverts = cverts;
    }
    ret = gxm->nverts;
    gxm->nverts += n;
    return ret;
}

static int gxmnvg__allocFragUniforms(GXMNVGcontext* gxm, int n)
{
    int ret = 0, structSize = gxm->fragSize;
    if (gxm->nuniforms + n > gxm->cuniforms)
    {
        unsigned char* uniforms;
        int cuniforms = gxmnvg__maxi(gxm->nuniforms + n, 128) + gxm->cuniforms / 2; // 1.5x Overallocate
        uniforms      = (unsigned char*)realloc(gxm->uniforms, structSize * cuniforms);
        if (uniforms == NULL)
            return -1;
        gxm->uniforms  = uniforms;
        gxm->cuniforms = cuniforms;
    }
    ret = gxm->nuniforms * structSize;
    gxm->nuniforms += n;
    return ret;
}

static GXMNVGfragUniforms* nvg__fragUniformPtr(GXMNVGcontext* gxm, int i)
{
    return (GXMNVGfragUniforms*)&gxm->uniforms[i];
}

static void gxmnvg__vset(NVGvertex* vtx, float x, float y, float u, float v)
{
    vtx->x = x;
    vtx->y = y;
    vtx->u = u;
    vtx->v = v;
}

static void gxmnvg__renderFill(void* uptr, NVGpaint* paint,
    NVGcompositeOperationState compositeOperation, NVGscissor* scissor,
    float fringe, const float* bounds, const NVGpath* paths, int npaths)
{
    GXMNVGcontext* gxm = (GXMNVGcontext*)uptr;
    GXMNVGcall* call   = gxmnvg__allocCall(gxm);
    NVGvertex* quad;
    GXMNVGfragUniforms* frag;
    int i, maxverts, offset;

    if (call == NULL)
        return;

    call->type          = GXMNVG_FILL;
    call->triangleCount = 4;
    call->pathOffset    = gxmnvg__allocPaths(gxm, npaths);
    if (call->pathOffset == -1)
        goto error;
    call->pathCount = npaths;
    call->image     = paint->image;
    call->blendFunc = gxmnvg__blendCompositeOperation(compositeOperation);

    if (npaths == 1 && paths[0].convex)
    {
        call->type          = GXMNVG_CONVEXFILL;
        call->triangleCount = 0; // Bounding box fill quad not needed for convex fill
    }

    // Allocate vertices for all the paths.
    maxverts = gxmnvg__maxVertCount(paths, npaths) + call->triangleCount;
    offset   = gxmnvg__allocVerts(gxm, maxverts);
    if (offset == -1)
        goto error;

    for (i = 0; i < npaths; i++)
    {
        GXMNVGpath* copy    = &gxm->paths[call->pathOffset + i];
        const NVGpath* path = &paths[i];
        memset(copy, 0, sizeof(GXMNVGpath));
        if (path->nfill > 0)
        {
            copy->fillOffset = offset;
            copy->fillCount  = path->nfill;
            memcpy(&gxm->verts[offset], path->fill, sizeof(NVGvertex) * path->nfill);
            offset += path->nfill;
        }
        if (path->nstroke > 0)
        {
            copy->strokeOffset = offset;
            copy->strokeCount  = path->nstroke;
            memcpy(&gxm->verts[offset], path->stroke, sizeof(NVGvertex) * path->nstroke);
            offset += path->nstroke;
        }
    }

    // Setup uniforms for draw calls
    if (call->type == GXMNVG_FILL)
    {
        // Quad
        call->triangleOffset = offset;
        quad                 = &gxm->verts[call->triangleOffset];
        gxmnvg__vset(&quad[0], bounds[2], bounds[3], 0.5f, 1.0f);
        gxmnvg__vset(&quad[1], bounds[2], bounds[1], 0.5f, 1.0f);
        gxmnvg__vset(&quad[2], bounds[0], bounds[3], 0.5f, 1.0f);
        gxmnvg__vset(&quad[3], bounds[0], bounds[1], 0.5f, 1.0f);

        call->uniformOffset = gxmnvg__allocFragUniforms(gxm, 2);
        if (call->uniformOffset == -1)
            goto error;
        // Simple shader for stencil
        frag = nvg__fragUniformPtr(gxm, call->uniformOffset);
        memset(frag, 0, sizeof(*frag));
        frag->strokeThr = -1.0f;
        frag->type      = NSVG_SHADER_SIMPLE;
        // Fill shader
        gxmnvg__convertPaint(gxm, nvg__fragUniformPtr(gxm, call->uniformOffset + gxm->fragSize), paint, scissor, fringe,
            fringe, -1.0f);
    }
    else
    {
        call->uniformOffset = gxmnvg__allocFragUniforms(gxm, 1);
        if (call->uniformOffset == -1)
            goto error;
        // Fill shader
        gxmnvg__convertPaint(gxm, nvg__fragUniformPtr(gxm, call->uniformOffset), paint, scissor, fringe, fringe, -1.0f);
    }

    return;

error:
    // We get here if call alloc was ok, but something else is not.
    // Roll back the last call to prevent drawing it.
    if (gxm->ncalls > 0)
        gxm->ncalls--;
}

static void gxmnvg__renderStroke(void* uptr, NVGpaint* paint,
    NVGcompositeOperationState compositeOperation, NVGscissor* scissor,
    float fringe, float strokeWidth, const NVGpath* paths, int npaths)
{
    GXMNVGcontext* gxm = (GXMNVGcontext*)uptr;
    GXMNVGcall* call   = gxmnvg__allocCall(gxm);
    int i, maxverts, offset;

    if (call == NULL)
        return;

    call->type       = GXMNVG_STROKE;
    call->pathOffset = gxmnvg__allocPaths(gxm, npaths);
    if (call->pathOffset == -1)
        goto error;
    call->pathCount = npaths;
    call->image     = paint->image;
    call->blendFunc = gxmnvg__blendCompositeOperation(compositeOperation);

    // Allocate vertices for all the paths.
    maxverts = gxmnvg__maxVertCount(paths, npaths);
    offset   = gxmnvg__allocVerts(gxm, maxverts);
    if (offset == -1)
        goto error;

    for (i = 0; i < npaths; i++)
    {
        GXMNVGpath* copy    = &gxm->paths[call->pathOffset + i];
        const NVGpath* path = &paths[i];
        memset(copy, 0, sizeof(GXMNVGpath));
        if (path->nstroke)
        {
            copy->strokeOffset = offset;
            copy->strokeCount  = path->nstroke;
            memcpy(&gxm->verts[offset], path->stroke, sizeof(NVGvertex) * path->nstroke);
            offset += path->nstroke;
        }
    }

    if (gxm->flags & NVG_STENCIL_STROKES)
    {
        // Fill shader
        call->uniformOffset = gxmnvg__allocFragUniforms(gxm, 2);
        if (call->uniformOffset == -1)
            goto error;

        gxmnvg__convertPaint(gxm, nvg__fragUniformPtr(gxm, call->uniformOffset), paint, scissor, strokeWidth, fringe,
            -1.0f);
        gxmnvg__convertPaint(gxm, nvg__fragUniformPtr(gxm, call->uniformOffset + gxm->fragSize), paint, scissor,
            strokeWidth, fringe, 1.0f - 0.5f / 255.0f);
    }
    else
    {
        // Fill shader
        call->uniformOffset = gxmnvg__allocFragUniforms(gxm, 1);
        if (call->uniformOffset == -1)
            goto error;
        gxmnvg__convertPaint(gxm, nvg__fragUniformPtr(gxm, call->uniformOffset), paint, scissor, strokeWidth, fringe,
            -1.0f);
    }

    return;

error:
    // We get here if call alloc was ok, but something else is not.
    // Roll back the last call to prevent drawing it.
    if (gxm->ncalls > 0)
        gxm->ncalls--;
}

static void gxmnvg__renderTriangles(void* uptr, NVGpaint* paint,
    NVGcompositeOperationState compositeOperation, NVGscissor* scissor,
    const NVGvertex* verts, int nverts, float fringe)
{
    GXMNVGcontext* gxm = (GXMNVGcontext*)uptr;
    GXMNVGcall* call   = gxmnvg__allocCall(gxm);
    GXMNVGfragUniforms* frag;

    if (call == NULL)
        return;

    call->type      = GXMNVG_TRIANGLES;
    call->image     = paint->image;
    call->blendFunc = gxmnvg__blendCompositeOperation(compositeOperation);

    // Allocate vertices for all the paths.
    call->triangleOffset = gxmnvg__allocVerts(gxm, nverts);
    if (call->triangleOffset == -1)
        goto error;
    call->triangleCount = nverts;

    memcpy(&gxm->verts[call->triangleOffset], verts, sizeof(NVGvertex) * nverts);

    // Fill shader
    call->uniformOffset = gxmnvg__allocFragUniforms(gxm, 1);
    if (call->uniformOffset == -1)
        goto error;
    frag = nvg__fragUniformPtr(gxm, call->uniformOffset);
    gxmnvg__convertPaint(gxm, frag, paint, scissor, 1.0f, fringe, -1.0f);
    frag->type = NSVG_SHADER_IMG;

    return;

error:
    // We get here if call alloc was ok, but something else is not.
    // Roll back the last call to prevent drawing it.
    if (gxm->ncalls > 0)
        gxm->ncalls--;
}

static void gxmnvg__renderDelete(void* uptr)
{
    GXMNVGcontext* gxm = (GXMNVGcontext*)uptr;
    int i;
    if (gxm == NULL)
        return;

    // Ensure the GPU is not using memory
    sceGxmFinish(gxm->context);
    sceGxmDisplayQueueFinish();

    gpu_unmap_free(gxm->verticesUid); // vertex stream

    for (i = 0; i < gxm->ntextures; i++)
    {
        if (gxm->textures[i].data_UID != 0 && (gxm->textures[i].flags & NVG_IMAGE_NODELETE) == 0)
            gpu_unmap_free(gxm->textures[i].data_UID);
    }

    gxmnvg__deleteShader(&gxm->shader);
    gxmnvg__deleteShader(&gxm->depth_shader);

    free(gxm->textures);
    free(gxm->paths);
    free(gxm->verts);
    free(gxm->uniforms);
    free(gxm->calls);

    free(gxm);
}

NVGcontext* nvgCreateGXM(NVGXMframebuffer* fb, int flags)
{
    NVGparams params;
    NVGcontext* ctx    = NULL;
    GXMNVGcontext* gxm = (GXMNVGcontext*)malloc(sizeof(GXMNVGcontext));
    if (gxm == NULL)
        goto error;
    memset(gxm, 0, sizeof(GXMNVGcontext));
    gxm->context        = fb->context;
    gxm->shader_patcher = fb->shader_patcher;
    gxm->msaa           = fb->msaa;

    memset(&params, 0, sizeof(params));
    params.renderCreate         = gxmnvg__renderCreate;
    params.renderCreateTexture  = gxmnvg__renderCreateTexture;
    params.renderDeleteTexture  = gxmnvg__renderDeleteTexture;
    params.renderUpdateTexture  = gxmnvg__renderUpdateTexture;
    params.renderGetTextureSize = gxmnvg__renderGetTextureSize;
    params.renderViewport       = gxmnvg__renderViewport;
    params.renderCancel         = gxmnvg__renderCancel;
    params.renderFlush          = gxmnvg__renderFlush;
    params.renderFill           = gxmnvg__renderFill;
    params.renderStroke         = gxmnvg__renderStroke;
    params.renderTriangles      = gxmnvg__renderTriangles;
    params.renderDelete         = gxmnvg__renderDelete;
    params.userPtr              = gxm;
    params.edgeAntiAlias        = flags & NVG_ANTIALIAS ? 1 : 0;

    gxm->flags = flags;

#ifdef USE_VITA_SHARK
    if (flags & NVG_DEBUG)
    {
        shark_set_warnings_level(SHARK_WARN_MAX);
        shark_install_log_cb(shark_log_cb);
    }
#endif

    ctx = nvgCreateInternal(&params);
    if (ctx == NULL)
        goto error;

    return ctx;

error:
    // 'gxm' is freed by nvgDeleteInternal.
    if (ctx != NULL)
        nvgDeleteInternal(ctx);
    return NULL;
}

void nvgDeleteGXM(NVGcontext* ctx)
{
    nvgDeleteInternal(ctx);
}

#endif /* NANOVG_GL_IMPLEMENTATION */
