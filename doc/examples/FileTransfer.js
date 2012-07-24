

//(EXAMPLE)
//Description: Example 1
//Class: FileTransfer
//Code:


var file = "index.html";
var ftp = new BasicFtpRequest;
var result = ftp.download(file);

Application.run();  // necessary to process events


class BasicFtpRequest
{
    function BasicFtpRequest()
    {
        // initialize the member variables
        m_secure     = false;                   // if true, use sftp; otherwise use ftp
        m_username   = "anonymous";             // username
        m_password   = "anonymous";             // password
        m_server_dir = "ftp-test.mozilla.org/"; // server directory; e.g. "www.domain.com/"
        m_local_dir  = "c:/";                   // local directory; e.g. "c:/temp/"
        m_file       = "";                      // file to download/upload; set later
        
        // create the ftp transfer object, set the file 
        // transfer option to async, and connect the
        // finished event
        m_ftp_transfer = new FileTransfer;
        m_ftp_transfer.setAsync(true);
        m_ftp_transfer.finished.connect(this, onFinished);
    }

    function onFinished()
    {
        // when we're finished, return an alert message
        alert("Finished transfering " + m_file + " to " + m_local_dir + m_file);        
    }

    function download(file)
    {
        // set the file member
        m_file = file;
    
        // create the full source path
        var source = (m_secure ? 's' : '') + 'ftp://'
            + (m_username ? m_username + ":" + m_password + "@" : '')
            + m_server_dir
            + m_file;
        
        // create the full destination path
        var destination = m_local_dir + m_file;

        // issue the download request
        return m_ftp_transfer.download(source, destination);
    }

    function upload(file)
    {
        // set the file member
        m_file = file;
    
        // create the full source path
        var source = m_local_dir + m_file;

        // create the full destination path
        var destination = (m_secure ? 's' : '') + 'ftp://'
            + (m_username ? m_username + ":" + m_password + "@" : '')
            + m_server_dir
            + m_file;

        // issue the upload request
        return m_ftp_transfer.upload(source, destination);
    }

    // member variables
    var m_secure;
    var m_username;
    var m_password;
    var m_server_dir;
    var m_local_dir;
    var m_file;
    var m_ftp_transfer;
}


//(/EXAMPLE)

