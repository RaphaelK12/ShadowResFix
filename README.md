## ShadowResFix

![Alt text](ScreenShots/ShaderEditor_Textures.png?raw=true "Textures")

![Alt text](ScreenShots/ShaderEditor_shaders.png?raw=true "Shaders")

# Fixes and Realtime Shader Editor for GTA4.
Originally ShadowResFix was a small plugin used to test modifications and make some fixes in GTA4 (like fixing the shadow map resolution), recently I added some features, the main one being a real-time shader editor, if you are using the shaders from " Parallellines/FusionFix", the shaders will appear with the names used in the .fxc files, otherwise a crc32 hash will be used to identify them.
The editor uses both shaders that the game loads from .fxc files, and shaders from the "shaders/asm/" and "shaders/hlsl/" folders.

Fixes:
- Fixed GTA4 half-resolution shadows
- Fixed tree leaves not swaying in the wind if the vegetation generates real shadows
- Black raindrops fixed, raindrops now have refraction like xbox (thanks akifle47)
- Possibility to increase the resolution of the paraboloid reflection map.
- Possibility to increase the resolution of the night shadow map.
- Option to correct mirrors with faded/washed reflection on AMD cards.
- Fix issues with near/far plane not being passed to some of the fixed and improved Parallellines shaders.
- Option to use another dxwrapper or dxvk mod with ShadowResFix, just edit the ShadowResFix.ini file and add for example:
```
  EnableProxyLibrary=1
  ProxyLibrary=d3d9_DXVK173.dll
```

*Known issue:*
For some reason the fixes may not work if you use some ProxyLibrary, so you can rename the ShadowResFix.asi file to d3d9.dll and place it together with the GTAIV.exe game executable.
