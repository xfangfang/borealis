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
    int _pad0;
    int _pad1;
    int _pad2;

    // Dolby Vision Profile 5 Metadata (std140)
    vec4 dovi_ycc[3];
    vec4 dovi_off;
    vec4 dovi_lms[3];
    vec4 dovi_lohi[3];
    vec4 dovi_piv[6];
    vec4 dovi_coef[24];
    vec4 dovi_mmr[144];
    vec4 dovi_meta; // meta.x: 1 when frame carries an RPU
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
    // ITU-R BT.2100 Table 5: HLG Reference EOTF with OOTF (system gamma = 1.2 for 1000 nits)
    float Y_S = dot(lin, vec3(0.2627, 0.6780, 0.0593));
    float ootf = (Y_S > 1e-6) ? pow(Y_S, 0.2) : 0.0;
    return lin * (1000.0 * ootf);
}

vec3 hlg_to_pq(vec3 hlg) {
    return nits_to_pq_vec3(hlg_to_nits(hlg));
}

vec3 hlg_to_sdr(vec3 hlg) {
    vec3 lin2020 = hlg_to_nits(hlg);
    return apply_libplacebo_bt2390_tone_mapping(lin2020);
}

// ===========================================================================
// Dolby Vision Profile 5 Reshaping & Decoding (libplacebo / RPU std140)
// ===========================================================================
vec3 sample_p010_ipt(vec2 coord) {
    ivec2 tex_sz = textureSize(tex, 0);
    int w = tex_sz.x;
    int h = (tex_sz.y * 2) / 3;

    // Luma (Y = I): sample with linear filtering
    vec2 uv_y = vec2(coord.x, coord.y * ((float(h) - 0.5) / float(tex_sz.y)));
    float Y_raw = texture(tex, uv_y).r;

    // Chroma (U = Ct, V = Cp): texelFetch from interleaved UV rows
    int chroma_row = h + int(clamp(coord.y * float(h / 2), 0.0, float(h / 2 - 1)));
    int chroma_k = int(clamp(coord.x * float(w / 2), 0.0, float(w / 2 - 1)));
    float U_raw = texelFetch(tex, ivec2(chroma_k * 2, chroma_row), 0).r;
    float V_raw = texelFetch(tex, ivec2(chroma_k * 2 + 1, chroma_row), 0).r;

    // P010 MSB normalization
    return clamp(vec3(Y_raw, U_raw, V_raw) * (65535.0 / 65472.0), 0.0, 1.0);
}

float dv_reshape_component(int c, vec3 sig, float s) {
    if (dovi_lohi[c].z == 0.0) {
        return s;
    }

    vec4 piv0 = dovi_piv[c * 2 + 0];
    vec4 piv1 = dovi_piv[c * 2 + 1];

    int coef_base = c * 8;
    vec4 c0 = dovi_coef[coef_base + 0];
    vec4 c1 = dovi_coef[coef_base + 1];
    vec4 c2 = dovi_coef[coef_base + 2];
    vec4 c3 = dovi_coef[coef_base + 3];
    vec4 c4 = dovi_coef[coef_base + 4];
    vec4 c5 = dovi_coef[coef_base + 5];
    vec4 c6 = dovi_coef[coef_base + 6];
    vec4 c7 = dovi_coef[coef_base + 7];

    float b0 = float(s >= piv0.x);
    float b1 = float(s >= piv0.y);
    float b2 = float(s >= piv0.z);
    float b3 = float(s >= piv0.w);
    float b4 = float(s >= piv1.x);
    float b5 = float(s >= piv1.y);
    float b6 = float(s >= piv1.z);

    vec4 coeffs = mix(
        mix(mix(c0, c1, b0),
            mix(c2, c3, b2),
            b1),
        mix(mix(c4, c5, b4),
            mix(c6, c7, b6),
            b5),
        b3
    );

    int order = int(coeffs.w);
    if (order == 0) {
        // Polynomial: s = c2 * s^2 + c1 * s + c0
        s = (coeffs.z * s + coeffs.y) * s + coeffs.x;
    } else {
        // MMR (Multivariate Multiple Regression)
        int mmr_idx = c * 48 + int(coeffs.y);
        vec4 sigX;
        s = coeffs.x;
        sigX.xyz = sig.xxy * sig.yzz;
        sigX.w = sigX.x * sig.z;
        s += dot(dovi_mmr[mmr_idx + 0].xyz, sig);
        s += dot(dovi_mmr[mmr_idx + 1], sigX);

        if (order >= 2) {
            vec3 sig2 = sig * sig;
            vec4 sigX2 = sigX * sigX;
            s += dot(dovi_mmr[mmr_idx + 2].xyz, sig2);
            s += dot(dovi_mmr[mmr_idx + 3], sigX2);

            if (order >= 3) {
                s += dot(dovi_mmr[mmr_idx + 4].xyz, sig2 * sig);
                s += dot(dovi_mmr[mmr_idx + 5], sigX2 * sigX);
            }
        }
    }

    return clamp(s, 0.0, 1.0);
}

vec3 dolby_vision_decode_rpu(vec3 raw) {
    vec3 sig = vec3(
        clamp(raw.x, dovi_lohi[0].x, dovi_lohi[0].y),
        clamp(raw.y, dovi_lohi[1].x, dovi_lohi[1].y),
        clamp(raw.z, dovi_lohi[2].x, dovi_lohi[2].y)
    );

    vec3 reshaped = vec3(
        dv_reshape_component(0, sig, sig.x),
        dv_reshape_component(1, sig, sig.y),
        dv_reshape_component(2, sig, sig.z)
    );

    vec3 t = reshaped - dovi_off.xyz;
    vec3 step1 = vec3(
        dot(dovi_ycc[0].xyz, t),
        dot(dovi_ycc[1].xyz, t),
        dot(dovi_ycc[2].xyz, t)
    );

    vec3 lin2020;
    if (dovi_meta.y == 1.0) {
        // HLG (Profile 8.4): step1 is BT.2020 RGB in HLG gamma
        lin2020 = hlg_to_nits(clamp(step1, 0.0, 1.0));
    } else {
        // PQ (Profile 5 and Profile 8.1): step1 is LMS' in PQ
        vec3 lms = pq_to_nits_vec3(clamp(step1, 0.0, 1.0));
        lin2020 = vec3(
            dot(dovi_lms[0].xyz, lms),
            dot(dovi_lms[1].xyz, lms),
            dot(dovi_lms[2].xyz, lms)
        );
    }

    return max(lin2020, vec3(0.0));
}

vec3 dolby_vision_profile5_fallback(vec3 raw_ipt) {
    float Y  = raw_ipt.x;
    float Ct = raw_ipt.y - 0.5;
    float Cp = raw_ipt.z - 0.5;

    float L_prime = clamp(Y + 0.09997559 * Ct + 0.21166992 * Cp, 0.0, 1.0);
    float M_prime = clamp(Y - 0.10827637 * Ct - 0.07995605 * Cp, 0.0, 1.0);
    float S_prime = clamp(Y + 0.55993652 * Ct - 0.32055664 * Cp, 0.0, 1.0);
    vec3 lin_lms = pq_to_nits_vec3(vec3(L_prime, M_prime, S_prime));

    const mat3 M_lms_to_2020 = mat3(
         3.06441879, -0.65612108,  0.01736321,
        -2.16597676,  1.78554118, -0.04725154,
         0.10155818, -0.12943749,  1.03004253
    );
    vec3 lin2020 = M_lms_to_2020 * lin_lms;
    return max(lin2020, vec3(0.0));
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
            vec3 raw_ipt = sample_p010_ipt(pt);
            vec3 lin2020 = (dovi_meta.x == 1.0) ? dolby_vision_decode_rpu(raw_ipt)
                                                 : dolby_vision_profile5_fallback(raw_ipt);
            if (isHdrDirect) {
                videoPq = nits_to_pq_vec3(lin2020);
            } else {
                color = vec4(apply_libplacebo_bt2390_tone_mapping(lin2020), 1.0);
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
            vec3 raw_ipt = sample_p010_ipt(ftcoord);
            vec3 lin2020 = (dovi_meta.x == 1.0) ? dolby_vision_decode_rpu(raw_ipt)
                                                 : dolby_vision_profile5_fallback(raw_ipt);
            if (isHdrDirect) {
                videoPq = nits_to_pq_vec3(lin2020);
            } else {
                color = vec4(apply_libplacebo_bt2390_tone_mapping(lin2020), 1.0);
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
        // The swapchain is a real 10bit PQ target (VK_FORMAT_A2B10G10R10 with
        // VideoOut scanning it out as BGR10A2 BT.2100 PQ), so the shader writes
        // PQ floats and lets the blending stay in the target. Packing the bits
        // into an 8bit buffer by hand cannot work: the painter blends with
        // premultiplied alpha, and blended packed bits are not a PQ value.
        vec3 finalPq;
        if (isVideo) {
            finalPq = videoPq * innerCol.rgb * strokeAlpha * scissor;
        } else {
            finalPq = srgb_to_pq_bt2408(clamp(result.rgb, 0.0, 1.0));
        }

        outColor = vec4(clamp(finalPq, 0.0, 1.0), clamp(result.a, 0.0, 1.0));
    } else {
        outColor = result;
    }
}
