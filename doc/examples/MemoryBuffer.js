

//(EXAMPLE)
//Description: Example 1
//Class: MemoryBuffer
//Code:


// create a memory buffer with 1000 bytes
var buf = new MemoryBuffer(1000);

// fill the buffer with some data
var i;
for (i = 0; i < 1000; ++i)
{
    buf[i] = i;
}

// create a binary file
var stream = File.open("c:\\text.txt",
                       FileMode.Create,
                       FileAccess.ReadWrite,
                       FileShare.None);

// write the bytes to the file     
var bytes_written = stream.write(buf, 0, 1000);


//(/EXAMPLE)

