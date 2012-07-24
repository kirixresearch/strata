

//(EXAMPLE)
//Description: Example 1
//Class: NativeModule
//Code:


// the following demonstrates loading the WIN32 MessageBox
// API call from user32.dll, attaching it to a function,
// and invoking it with parameters

var mod = new NativeModule("user32.dll");

var MB_OKCANCEL = 1;

var Win32MessageBox = mod.getFunction(
                           NativeCall.StdCall,  // calling convention
                           NativeType.Int32,    // return value
                           "MessageBoxA",       // function name
                           NativeType.Int32,    // parent window handle
                           NativeType.String8,  // message body string
                           NativeType.String8,  // message box caption
                           NativeType.Int32);   // message box type
                        
Win32MessageBox(0, "This is a message box", "Win32 Message Box", MB_OKCANCEL);


//(/EXAMPLE)

