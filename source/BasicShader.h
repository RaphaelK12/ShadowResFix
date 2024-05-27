#pragma once

#include "checksum.h"
#include "Log.h"
#include <map>
#include "dxsdk\d3dx9math.h"

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p)      { if (p) { (p)->Release(); (p)=NULL; } }
#endif

enum ShaderType {
    UnknowShader = 0,
    PS_ASM = 1, // pixel shader asm
    PS_FX,      // pixel shader hlsl
    VS_ASM,
    VS_FX
};

enum ShaderUse {
    SU_FXC=0,   // fxc
    SU_LASM,    // loaded asm
    SU_EASM,    // edited asm
    SU_LFX,     // loaded hlsl
    SU_EFX      // edited hlsl
};

enum ShaderCreationMode {
    SC_FXC=0,   // fxc
    SC_GAME,    // game asm
    SC_LOAD,    // loaded asm
    SC_NEW,     // my asm
};

struct crc_name {
    uint32_t crc32;
    std::string name;
    int id;
    crc_name(std::string _name, uint32_t _crc32, int _id) {
        name = _name;
        crc32 = _crc32;
        id = _id;
    }
};

extern std::vector<crc_name*> crclist_ps;
extern std::vector<crc_name*> crclist_vs;


extern long long g_id;

extern HRESULT SaveFX(std::string fxname, std::string shadername, std::string source, ShaderType shaderType);
extern HRESULT SaveASM(std::string fxname, std::string shadername, std::string source, ShaderType shaderType);
extern int getfxid(int id, std::string oName);

extern std::string LoadFX(std::string fxname, std::string shadername);
extern std::string LoadASM(std::string fxname, std::string shadername);

class basicShader {
public:
    int used = 0;	                // times this shader was used in SetShader(*)
    int id;			                // Shader ID, get with getSignature(), else -1
    int fxid;		                // fx id, used to get name of fxc file
    long long gid;	                // global id
    float constants[256][4] = { 0.f }; // constant table, set with Set*ShaderConstantF
    ShaderUse  usingShader = SU_FXC; // compiled shader to be used
    std::string oName;	            // original name, shader_names_ps[id]
    std::string fxName;	            // name of fcx file without extension

    std::string fxcAsm;	            // original asm from fxc, use GetAsm() or asm file read
    std::string loadedAsm;	        // loaded asm from shaders/asm/.
    std::string editedAsm;	        // edited asm, temp asm

    std::string loadedFx;	        // fx shader source loaded from update/shader/fx/%oName
    std::string editedFx;	        // edited fx

    std::string entryFunction;	    // entry point to compile the shader
    std::string fileName;	        // file name to load this shader, if this shader is a loaded shader
    UINT crc32 = 0;

    bool useNewShader = false;	    // if use new edited/recompiled shader in place
    bool disable = false;		    // use dummy shader
    bool dirt = false;			    // if edited
    bool pixel = false;			    // is pixel shader
    bool useBias = false;		    // use bias
    bool overwriteDepth = false;    // if overwrite depth write
    bool depthWrite = false;	    // if write depth

    // return shader asm
    virtual std::string GetAsm() { return ""; };

    virtual HRESULT setCompiledShaderToUse(ShaderUse s){ return S_FALSE; };
    virtual HRESULT compileShaderSource(std::string source, ShaderType type, ShaderUse use) { return S_FALSE; };

    // compile edited shader source asm or fx
    virtual HRESULT compileNewASM() { return S_FALSE; };
    virtual HRESULT compileNewFx() { return S_FALSE; };

    basicShader() : id(-1), fxid(-1), gid(g_id) { g_id++; }

    bool constantIndex[256] = { 0 };
    std::map<int, D3DXVECTOR4> constantReplace ;

};
