# ShadowResFix
- Fix GTA4 half resolution shadows
- Fix tree leaves not swaying in the wind
- Option to use another dxwrapper or dxvk mod with ShadowResFix, just edit the ShadowResFix.ini file and add for example:
```
  EnableProxyLibrary=1
  ProxyLibrary=d3d9_DXVK173.dll
```

*Known issue:*
For some reason the corrections may not work if you use some ProxyLibrary, so you can rename the ShadowResFix.asi file to d3d9.dll and move it to the folder together with GTAIV.exe (if ShadowResFix.asi and ShadowResFix.ini are in the plugins or scripts folder).
