# Microsoft Developer Studio Project File - Name="adplug" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** NICHT BEARBEITEN **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=adplug - Win32 Debug
!MESSAGE Dies ist kein gültiges Makefile. Zum Erstellen dieses Projekts mit NMAKE
!MESSAGE verwenden Sie den Befehl "Makefile exportieren" und führen Sie den Befehl
!MESSAGE 
!MESSAGE NMAKE /f "adplug.mak".
!MESSAGE 
!MESSAGE Sie können beim Ausführen von NMAKE eine Konfiguration angeben
!MESSAGE durch Definieren des Makros CFG in der Befehlszeile. Zum Beispiel:
!MESSAGE 
!MESSAGE NMAKE /f "adplug.mak" CFG="adplug - Win32 Debug"
!MESSAGE 
!MESSAGE Für die Konfiguration stehen zur Auswahl:
!MESSAGE 
!MESSAGE "adplug - Win32 Release" (basierend auf  "Win32 (x86) Static Library")
!MESSAGE "adplug - Win32 Debug" (basierend auf  "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "adplug - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_MBCS" /D "_LIB" /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x407 /d "NDEBUG"
# ADD RSC /l 0x407 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Installing library and headers...
PostBuild_Cmds=call vc6inst l "Release\adplug.lib"	call vc6inst i *.h adplug
# End Special Build Tool

!ELSEIF  "$(CFG)" == "adplug - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "_DEBUG" /D "DEBUG" /D "WIN32" /D "_MBCS" /D "_LIB" /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x407 /d "_DEBUG"
# ADD RSC /l 0x407 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"Debug\adplugd.lib"
# Begin Special Build Tool
SOURCE="$(InputPath)"
PostBuild_Desc=Installing library and headers...
PostBuild_Cmds=call vc6inst l "Debug\adplugd.lib"	call vc6inst i *.h adplug
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "adplug - Win32 Release"
# Name "adplug - Win32 Debug"
# Begin Group "Quellcodedateien"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\a2m.cpp
# End Source File
# Begin Source File

SOURCE=.\adlibemu.c
# End Source File
# Begin Source File

SOURCE=.\adplug.cpp
# End Source File
# Begin Source File

SOURCE=.\adtrack.cpp
# End Source File
# Begin Source File

SOURCE=.\amd.cpp
# End Source File
# Begin Source File

SOURCE=.\analopl.cpp
# End Source File
# Begin Source File

SOURCE=.\bam.cpp
# End Source File
# Begin Source File

SOURCE=.\bmf.cpp
# End Source File
# Begin Source File

SOURCE=.\cff.cpp
# End Source File
# Begin Source File

SOURCE=.\d00.cpp
# End Source File
# Begin Source File

SOURCE=.\database.cpp
# End Source File
# Begin Source File

SOURCE=.\debug.c
# End Source File
# Begin Source File

SOURCE=.\dfm.cpp
# End Source File
# Begin Source File

SOURCE=.\diskopl.cpp
# End Source File
# Begin Source File

SOURCE=.\dmo.cpp
# End Source File
# Begin Source File

SOURCE=.\dro.cpp
# End Source File
# Begin Source File

SOURCE=.\dtm.cpp
# End Source File
# Begin Source File

SOURCE=.\emuopl.cpp
# End Source File
# Begin Source File

SOURCE=.\flash.cpp
# End Source File
# Begin Source File

SOURCE=.\fmc.cpp
# End Source File
# Begin Source File

SOURCE=.\fmopl.c
# ADD CPP /W1
# End Source File
# Begin Source File

SOURCE=.\fprovide.cpp
# End Source File
# Begin Source File

SOURCE=.\hsc.cpp
# End Source File
# Begin Source File

SOURCE=.\hsp.cpp
# End Source File
# Begin Source File

SOURCE=.\hybrid.cpp
# End Source File
# Begin Source File

SOURCE=.\hyp.cpp
# End Source File
# Begin Source File

SOURCE=.\imf.cpp
# End Source File
# Begin Source File

SOURCE=.\ksm.cpp
# End Source File
# Begin Source File

SOURCE=.\lds.cpp
# End Source File
# Begin Source File

SOURCE=.\mad.cpp
# End Source File
# Begin Source File

SOURCE=.\mid.cpp
# End Source File
# Begin Source File

SOURCE=.\mkj.cpp
# End Source File
# Begin Source File

SOURCE=.\mtk.cpp
# End Source File
# Begin Source File

SOURCE=.\player.cpp
# End Source File
# Begin Source File

SOURCE=.\players.cpp
# End Source File
# Begin Source File

SOURCE=.\protrack.cpp
# End Source File
# Begin Source File

SOURCE=.\psi.cpp
# End Source File
# Begin Source File

SOURCE=.\rad.cpp
# End Source File
# Begin Source File

SOURCE=.\rat.cpp
# End Source File
# Begin Source File

SOURCE=.\raw.cpp
# End Source File
# Begin Source File

SOURCE=.\realopl.cpp
# End Source File
# Begin Source File

SOURCE=.\rol.cpp
# End Source File
# Begin Source File

SOURCE=.\s3m.cpp
# End Source File
# Begin Source File

SOURCE=.\sa2.cpp
# End Source File
# Begin Source File

SOURCE=.\sng.cpp
# End Source File
# Begin Source File

SOURCE=.\u6m.cpp
# End Source File
# Begin Source File

SOURCE=.\xad.cpp
# End Source File
# Begin Source File

SOURCE=.\xsm.cpp
# End Source File
# End Group
# Begin Group "Header-Dateien"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\a2m.h
# End Source File
# Begin Source File

SOURCE=.\adlibemu.h
# End Source File
# Begin Source File

SOURCE=.\adplug.h
# End Source File
# Begin Source File

SOURCE=.\adtrack.h
# End Source File
# Begin Source File

SOURCE=.\amd.h
# End Source File
# Begin Source File

SOURCE=.\analopl.h
# End Source File
# Begin Source File

SOURCE=.\bam.h
# End Source File
# Begin Source File

SOURCE=.\bmf.h
# End Source File
# Begin Source File

SOURCE=.\cff.h
# End Source File
# Begin Source File

SOURCE=.\d00.h
# End Source File
# Begin Source File

SOURCE=.\database.h
# End Source File
# Begin Source File

SOURCE=.\debug.h
# End Source File
# Begin Source File

SOURCE=.\dfm.h
# End Source File
# Begin Source File

SOURCE=.\diskopl.h
# End Source File
# Begin Source File

SOURCE=.\dmo.h
# End Source File
# Begin Source File

SOURCE=.\dro.h
# End Source File
# Begin Source File

SOURCE=.\dtm.h
# End Source File
# Begin Source File

SOURCE=.\emuopl.h
# End Source File
# Begin Source File

SOURCE=.\flash.h
# End Source File
# Begin Source File

SOURCE=.\fmc.h
# End Source File
# Begin Source File

SOURCE=.\fmopl.h
# End Source File
# Begin Source File

SOURCE=.\fprovide.h
# End Source File
# Begin Source File

SOURCE=.\hsc.h
# End Source File
# Begin Source File

SOURCE=.\hsp.h
# End Source File
# Begin Source File

SOURCE=.\hybrid.h
# End Source File
# Begin Source File

SOURCE=.\hyp.h
# End Source File
# Begin Source File

SOURCE=.\imf.h
# End Source File
# Begin Source File

SOURCE=.\imfcrc.h
# End Source File
# Begin Source File

SOURCE=.\kemuopl.h
# End Source File
# Begin Source File

SOURCE=.\ksm.h
# End Source File
# Begin Source File

SOURCE=.\lds.h
# End Source File
# Begin Source File

SOURCE=.\mad.h
# End Source File
# Begin Source File

SOURCE=.\mid.h
# End Source File
# Begin Source File

SOURCE=.\mididata.h
# End Source File
# Begin Source File

SOURCE=.\mkj.h
# End Source File
# Begin Source File

SOURCE=.\mtk.h
# End Source File
# Begin Source File

SOURCE=.\opl.h
# End Source File
# Begin Source File

SOURCE=.\player.h
# End Source File
# Begin Source File

SOURCE=.\players.h
# End Source File
# Begin Source File

SOURCE=.\protrack.h
# End Source File
# Begin Source File

SOURCE=.\psi.h
# End Source File
# Begin Source File

SOURCE=.\rad.h
# End Source File
# Begin Source File

SOURCE=.\rat.h
# End Source File
# Begin Source File

SOURCE=.\raw.h
# End Source File
# Begin Source File

SOURCE=.\realopl.h
# End Source File
# Begin Source File

SOURCE=.\rol.h
# End Source File
# Begin Source File

SOURCE=.\s3m.h
# End Source File
# Begin Source File

SOURCE=.\sa2.h
# End Source File
# Begin Source File

SOURCE=.\silentopl.h
# End Source File
# Begin Source File

SOURCE=.\sng.h
# End Source File
# Begin Source File

SOURCE=.\u6m.h
# End Source File
# Begin Source File

SOURCE=.\xad.h
# End Source File
# Begin Source File

SOURCE=.\xsm.h
# End Source File
# End Group
# End Target
# End Project
