




Set fs = CreateObject("Scripting.FileSystemObject")


If WScript.Arguments.Count < 2 Then
    ShowUsage()
    WScript.Quit
End If



' Parameters
ConfigDirectory = WScript.Arguments.Item(0)

VersionFile = ConfigDirectory & "\build_version.txt"
VersionMajor = 1            ' must be >= 0 and <= 255
VersionMinor = 0            ' must be >= 0 and <= 9
VersionSubminor = 0         ' must be >= 0 and <= 9
VersionBuildSerial = 0      ' must be >= 0 and <= 9999


SetupVersion()



If (WScript.Arguments.Item(1) = "write_config") Then
    WriteBuildHeaderFile(WScript.Arguments.Item(2))
ElseIf (WScript.Arguments.Item(1) = "increment") Then
    IncrementSerialNumber()
ElseIf (WScript.Arguments.Item(1) = "show_buildnum") Then
    WScript.Echo(VersionBuildSerial)
ElseIf (WScript.Arguments.Item(1) = "show_guid") Then
    WScript.Echo(GenerateGuid())
ElseIf (WScript.Arguments.Item(1) = "process_wix") Then
    ProcessWixFile WScript.Arguments.Item(2), WScript.Arguments.Item(3)
ElseIf (WScript.Arguments.Item(1) = "open") Then
        set shl = CreateObject("Shell.Application")
        shl.Open(WScript.Arguments.Item(2))
        set shl = nothing
Else
    ShowUsage()
End If

WScript.Quit




Function SetupVersion

    If Not fs.FileExists(VersionFile) Then
        Set f = fs.CreateTextFile(VersionFile, True)
        f.WriteLine("VersionMajor = 1            ' must be >= 0 and <= 255")
        f.WriteLine("VersionMinor = 0            ' must be >= 0 and <= 9")
        f.WriteLine("VersionSubminor = 0         ' must be >= 0 and <= 9")
        f.WriteLine("VersionBuildSerial = 0      ' must be >= 0 and <= 9999")
        f.Close
    End If

    Set f = fs.OpenTextFile(VersionFile, 1)
    s = f.ReadAll()
    ExecuteGlobal s
    Set f = Nothing

End Function





Function GenerateGuid

	set obj = CreateObject("Scriptlet.TypeLib")
	GenerateGuid = Mid(obj.Guid,2,36)

End Function




Function IncrementSerialNumber

    SetupVersion()
    VersionBuildSerial = VersionBuildSerial + 1


    ' Read File Contents
    Set f = fs.OpenTextFile(VersionFile, 1)
    s = f.ReadAll()
    Set f = Nothing

    
    ' Do Increment
    Set r = new RegExp
    r.Pattern = "^(VersionBuildSerial = )(\d+)(.*)$"
    r.Global = True
    r.IgnoreCase = True
    r.Multiline = True

    s = r.Replace(s, "$1" & VersionBuildSerial & "$3")


    Set f = fs.CreateTextFile(VersionFile, True)
    f.WriteLine(s)
    f.Close()
    
    IncrementSerialNumber = VersionBuildSerial
 
End Function





Function ProcessWixFile(input_file, output_file)

    Dim guid
    Dim line
    Const ForReading = 1
    Set outf = fs.CreateTextFile(output_file, True)
    Set inf = fs.OpenTextFile(input_file, ForReading)

    While not inf.AtEndOfStream
        line = inf.ReadLine

        If InStr(line, "%GUID%") <> 0 Then
            line = Replace(line, "%GUID%", GenerateGuid())
        End If

        If InStr(line, "%VERSION%") <> 0 Then
            line = Replace(line, "%VERSION%", GetQuadVersionString())
        End If
		
		If InStr(line, "%VERSIONQUAD%") <> 0 Then
            line = Replace(line, "%VERSIONQUAD%", GetQuadVersionString())
        End If
		
		If InStr(line, "%VERSIONTRIPLE%") <> 0 Then
            line = Replace(line, "%VERSIONTRIPLE%", GetTripleVersionString())
        End If

        outf.WriteLine(line)
    Wend

    inf.Close
    outf.Close

End Function



Function WriteBuildHeaderFile(location)

    Set f = fs.CreateTextFile(location, True)
    
    f.WriteLine("")
    f.WriteLine("// version info for main application executable")
    f.WriteLine("#define APP_VERSION_MAJOR " & VersionMajor)
    f.WriteLine("#define APP_VERSION_MINOR " & VersionMinor)
    f.WriteLine("#define APP_VERSION_SUBMINOR " & VersionSubminor)
    f.WriteLine("#define APP_VERSION_BUILDSERIAL " & VersionBuildSerial)
    f.WriteLine("")

    f.Close

End Function






Function PadDigits(n, totalDigits) 
    PadDigits = Right(String(totalDigits,"0") & n, totalDigits) 
End Function 

Function GetQuadVersionString
    GetVersionString = VersionMajor & "." & VersionMinor & VersionSubminor & "." & PadDigits(VersionBuildSerial,4)
End Function

Function GetTripleVersionString
    GetVersionString = VersionMajor & "." & VersionMinor & VersionSubminor
End Function

Function ShowUsage
    WScript.Echo "Usage:  buildutil.vbs {config_dir} {write_config|increment|show_buildnum}"
End Function

