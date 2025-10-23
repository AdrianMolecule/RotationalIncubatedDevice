# 
This is the basic code for a generic rotational device like the orbital shaker or tube rotator
it supports temperature and microstepping where available


write platfomio esp32 code that reads a value called sensorValue from the serial and displays it on a HTML screen and changes the value automatically when the value changes.
The screen should display a editable field called screenToBoard that when edited will change a field called screenToBoardValue on the board and prints the changed value to the serial. use asynch calls using ESPAsyncWebServer. use getSsid() external method to get the ssid and getPassword() external method to get the password, both methods are defined in a local pass.h file


Write C++ platfomio esp32 code. use web sockets. create a class with 2 int fields f1 and f2 and an object of that calss called model.
Show the values of the fields as editable html fields with labels in a table in a html page.
When the screen values are updated by editing on the screen the model field values should be updated asynchroniously and the new value should printed at the serial.
When model field values are updated by entering new data in  the serial the screen values  the html values should be updated asynchroniously.
use readSsid() preexisting method to get the ssid and readPassword() preexisting method to get the password, both methods are defined in a local pass.h file