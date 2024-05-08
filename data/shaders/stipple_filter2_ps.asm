// Stipple filter + custom mask
// Generated by Microsoft (R) HLSL Shader Compiler 9.26.952.2844
//
// Parameters:
//
//   sampler2D HDRSampler;
//   sampler2D StencilCopySampler;
//   float4 TexelSize;
//
//
// Registers:
//
//   Name                         Reg   Size
//   ---------------------------- ----- ----
//   TexelSize                    c76      1
//   HDRSampler                   s2       1
//   StencilCopySampler           s7       1
//

    ps_3_0
    def c1, 0, 0.212500006, 0.715399981, 0.0720999986
    def c2, 0.25, 1, 256, 0
    def c4, -0.5, -1.5, 1.5, 0.5
	
	def c8, 0.012156862745098, 0.016078431372549, 0.0435294117647059, 0.0474509803921569 // 3.1, 4.1, 11.1, 12.1
	def c9, 0.5141176470588235, 0.5180392156862745, 0.5454901960784314, 0.5494117647058824 // 131.1, 132.1, 139.1, 140.1
	def c10, 0.027843137254902, 0.0007843137254902, 0, 0 // 7.1, 0.2
	
    dcl_texcoord v0.xy
    dcl_2d s2
    dcl_2d s7
	
    mov r3, c4
    mad r4, c76.xyxy, r3.xyzx, v0.xyxy
    mad r6, c76.xyxy, r3.wzyw, v0.xyxy
    texld r7.xyz, v0, s2
    texld r3.xyz, r4.xy, s2
    texld r4.xyz, r4.zw, s2
    texld r5.xyz, r6.xy, s2
    texld r6.xyz, r6.zw, s2
	
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
	
	mov r31, c1.x
	if_ne r31.x, c223.z // Definition mask
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
		
		texldl r22, r21.xw, s7
		add r23, r22.x, -c8
		add_sat r23, -r23_abs, c10.y
		add r20, r20, r23
		add r23, r22.x, -c9
		add_sat r23, -r23_abs, c10.y
		add r20, r20, r23
		add r23.x, r22.x, -c10.x
		add_sat r23.x, -r23_abs, c10.y
		add r20.x, r20, r23
		
		texldl r22, r21.zy, s7
		add r23, r22.x, -c8
		add_sat r23, -r23_abs, c10.y
		add r20, r20, r23
		add r23, r22.x, -c9
		add_sat r23, -r23_abs, c10.y
		add r20, r20, r23
		add r23.x, r22.x, -c10.x
		add_sat r23.x, -r23_abs, c10.y
		add r20.x, r20, r23
		
		texldl r22, r21.xy, s7
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
	
    cmp r0.xz, r0.x, c2.yyww, c2.wwyy
	add r3.xyz, r3, r4
	add r3.xyz, r3, r5
	add r3.xyz, r3, r6
	mul r3.xyz, r3, c2.x
	mul r3.xyz, r3, r0.x
	mad oC0.xyz, r7, r0.z, r3
    mov oC0.w, c2.y

// approximately 176 instruction slots used (14 texture, 162 arithmetic)
