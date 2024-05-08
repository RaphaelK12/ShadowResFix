workspace "ShadowResFix"
   configurations { "Release", "Debug" }
   platforms { "Win32", "Win64" }
   location "build"
   objdir ("build/obj")
   buildlog ("build/log/%{prj.name}.log")
   buildoptions {"-std:c++latest"}
   
   disablewarnings { "4018" }

   kind "SharedLib"
   language "C++"
   targetname "d3d9"
   targetextension ".dll"
   characterset ("MBCS")
   flags { "StaticRuntime" }
   
   defines { "_CRT_SECURE_NO_WARNINGS" }
   
   defines { "rsc_CompanyName=\"FixingIV\"" }
   defines { "rsc_LegalCopyright=\"MIT License\""} 
   defines { "rsc_FileVersion=\"1.0.0.8\"", "rsc_ProductVersion=\"1.0.0.8\"" }
   defines { "rsc_InternalName=\"%{prj.name} IM\"", "rsc_ProductName=\"%{prj.name} IM\"", "rsc_OriginalFilename=\"d3d9.dll\"" }
   defines { "rsc_FileDescription=\"https://github.com/RaphaelK12/%{prj.name}\"" }
   defines { "rsc_UpdateUrl=\"https://github.com/RaphaelK12/%{prj.name}\"" }
   
   files { "source/*.h", "source/*.cpp" }
   files { "source/*.def" }
   files { "source/*.rc" }
   
   links "dinput8.lib"
   links "dxguid.lib"
   links "version.lib"
	  
   includedirs { "source/dxsdk" }
      
   filter "configurations:Debug"
      defines "DEBUG"
      symbols "On"

   filter "configurations:Release"
      defines "NDEBUG"
      optimize "speed"

   filter "platforms:Win32"
      architecture "x32"
      targetdir "data"
	  libdirs { "source/dxsdk/lib/x86" }
      
   filter "platforms:Win64"
      architecture "x64"
      targetdir "data/x64"
	  libdirs { "source/dxsdk/lib/x64" }
project "ShadowResFix"

