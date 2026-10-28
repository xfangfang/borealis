#version 450 core

layout(std140, binding = 1) uniform frag {
    mat3 scissorMat;
    mat3 paintMat;
    vec4 innerCol;
    vec4 outerCol;
    vec2 scissorExt;
    vec2 scissorScale;
    vec2 extent;
    float radius;
    float feather;
    float strokeMult;
    float strokeThr;
    int texType;
    int type;
    int hdrOutput; // 0: SDR 模式; 1: 物理真 HDR 模式 (Bgr10A2Bt2100Pq)
};

layout(binding = 2) uniform sampler2D tex;

layout(location = 0) in vec2 ftcoord;
layout(location = 1) in vec2 fpos;

layout(location = 0) out vec4 outColor;

float sdroundrect(vec2 pt, vec2 ext, float rad) {
    vec2 ext2 = ext - vec2(rad, rad);
    vec2 d = abs(pt) - ext2;
    return min(max(d.x, d.y), 0.0) + length(max(d, 0.0)) - rad;
}

float scissorMask(vec2 p) {
    vec2 sc = (abs((scissorMat * vec3(p, 1.0)).xy) - scissorExt);
    sc = vec2(0.5, 0.5) - sc * scissorScale;
    return clamp(sc.x, 0.0, 1.0) * clamp(sc.y, 0.0, 1.0);
}

#ifdef EDGE_AA
float strokeMask() {
    return min(1.0, (1.0 - abs(ftcoord.x * 2.0 - 1.0)) * strokeMult) * min(1.0, ftcoord.y);
}
#endif

// ===========================================================================
// SMPTE ST 2084 PQ Color Transfer (Forward OETF & Inverse EOTF)
// ===========================================================================
const float pq_m1 = 0.1593017578125;
const float pq_m2 = 78.84375;
const float pq_c1 = 0.8359375;
const float pq_c2 = 18.8515625;
const float pq_c3 = 18.6875;

float pq_to_nits(float N) {
    if (N <= 0.0) return 0.0;
    float N_p = pow(N, 1.0 / pq_m2);
    float num = max(N_p - pq_c1, 0.0);
    float den = pq_c2 - pq_c3 * N_p;
    return 10000.0 * pow(num / den, 1.0 / pq_m1);
}

vec3 pq_to_nits_vec3(vec3 V) {
    return vec3(pq_to_nits(V.r), pq_to_nits(V.g), pq_to_nits(V.b));
}

float nits_to_pq(float L) {
    if (L <= 0.0) return 0.0;
    float Y = clamp(L / 10000.0, 0.0, 1.0);
    float Y_m1 = pow(Y, pq_m1);
    float num = pq_c1 + pq_c2 * Y_m1;
    float den = 1.0 + pq_c3 * Y_m1;
    return pow(num / den, pq_m2);
}

vec3 nits_to_pq_vec3(vec3 L) {
    return vec3(nits_to_pq(L.r), nits_to_pq(L.g), nits_to_pq(L.b));
}

// ===========================================================================
// ITU-R BT.2390 EETF Tone Mapping (libplacebo implementation)
// ===========================================================================
float bt2390_spline(float E, float KS, float maxLum) {
    float tb = (E - KS) / (1.0 - KS);
    float tb2 = tb * tb;
    float tb3 = tb2 * tb;
    return (2.0 * tb3 - 3.0 * tb2 + 1.0) * KS +
           (tb3 - 2.0 * tb2 + tb) * (1.0 - KS) +
           (-2.0 * tb3 + 3.0 * tb2) * maxLum;
}

vec3 apply_libplacebo_bt2390_tone_mapping(vec3 lin2020) {
    float Y = dot(lin2020, vec3(0.2627, 0.6780, 0.0593));
    if (Y <= 1e-4) return vec3(0.0);

    // 0 ~ 100 nits (SDR standard range) 保持 1:1 线性光照，确保人脸与环境通透不暗淡
    // 100 ~ 1000+ nits (HDR highlights) 采用高动态平滑滚降曲线，保留层次
    float Y1 = Y <= 100.0 ? Y : (100.0 + (Y - 100.0) / (1.0 + (Y - 100.0) / 120.0));

    vec3 mapped2020 = lin2020 * (Y1 / (Y * 100.0));

    const mat3 M_2020_to_709 = mat3(
         1.6604910, -0.1245505, -0.0181508,
        -0.5876411,  1.1328999, -0.1005789,
        -0.0728499, -0.0083494,  1.1187297
    );
    vec3 lin709 = max(M_2020_to_709 * mapped2020, vec3(0.0));

    // BT.709 OETF (Linear -> Gamma 2.4 / sRGB)
    vec3 srgb = mix(
        lin709 * 12.92,
        1.055 * pow(lin709, vec3(1.0 / 2.4)) - 0.055,
        greaterThan(lin709, vec3(0.0031308))
    );
    return clamp(srgb, 0.0, 1.0);
}

vec3 hdr10_to_sdr(vec3 pq) {
    vec3 lin2020 = pq_to_nits_vec3(pq);
    return apply_libplacebo_bt2390_tone_mapping(lin2020);
}

// ===========================================================================
// ITU-R BT.2100 HLG (Hybrid Log-Gamma, ARIB STD-B67) EOTF
// ===========================================================================
vec3 hlg_to_nits(vec3 hlg) {
    const float a = 0.17883277;
    const float b = 0.28466889;
    const float c = 0.55991073;
    vec3 h = clamp(hlg, vec3(0.0), vec3(1.0));
    vec3 lin = mix(
        (h * h) / 3.0,
        (exp((h - vec3(c)) / a) + vec3(b)) / 12.0,
        greaterThan(h, vec3(0.5))
    );
    // HLG 标称峰值白点为 1000 Nits
    return lin * 1000.0;
}

vec3 hlg_to_pq(vec3 hlg) {
    return nits_to_pq_vec3(hlg_to_nits(hlg));
}

vec3 hlg_to_sdr(vec3 hlg) {
    vec3 lin2020 = hlg_to_nits(hlg);
    return apply_libplacebo_bt2390_tone_mapping(lin2020);
}

// ===========================================================================
// Dolby Vision Profile 5 Reshaping & Decoding
// ===========================================================================
vec3 dolby_vision_profile5_to_nits(vec3 c) {
    vec3 pseudo_rgb = c.rgb;

    // 1. 从伪 RGB 提取 swscale 的 YUV 信号:
    // Y (明度 I), Cb (色度差 Ct), Cr (色度差 Cp)
    const mat3 M_rgb_to_yuv_709 = mat3(
         0.21260000, -0.11457211,  0.50000000,
         0.71520000, -0.38542789, -0.45415291,
         0.07220000,  0.50000000, -0.04584709
    );
    vec3 yuv = M_rgb_to_yuv_709 * pseudo_rgb;
    
    // 2. 补偿 mpv swscale 默认按 Limited Range (16-235) 造成的色度过载:
    // Profile 5 属于 Full Range，swscale 将其当作 Limited Range 处理时色度被拉伸了约 14%
    float Y = clamp(yuv.x, 0.0, 1.0);
    float Ct = yuv.y * (224.0 / 255.0);
    float Cp = yuv.z * (224.0 / 255.0);

    // 3. 抑制暗部由于非线性色度差导致的绿色过敏泄漏:
    // 在低明度暗调区域 (Y < 0.25)，色度噪声在 PQ 极陡曲线下会被放大数倍，进行软平滑衰减
    float chroma_damp = smoothstep(0.02, 0.25, Y);
    Ct *= mix(0.7, 1.0, chroma_damp);
    Cp *= mix(0.7, 1.0, chroma_damp);

    // 4. 重建标准 ITU-R BT.2100 非线性 LMS' (L', M', S')
    float L_prime = clamp(Y + 0.00860904 * Ct + 0.11102963 * Cp, 0.0, 1.0);
    float M_prime = clamp(Y - 0.00860904 * Ct - 0.11102963 * Cp, 0.0, 1.0);
    float S_prime = clamp(Y + 0.56003134 * Ct - 0.32062717 * Cp, 0.0, 1.0);
    vec3 lms_prime = vec3(L_prime, M_prime, S_prime);

    // 5. PQ EOTF -> 线性 LMS 光照
    vec3 lin_lms = pq_to_nits_vec3(lms_prime);

    // 6. 线性 LMS -> BT.2020 矩阵 (libplacebo dovi_lms2rgb)
    const mat3 M_lms_to_2020 = mat3(
         3.06441879, -0.65612108,  0.01736321,
        -2.16597676,  1.78554118, -0.04725154,
         0.10155818, -0.12943749,  1.03004253
    );
    vec3 lin2020 = M_lms_to_2020 * lin_lms;

    // 7. 色域边界保护: 防止极端暗部产生非物理负值导致的荧光绿偏色
    float min_c = min(lin2020.r, min(lin2020.g, lin2020.b));
    if (min_c < 0.0) {
        lin2020 -= vec3(min_c);
    }
    return max(lin2020, vec3(0.0));
}

vec3 dolby_vision_profile5_to_sdr(vec3 c) {
    vec3 lin2020 = dolby_vision_profile5_to_nits(c);
    return apply_libplacebo_bt2390_tone_mapping(lin2020);
}

vec3 dolby_vision_profile5_to_pq(vec3 c) {
    vec3 lin2020 = dolby_vision_profile5_to_nits(c);
    return nits_to_pq_vec3(lin2020);
}

// ===========================================================================
// ITU-R BT.2408: UI sRGB -> 203 Nits (Graphics White) BT.2020 PQ
// ===========================================================================
vec3 srgb_to_pq_bt2408(vec3 srgb) {
    vec3 lin_srgb = mix(
        srgb / 12.92,
        pow((srgb + vec3(0.055)) / 1.055, vec3(2.4)),
        greaterThan(srgb, vec3(0.04045))
    );
    const mat3 M_709_to_2020 = mat3(
        0.6274040, 0.0690970, 0.0163916,
        0.3292820, 0.9195400, 0.0880132,
        0.0433136, 0.0113612, 0.8955950
    );
    vec3 lin_2020 = max(M_709_to_2020 * lin_srgb, vec3(0.0));
    return nits_to_pq_vec3(lin_2020 * 203.0);
}

void main(void) {
    vec4 result;
    float scissor = scissorMask(fpos);
#ifdef EDGE_AA
    float strokeAlpha = strokeMask();
    if (strokeAlpha < strokeThr) discard;
#else
    float strokeAlpha = 1.0;
#endif

    bool isHdrDirect = (hdrOutput == 1);
    vec3 videoPq = vec3(0.0);
    bool isVideo = false;

    if (type == 0) { // Gradient / Solid
        vec2 pt = (paintMat * vec3(fpos, 1.0)).xy;
        float d = clamp((sdroundrect(pt, extent, radius) + feather * 0.5) / feather, 0.0, 1.0);
        vec4 color = mix(innerCol, outerCol, d);
        color *= strokeAlpha * scissor;
        result = color;
    } else if (type == 1) { // Image
        vec2 pt = (paintMat * vec3(fpos, 1.0)).xy / extent;
        vec4 color = texture(tex, pt);
        if (texType == 1) color = vec4(color.xyz * color.w, color.w);
        else if (texType == 2) color = vec4(color.x);
        else if (texType == 4) {
            isVideo = true;
            if (isHdrDirect) {
                videoPq = clamp(color.rgb, 0.0, 1.0);
            } else {
                color = vec4(hdr10_to_sdr(color.rgb), color.a);
            }
        } else if (texType == 5) {
            isVideo = true;
            if (isHdrDirect) {
                videoPq = dolby_vision_profile5_to_pq(color.rgb);
            } else {
                color = vec4(dolby_vision_profile5_to_sdr(color.rgb), color.a);
            }
        } else if (texType == 6) {
            isVideo = true;
            if (isHdrDirect) {
                videoPq = hlg_to_pq(color.rgb);
            } else {
                color = vec4(hlg_to_sdr(color.rgb), color.a);
            }
        }

        color *= innerCol;
        color *= strokeAlpha * scissor;
        result = color;
    } else if (type == 2) { // Stencil fill
        result = vec4(1.0);
    } else if (type == 3) { // Textured tris
        vec4 color = texture(tex, ftcoord);
        if (texType == 1) color = vec4(color.xyz * color.w, color.w);
        else if (texType == 2) color = vec4(color.x);
        else if (texType == 4) {
            isVideo = true;
            if (isHdrDirect) {
                videoPq = clamp(color.rgb, 0.0, 1.0);
            } else {
                color = vec4(hdr10_to_sdr(color.rgb), color.a);
            }
        } else if (texType == 5) {
            isVideo = true;
            if (isHdrDirect) {
                videoPq = dolby_vision_profile5_to_pq(color.rgb);
            } else {
                color = vec4(dolby_vision_profile5_to_sdr(color.rgb), color.a);
            }
        } else if (texType == 6) {
            isVideo = true;
            if (isHdrDirect) {
                videoPq = hlg_to_pq(color.rgb);
            } else {
                color = vec4(hlg_to_sdr(color.rgb), color.a);
            }
        }

        color *= scissor;
        result = color * innerCol;
    }

    if (isHdrDirect) {
        vec3 finalPq;
        if (isVideo) {
            finalPq = videoPq * innerCol.rgb * strokeAlpha * scissor;
        } else {
            finalPq = srgb_to_pq_bt2408(clamp(result.rgb, 0.0, 1.0));
        }

        uint b10 = uint(clamp(finalPq.b, 0.0, 1.0) * 1023.0 + 0.5);
        uint g10 = uint(clamp(finalPq.g, 0.0, 1.0) * 1023.0 + 0.5);
        uint r10 = uint(clamp(finalPq.r, 0.0, 1.0) * 1023.0 + 0.5);
        uint a2  = uint(clamp(result.a, 0.0, 1.0) * 3.0 + 0.5);
        uint u32 = b10 | (g10 << 10) | (r10 << 20) | (a2 << 30);

        outColor = vec4(
            float(u32 & 0xffu) / 255.0,
            float((u32 >> 8) & 0xffu) / 255.0,
            float((u32 >> 16) & 0xffu) / 255.0,
            float((u32 >> 24) & 0xffu) / 255.0
        );
    } else {
        outColor = result;
    }
}
