//
// Generated by RAGE Shader Editor 1.4.4
    ps_3_0
    def c219, 1.83951739e+025, 3.99382587e+024, 4.54357875e+030, 1.16447902e-042
    def c127, 0.999999881, 1, 0, 0
    def c0, 58.1640015, 47.1300011, 3.20000005, 1.79999995
    def c1, 0, 0.212500006, 0.715399981, 0.0720999986
    def c2, 0.25, 1, 0.5, 0
    def c3, 0, 2, 4, 8
    def c4, -0.5, -1.5, 1.5, 0.5
    def c5, 2, -1, 0.125, 0
    def c6, 1.10000002, 0, 0, 0
    def c8, 0.0121568628, 0.0160784312, 0.0435294099, 0.047450982
    def c9, 0.514117658, 0.518039227, 0.545490205, 0.549411774
    def c10, 0.0278431363, 0.000784313714, 0, 0
    def c11, 1.20000005, 0.0078125, 0.00390625, 0
    defi i0, 7, 0, 0, 0
    dcl_texcoord v0.xy
    dcl_2d s1
    dcl_2d s2
    dcl_2d s3
    dcl_2d s4
    dcl_2d s5
    dcl_2d s6
    dcl_2d s7
    dcl_2d s10
    mov r31, c2.w
    mov r3, c4
    mad r4, c76.xyxy, r3.xyzx, v0.xyxy
    mad r6, c76.xyxy, r3.wzyw, v0.xyxy
    texld r7.xyz, v0, s2
    texld r3.xyz, r4.xyyy, s2
    texld r4.xyz, r4.zwww, s2
    texld r5.xyz, r6.xyyy, s2
    texld r6.xyz, r6.zwww, s2
    dp3 r0.x, r7, c1.yzww
    dp3 r8.x, r3, c1.yzww
    dp3 r8.y, r4, c1.yzww
    dp3 r8.z, r5, c1.yzww
    dp3 r8.w, r6, c1.yzww
    dp4 r0.z, r8, c2.x
    add r8, r8, -r0.z
    dp4 r2.w, r8, r8
    add r0.x, r0.x, -r0.z
    mad r0.x, r0.x, r0.x, -r2.w
    if_eq r31.x, c223.z
      mov r21, c76
      texld r22, v0, s7
      add r23, r22.x, -c8
      add_sat r20, -r23_abs, c10.y
      add r23, r22.x, -c9
      add_sat r23, -r23_abs, c10.y
      add r20, r20, r23
      add r23.x, r22.x, -c10.x
      add_sat r23.x, -r23_abs, c10.y
      add r20.x, r20, r23
      mad r21, r21.xyxy, c2.yyww, v0.xyxy
      texldl r22, r21.xwww, s7
      add r23, r22.x, -c8
      add_sat r23, -r23_abs, c10.y
      add r20, r20, r23
      add r23, r22.x, -c9
      add_sat r23, -r23_abs, c10.y
      add r20, r20, r23
      add r23.x, r22.x, -c10.x
      add_sat r23.x, -r23_abs, c10.y
      add r20.x, r20, r23
      texldl r22, r21.zyyy, s7
      add r23, r22.x, -c8
      add_sat r23, -r23_abs, c10.y
      add r20, r20, r23
      add r23, r22.x, -c9
      add_sat r23, -r23_abs, c10.y
      add r20, r20, r23
      add r23.x, r22.x, -c10.x
      add_sat r23.x, -r23_abs, c10.y
      add r20.x, r20, r23
      texldl r22, r21.xyyy, s7
      add r23, r22.x, -c8
      add_sat r23, -r23_abs, c10.y
      add r20, r20, r23
      add r23, r22.x, -c9
      add_sat r23, -r23_abs, c10.y
      add r20, r20, r23
      add r23.x, r22.x, -c10.x
      add_sat r23.x, -r23_abs, c10.y
      add r20.x, r20, r23
      dp4 r20.x, r20, c2.y
      cmp r0.x, -r20_abs.x, -c2.y, r0.x
    endif
    cmp r0.xw, r0.x, c2.yzzz, c2.wyyy
    add r3.xyz, r3, r4
    add r3.xyz, r3, r5
    add r3.xyz, r3, r6
    mul r3.xyz, r3, c2.x
    lrp r3.xyz, r0.x, r3, r7
    if_ne r31.x, c222.w
      texld r0.x, v0, s1
      if_ne r0.x, c127.y
        rcp r20.x, c128.x
        mul r20.x, r20.x, c128.y
        pow r20.x, r20.x, r0.x
        mul r20.x, r20.x, c128.x
        add r20.y, r20.x, -c128.x
        add r20.z, c128.y, -c128.x
        mul r20.y, r20.y, c128.y
        mul r20.z, r20.z, r20.x
        rcp r20.z, r20.z
        mul r20.w, r20.y, r20.z
        min r0.x, r20.w, c127.x
      endif
      add r0.y, -c77.x, c77.y
      rcp r0.y, r0.y
      mul r0.z, r0.y, c77.y
      mul r0.z, r0.z, -c77.x
      mad r0.x, c77.y, -r0.y, r0.x
      rcp r0.x, r0.x
      mul r0.y, r0.z, r0.x
      mad r4.xyz, v0.yxyw, c5.x, c5.y
      mul r0.z, r4.y, c77.z
      mul r0.z, r0.y, r0.z
      mul r1.w, -r4.x, c77.w
      mul r1.w, r0.y, r1.w
      mul r5.xyz, r1.w, c73
      mad r5.xyz, r0.z, c72, r5
      mad r5.xyz, -r0.y, c74, r5
      add r5.xyz, r5, c75
      mul r0.y, -r5.z, c77.z
      rcp r0.y, r0.y
      mul r6.x, r5.x, r0.y
      mul r0.y, r5.z, c77.w
      rcp r0.y, r0.y
      mul r6.y, r5.y, r0.y
      add r0.yz, -r4, r6.xxyw
      mov r20.x, c221.z
      mul_sat r20.x, r20.x, c80.x
      mul r0.yz, r0, r20.x
      mul r4.xy, r0.yzzw, c5.z
      texld r5, v0, s7
      add r1.w, r5.x, -c86.x
      mul r4.zw, c0.xyxy, v0.xyxy
      mad r4.zw, r3.xyxy, c3.w, r4
      texldl r5, r4.zwzw, s6
      add r2.w, r5.x, c4.x
      mad r4.zw, r4.xyxy, r2.w, v0.xyxy
      mov r5.xyz, r3
      mov r2.w, c2.y
      mov r3.w, c2.y
      rep i0
        mad r6.xy, r4, r3.w, r4.zwzw
        texldl r7, r6, s7
        add r5.w, r7.x, -c86.x
        cmp r5.w, r5.w, c2.w, c2.y
        texldl r6, r6, s2
        mad r5.xyz, r6, r5.w, r5
        add r2.w, r2.w, r5.w
        add r3.w, r3.w, c2.y
      endrep
      rcp r2.w, r2.w
      mul r0.yz, r0, c44.xxyw
      dp2add r0.y, r0.yzzw, r0.yzzw, c1.x
      rsq r0.y, r0.y
      rcp r0.y, r0.y
      mul r0.y, r0.y, c4.w
      mul_sat r0.x, r0.y, r0.w
      mad r4.xyz, r5, r2.w, -r3
      mad r0.xyz, r0.x, r4, r3
      cmp r3.xyz, r1.w, r3, r0
    endif
    texld r1, c1.x, s5
    rcp r0.w, r1.x
    mul r0.w, r0.w, c81.y
    mul r0.xyz, r3, c66.x
    mul r1.xyz, r0.w, r0
    dp3 r1.x, r1, c1.yzww
    mad r0.xyz, r0, r0.w, -r1.x
    mad r0.xyz, c82.x, r0, r1.x
    mul r0.w, r1.x, c84.w
    mul r1.yzw, r0.w, c84.xxyz
    mov_sat r2.x, r0.w
    mad r0.xyz, c84, -r0.w, r0
    mad r0.xyz, r2.x, r0, r1.yzww
    mul r0.xyz, r0, c83
    add r0.xyz, r0, r0
    mov_sat r1.x, r1.x
    mov r8.y, c2.y
    add r0.w, -r8.y, c82.z
    pow r2.x, r1.x, r0.w
    mul_sat r0.xyz, r0, r2.x
    if_ne r31.x, c222.z
      pow r0.x, r0.x, c11.x
      pow r0.y, r0.y, c11.x
      pow r0.z, r0.z, c11.x
    endif
    mul r1.xy, v0.xyyy, c44.xyyy
    mul r1.xy, r1.xyyy, c11.z
    texld r1, r1, s10
    add r1.z, r1.z, c4.x
    mad_sat oC0.xyz, r1.z, c11.y, r0
    mov oC0.w, c2.y

// approximately 202 instruction slots used (22 texture, 180 arithmetic)
