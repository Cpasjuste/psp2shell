# Description

kuio is a lightweight kernel module for taiHen that allows user modules to access ux0:data for basic I/O operations.<br>
It currently gives abstractions for these functions:

*sceIoOpen* -> *kuIoOpen*<br>
*sceIoWrite* -> *kuIoWrite*<br>
*sceIoRead* -> *kuIoRead*<br>
*sceIoClose* -> *kuIoClose*<br>
*sceIoLseek* -> *kuIoLseek*<br>
*sceIoRemove* -> *kuIoRemove*<br>
*sceIoMkdir* -> *kuIoMkdir*<br>
*sceIoRmdir* -> *kuIoRmdir*<br>
*ftell* -> *kuIoTell* (kuIoLseek doesn't return position)<br><br>

# Credits

Thanks to everyone who helped me during this journey trying to get SD access on user modules on #vitasdk and #henkaku. (noname120, xerpi, yifanlu, davee, xyz, frangarcj)
