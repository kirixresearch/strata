

//(EXAMPLE)
//Description: Example 1
//Class: FileStream
//Code:


// create a binary file
var f = File.open("data.bin",
                  FileMode.Create,
                  FileAccess.ReadWrite,
                  FileShare.None);

// make a buffer with some data to write       
var buf = new MemoryBuffer(3);
buf[0] = 0;
buf[1] = 1;
buf[2] = 2;

// write the buffer to the file
f.write(buf, 0, 3);

// close the file
f.close();

// check to make sure it exists
if (File.exists("data.bin"))
{
    alert("file created successfully.");
}

// open up the file for reading
f = File.open("data.bin",
              FileMode.Open,
              FileAccess.Read,
              FileShare.None);
buf.clear();

// read 3 bytes from the file
f.read(buf, 0, 3);

alert("The following value should be 2.  It is: " + buf[2]);


//(/EXAMPLE)

