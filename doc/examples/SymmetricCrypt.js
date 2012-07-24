

//(EXAMPLE)
//Description: Example 1
//Class: SymmetricCrypt
//Code:


// create a password entry dialog
var dialog = new PasswordEntryDialog;
dialog.setCaption("Encryption Example");
dialog.setMessage("Please enter text to encrypt:");

if (!dialog.showDialog())
{
    alert("Cancelled.");
}
else
{
    // encrypt the text with an encryption key and then decrypt it
    var key = "ABCDEF";
    var encrypted_text = SymmetricCrypt.encryptString(dialog.getText(), key);
    var decrypted_text = SymmetricCrypt.decryptString(encrypted_text, key);
    
    // show the key, the encrypted text, and the decrypted text
    var result = "";
    result += "Encryption Key: " + key + "\n";
    result += "Encrypted Text: " + encrypted_text + "\n";
    result += "Decrypted Text: " + decrypted_text;
    alert(result);
}


//(/EXAMPLE)

