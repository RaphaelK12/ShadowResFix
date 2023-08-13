# ShadowResFix
- Fixed GTA4 half resolution shadows
- Fixed tree leaves not swaying in the wind
- Fixed black raindrops, raindrops now have refraction like xbox (thanks akifle47)
- Option to use another dxwrapper or dxvk mod with ShadowResFix, just edit the ShadowResFix.ini file and add for example:
```
  EnableProxyLibrary=1
  ProxyLibrary=d3d9_DXVK173.dll
```

*Known issue:*
For some reason the corrections may not work if you use some ProxyLibrary, so you can rename the ShadowResFix.asi file to d3d9.dll and move it to the folder together with GTAIV.exe (and ShadowResFix.ini).
