workspace "ShadowResFix"
   configurations { "Release", "Debug" }
   platforms { "Win32", "Win64" }
   location "bin"
   objdir ("bin/obj")
   buildlog ("bin/log/%{prj.name}.log")
   buildoptions {"-std:c++latest"}
   
   kind "SharedLib"
   language "C++"
   targetname "ShadowResFix"
   targetextension ".asi"
   characterset ("MBCS")
   flags { "StaticRuntime" }
   
   defines { "rsc_CompanyName=\"FixingIV\"" }
   defines { "rsc_LegalCopyright=\"MIT License\""} 
   defines { "rsc_FileVersion=\"1.0.0.0\"", "rsc_ProductVersion=\"1.0.0.0\"" }
   defines { "rsc_InternalName=\"%{prj.name}\"", "rsc_ProductName=\"%{prj.name}\"", "rsc_OriginalFilename=\"d3d9.dll\"" }
   defines { "rsc_FileDescription=\"https://github.com/RaphaelK12/%{prj.name}\"" }
   defines { "rsc_UpdateUrl=\"https://github.com/RaphaelK12/%{prj.name}\"" }
   
   files { "source/*.h", "source/*.cpp" }
   files { "source/*.def" }
   files { "source/*.rc" }
   includedirs { "source/dxsdk" }
      
   filter "configurations:Debug"
      defines "DEBUG"
      symbols "On"

   filter "configurations:Release"
      defines "NDEBUG"
      optimize "On"
      
   filter "platforms:Win32"
      architecture "x32"
      targetdir "data"
	  libdirs { "source/dxsdk/lib/x86" }
      
   filter "platforms:Win64"
      architecture "x64"
      targetdir "data/x64"
	  libdirs { "source/dxsdk/lib/x64" }

project "ShadowResFix"
