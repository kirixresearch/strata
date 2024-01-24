'
' Copyright (c) 2009-2022, Kirix Research, LLC.  All rights reserved.
'
' Project:  Build Utilities Script
' Author:   Benjamin I. Williams
' Created:  2009-01-01
'


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
    SetVersion WScript.Arguments.Item(2), WScript.Arguments.Item(3)
ElseIf (WScript.Arguments.Item(1) = "set_version") Then
    SetVersion WScript.Arguments.Item(2), WScript.Arguments.Item(3)
ElseIf (WScript.Arguments.Item(1) = "open") Then
    Set shl = CreateObject("Shell.Application")
    shl.Open(WScript.Arguments.Item(2))
    Set shl = Nothing
Else
    ShowUsage()
End If

WScript.Quit

' SetupVersion function
' Initializes the version by creating a version file if it does not exist and reading its content.
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

' GenerateGuid function
' Generates a new GUID and returns it as a string.
Function GenerateGuid
    Set obj = CreateObject("Scriptlet.TypeLib")
    GenerateGuid = Mid(obj.Guid, 2, 36)
End Function

' IncrementSerialNumber function
' Increments the build serial number by one and updates the version file.
Function IncrementSerialNumber
    SetupVersion()
    VersionBuildSerial = VersionBuildSerial + 1

    ' Read File Contents
    Set f = fs.OpenTextFile(VersionFile, 1)
    s = f.ReadAll()
    Set f = Nothing

    ' Do Increment
    Set r = New RegExp
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

' SetVersion function
' Replaces placeholders in a file with the current version and GUID.
' input_file: The source file path.
' output_file: The destination file path where the modified content will be saved.
Function SetVersion(input_file, output_file)
    Dim line
    Const ForReading = 1
    Set outf = fs.CreateTextFile(output_file, True)
    Set inf = fs.OpenTextFile(input_file, ForReading)

    While Not inf.AtEndOfStream
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

' WriteBuildHeaderFile function
' Writes the current version information to a specified location.
' location: The file path where the version header will be written.
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

' PadDigits function
' Pads a number with zeros on the left up to the specified total digit count.
' n: The number to be padded.
' totalDigits: The total number of digits the result should have.
Function PadDigits(n, totalDigits) 
    PadDigits = Right(String(totalDigits, "0") & n, totalDigits) 
End Function 

' GetQuadVersionString function
' Returns a string representation of the current version in four-number format.
Function GetQuadVersionString
    GetVersionString = VersionMajor & "." & VersionMinor & "." & VersionSubminor & "." & PadDigits(VersionBuildSerial, 4)
End Function

' GetTripleVersionString function
' Returns a string representation of the current version in three-number format.
Function GetTripleVersionString
    GetVersionString = VersionMajor & "." & VersionMinor & "." & VersionSubminor
End Function

' ShowUsage function
' Displays the usage instructions for the script.
Function ShowUsage
    WScript.Echo "Usage: buildutil.vbs {config_dir} {write_config|increment|show_buildnum|show_guid|process_wix|set_version|open}"
End Function
