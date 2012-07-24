

//(EXAMPLE)
//Description: Example 1
//Class: HostPreferences
//Code:


// instantiate a preferences object
var prefs = new HostPreferences;

// set an preference value
prefs.setValue("myextension.mysection.mypref", "This is a test");

// get a preference value
var value = prefs.getValue("myextension.mysection.mypref");

// display it
alert(value);

// get all preferences
var preferences;
var all_prefs = prefs.getAll();
for (var pref_name in all_prefs)
{
    preferences += pref_name;
    preferences += " equals ";
    preferences += all_prefs[pref_name];
    preferences += "\n";
}

alert(preferences);


//(/EXAMPLE)

