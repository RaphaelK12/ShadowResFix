float4 gDirectionalColour             : register(c18);
float4 globalScreenSize               : register(c44);
float Exposure                        : register(c66);
row_major float4x4 motionBlurMatrix   : register(c72);
float4 TexelSize                      : register(c76);
float4 dofProj                        : register(c77);
float4 dofDist                        : register(c78);
float4 dofBlur                        : register(c79);
float gDirectionalMotionBlurLength    : register(c80);
float4 ToneMapParams                  : register(c81);
float4 deSatContrastGamma             : register(c82);
float4 ColorCorrect                   : register(c83);
float4 ColorShift                     : register(c84);
float PLAYER_MASK                     : register(c85);
sampler2D GBufferTextureSampler2      : register(ps, s0);
sampler2D GBufferTextureSampler3      : register(ps, s1);
sampler2D HDRSampler                  : register(ps, s2);
sampler2D BloomSampler                : register(ps, s3);
sampler2D AdapLumSampler              : register(ps, s4);
sampler2D JitterSampler               : register(ps, s5);
sampler2D StencilCopySampler          : register(ps, s6);
sampler2D HDRHalfTex                  : register(ps, s8);
sampler2D bluenoisevolume             : register(ps, s9);
sampler2D HDRSampler2                 : register(ps, s11);
sampler2D HDRSampler3                 : register(ps, s13);


float4 globalFogParams       : register( c41 );     
float4 globalFogColor        : register( c42 );     
float4 globalFogColorN       : register( c43 );     




float4 NoiseSale : register(c144);
float4 blueTimerVec4 : register(c218);

float4	 SS_params   : register(c96);
float4	 SS_params2   : register(c99);
float4	 SunDirection   : register(c97);
float4	 SunColor   : register(c98);
                // gWorld[4] = { 0.f };
                // gWorldView[4] = { 0.f };
                // gWorldViewProj[4] = { 0.f };
                // gViewInverse[4] = { 0.f };

row_major float4x4 gWorld         : register(c100);
row_major float4x4 gWorldView     : register(c104);
row_major float4x4 gWorldViewProj : register(c108);
row_major float4x4 gViewInverse   : register(c112);
row_major float4x4 gShadowMatrix  : register(c116);

struct PS_IN
{
	float2 texcoord : TEXCOORD;
	float4 texcoord1 : TEXCOORD1;
};

#define NUM_SAMPLES 50

// gpugems3-chapter-13-volumetric-light-scattering-post-process
float4 SunShafts(float2 uv, float2 ScreenLightPos){
	float Weight = SS_params.x; // 0.3;
	float Density = SS_params.y; // 0.3;
	float exposure = (SS_params.z *0.15) / Exposure; // 0.015;
	float Decay = SS_params.w; // 0.998;
	// Calculate vector from pixel to light source in screen space.
	float2 deltaTexCoord = (uv - ScreenLightPos.xy);
	// Divide by number of samples and scale by control factor.
	deltaTexCoord *= 1.0f / NUM_SAMPLES * Density;
	// Store initial sample.
	float3 color = tex2D(HDRSampler, uv).xyz;
	// Set up illumination decay factor.
	float illuminationDecay = 1.0f;
	float3 SunCol= float3(1.0, 0.3, 0.2);
	float2 ratio = float2(globalScreenSize.x/globalScreenSize.y, 1.0);
	// Evaluate summation from Equation 3 NUM_SAMPLES iterations.
	for (int i = 0; i < NUM_SAMPLES; i++)   {
		// Step sample location along ray.
		uv -= deltaTexCoord;
		// Retrieve sample at new location.
		// HDRSampler, HDRSampler2, HDRSampler3, BloomSampler, HDRHalfTex
		float3 sample = tex2D(HDRSampler, uv) * saturate(pow(tex2D(GBufferTextureSampler3, uv).x, SS_params2.z*50))*
		SunColor * pow(saturate(SS_params2.x*0.1-distance(uv*ratio, ScreenLightPos*ratio)),SS_params2.y*0.1) *SS_params2.w*1*
		saturate(pow((tex2D(HDRSampler3, uv).z)*1, 8))*(1-globalFogParams.w);
		// No sky check
		//float3 sample = tex2D(HDRSampler3, uv) * float3(1.0, 0.3, 0.2);

		// Apply sample attenuation scale/decay factors.
		sample *= illuminationDecay * Weight;
		// Accumulate combined color.
		color += sample;
		// Update exponential decay factor.
		illuminationDecay *= Decay;
	  }
	// Output final color with a further scale control factor.
	return float4( color * exposure, 1);
}
float4 SunShafts2(float2 uv, float2 ScreenLightPos){
	float Weight = SS_params.x; // 0.3;
	float Density = SS_params.y; // 0.3;
	float exposure = (SS_params.z *0.15) / Exposure; // 0.015;
	float Decay = SS_params.w; // 0.998;
	// Calculate vector from pixel to light source in screen space.
	float2 deltaTexCoord = (uv - ScreenLightPos.xy);
	// Divide by number of samples and scale by control factor.
	deltaTexCoord *= 1.0f / NUM_SAMPLES * Density;
	// Store initial sample.
	float3 color = tex2D(HDRSampler2, uv).xyz;
	// Set up illumination decay factor.
	float illuminationDecay = 1.0f;
	float3 SunCol= float3(1.0, 0.3, 0.2);
	// Evaluate summation from Equation 3 NUM_SAMPLES iterations.
	for (int i = 0; i < NUM_SAMPLES; i++)   {
		// Step sample location along ray.
		uv -= deltaTexCoord;
		// Retrieve sample at new location.
		// HDRSampler, HDRSampler2, HDRSampler3, BloomSampler
		float3 sample = tex2D(HDRSampler, uv) * saturate(pow(tex2D(GBufferTextureSampler3, uv).x, 50))*SunColor;
		// No sky check
		//float3 sample = tex2D(HDRSampler3, uv) * float3(1.0, 0.3, 0.2);

		// Apply sample attenuation scale/decay factors.
		sample *= illuminationDecay * Weight;
		// Accumulate combined color.
		color += sample;
		// Update exponential decay factor.
		illuminationDecay *= Decay;
	  }
	// Output final color with a further scale control factor.
	return float4( color * exposure, 1);
}



float4 main(PS_IN i) : COLOR{
	float4 pos = mul( float3(SunDirection.x,-SunDirection.z,SunDirection.y), gWorldViewProj);
	float4 o = tex2D(HDRSampler, i.texcoord);
	// float p = saturate((0.1-distance(float2(pos.x,pos.y), (i.texcoord.xy)*2-1))*10 * saturate(pos.z))*10;

	pos.y = -pos.y;
	pos.xy/=pos.w;
	pos.xy=pos.xy*0.5+0.5;
	//return (0.1-distance(pos.xy, i.texcoord.xy))*30;
	return clamp(SunShafts(i.texcoord, pos.xy)
		* saturate(pos.z * 1.5 - 0.5)
		* saturate(
		  pow(SunDirection.z + 0.15, 2) * 40
		* dot(float2(0.5, 0.5), pos.xy)
	), 0, 20) * Exposure * 10 + o ;
}
