2/27/90

This is a preliminary document for the NetTest application.
Some of the descriptions within this document and the current
form of Nettest.exe's APIs are out of date and will be corrected
shortly (specifically the WNetGetErrorText implementation).
Also, the test application has not been updated to reflect the
latest WinNet spec or WinNet.h include file changes.

		The Nettest application provides quick and simple
		access to testing some of the Windows network
		driver funtions and some NetBIOS API.  It is
		not an all encompassing test but is designed to
		facilitate quick and easily analyzed feedback



he integrity of the WinNet.drv and VNetBIOS.386 components being tested.  All activity performed while the application is running is logged to the NETTEST.LOG file in the current directory.  Logging includes the log file name, a time and date stamp, 



a title for each functional section and function as it is performed, and Pass/Fail reporting for the selected commands.  If a test fails then a Windows' messagebox as well as a logged error description are presented.  Below are simple explanations of



 each available command.  Refer back to this section when using this test in section 2.2 of the suite.

	1.61	The File Menu
		 Help
		  Provides a quick description of available commands and the objective of the test the application performs.
		 About
	       Displays current app version number.
		 Exit				  Exits application.
	1.62 The Run Menu [Note: not implemented yet]
		 Batch
		  Prompts user for an output file name and runs all the available commands in the application, logging results to the named file.  A message box is displayed when all the tests are completed.  The default name is Nettest.log.
	1.63	The Misc Menu
		 Browse Dialog Box...
		  Browse is a network driver function which allows the  driver to display one or more dialog boxes that enable the user to select a network resources.  The possible dialogs purtain to a driver defined option (Unknown), a disk tree, a 



print queue, a serial device, and interprocess communication.  If a dialog is not supported a message is presented, otherwise it is displayed.
	      Driver Specific Dialog Box...
		  This command brings up the driver's about box.
	  	 Get Current User Name
		  Displays the name of the current user.
		 Network Capabilities
		  Displays the capabilities of the network as perceived by the Windows network driver and writes this information to the log file.  Use the log file during later portions of the test to verify the correct results of certain driver dep



endent functionality.
	1.64 The Connections Menu
		 Add Net Connection...
		  Allows the user to add a network connection.
		 List Network Connections
		  Lists all the current network connections.
		 Remove Net Connection...
		  Allows the user to remove a network connection.
	1.65 The Errors Menu
		 Get Most Recent Net Error
		  Allows the user to Verify that the driver is in fact returning the correct error codes and text immediately after a driver related network error has occured.  Note that for this function to work correctly it must be call immediately



 after a net error has occurred and not after the driver has reset or changed the current error status.
		 Get Net Error Text
		This dialog allows the user to select an Error Code and Test the returned error text message against NetTest's knowledge of what the network error should be.  For example, from the listbox select WN_BAD_FILE_HANDLE and then select the



 Test button.  NetTest will display the "Correct Text for the Error Code" and also the "Text Returned From the Driver."  It is not necessary that these two messsages match exactly but they should have similar meaning.
	1.66 The Test Menu
		 NetBIOS Test
		  Executing this command will spawn the program NetBIOSW.exe, which must reside in the same directory as NetTest.exe.  NetBIOSW performs some standard NetBIOS functions through Int 5Ch and Int 2Ah.  The results are printed to Com1 as 



well as to the output file NetBIOSW.log.  The output logs the time, the test, and the PASS or FAIL results of each API executed.
		 Count File Handles
		This command will display the number of available file handles in the system.  It is used to make sure that Windows is cleaning up all its open files.

3.1 Nettest.exe Application Script
	Preface: When verifying error results in this section of the script it is neccessary that they be validated.  For example, if you receive a NET ERROR message while attempting to add a network connection you must then go to DOS (or a COMMAND P



rompt) and verify that there is indeed a network error.  If this is not the case then there would appear to be a bug in the driver's error detection.
	If any messages occur which do not appear in this document, please bring them to the attention of Richard Saunders before considering it for Winbug.
	All results are logged to NETTEST.LOG in the current directory.

	3.11 The Misc Menu
	     MENU COMMAND: Browse Dialog Boxes...
			TEST STEPS: Verify that each or the five dialog options are displayed:
			1) UNKNOWN
			2) DISKTREE
			3) PRINTQ
			4) DEVICE
			5) IPC
			6) Disconnect the net cable before selecting the menu command.

				RESULTS: Verify that each possible occurrence displays a driver options dialog or returns one of the following errors:
				1) NOT SUPPORTED - Function not supported
				2) NET ERROR - Nework error
				3) BAD POINTER - Invalid pointer
				4) MORE DATA - Buffer was too small
					
					If a driver options dialog is displayed, test it for proper functionality.  These options will vary from driver to driver.  

		MENU COMMAND: Driver Specific Dialog Box...
			TEST STEPS: Verify text in dialog.
				RESULTS: Verify that the dialog displays correct information (eg. version number corresponds with the one returned by the Network Capabilities funtion, described below)  and that it closes.

		MENU COMMAND: Get Current User Name
			TEST STEPS: Verify text in dialog.
				RESULTS: Verify that the correct user name is displayed correctly.

		MENU COMMAND: Network Capabilities
			TEST STEPS: Verify text in messages and in the main dialog.  Get a printout of file NetTest.log file created upon selection of this command for later usage and functionality verification.
				RESULTS: If commands are reported by Nettest as not supported then verify this with the documentation notes.  When performing this test without the network installed verify that no functions are supported and that the 



command does not hang or crash.  

	3.12	The Connections Menu
		MENU COMMAND: Add Net Connection...
			TEST STEPS:
			1) Enter valid Net Path Name
			2) Enter invalid Net Path Name
			3) Enter valid Local Name
			4) Enter invalid Local Name
			5) Enter valid Password
			6) Enter invalid Password
			7) Enter valid connection, and then enter another valid connection using the same local device name
			8) Disconnect the net cable and make a valid net connection.
				RESULTS: Check for the correct result of the step executed by verifying it with the error codes below:

				1) If no error is given verify that the connection was made by selecting the List Network Connections command from the Connections menu.  Verify that it is indeed accessable.
				2) Not Supported - verify this against NetTest.log.
				3) Net Error - a network error has occurred.	
				4) Bad Net Name - Invalid network resource name.
				5) Bad Local Name - Invalid local device
				6) Bad Password - Invalid Password
				7) Access Denied - Security violation
				8) Already Connected - Local device already connected to a remote resource		

		MENU COMMAND: List Net Connections
			TEST STEPS: Select Get Connections and verify text.
				RESULTS: Verify that the correct information is displayed running a DOS Prompt and performing the USE command (or equivalent) to list all network connections (Note: in order for this to succeed the DOS Prompt must be r



un AFTER the network connections or cancellations are made.

		MENU COMMAND: Remove Net Connection...
			TEST STEPS: 
 			1) Enter valid Local Device Name: A - z, LPT? (must be uppercase for MSNET.DRV), and select Remove (Note: valid and invalid local devices will be influenced by the LastDrive= setting in your config.sys file). 
			2) Enter invalid LocalName: ! - +, etc, and select Remove.
			3) Add a net connection.  Open a text file over on the network with Notepad.  Select Remove.
			4) Add a net connection.  Open a text file over on the network with Notepad.  Return to Nettest and select Force using that particular local device name.
			5) Set the Print Manager option equal to NO (via win.ini or Control Panel/Printers section).  Add a valid network printer name using local device name LPT1, LPT2, or LPT3.  Select this printer as the default Windows printer vi



a Control Panel.  Initiate printing from a Windows application to this network printer.  Select Disconnect Net Connection from Nettest using that LPT port name, select Remove.    
			6) Same as #4 but select Force instead of Remove.
			7) Disconnect the net cable and remove a valid device.
			8) Before entering Windows connect to a net work drive.  Remove that net drive.
				RESULTS: Check for the correct result of the step executed by verifying it with the error codes below:

				1) If no error is given verify this by selecting List Network Connections from the Connections menu.  Also leave Nettest and verify in the Windows File Manager that the connection was cancelled, i.e. no net drive icon 



exists.
				2) Not Supported - verify this against NetTest.log.
				3) Net Error - a network error has occurred.	
				4) Bad Value - not a valid local device name.
				5) Not Connected - device is not a redirected local device or currently accessed network resource.
				6) Open Files - Files are open and the Remove option was selected.  If force is selected the device would have been closed automatically, closing the files with no messages. (Note: under MSNet.drv no error messages are



 given for open files).
				7) If you are running under Windows/386 and you connected to a drive before entering Windows, in attempting to Remove that net connection you should have be presented with the Windows/386 message "You cannot break the 



network connection to drive:..."  Press any key to continue, verify that no disconnecting took place and that you can still access that drive.

	3.13 The Errors Menu
		MENU COMMAND: Get Most Recent Net Error
			TEST STEPS: 
			1. In the Connnections menu select Add Net Connections and enter an invalid Local Name.
			2. In the Connnections menu select Add Net Connections and enter an invalid Net Path name.
			3. In the Connnections menu select Add Net Connections and enter an invalid Password.
			4. In the Connnections menu select Remove Net Connections and enter an invalid Device.
			5. If the driver supports the Force option, open a file over the network (ie. using Notepad).  In the Connnections menu select Remove Net Connections and select Force from drive which Notepad currently has the open file on.
				RESULTS: After each error select Get Most Recent Net Error from the Errors menu and verify that the message returned by the driver correspond to the error situation which you created.  Note that it will vary from drive



r to driver as to how verbose the error reporting will be.  The MSNet.drv for example is not very verbose and returns "Net Error" for most of its error detection.  MSNet.drv also does not support the GetNetError function.

		MENU COMMAND: Get Net Error Text
			TEST STEPS: Select each error code from the Select Error Code listbox and then select the Test option.  
				RESULTS: Verify that the messages displayed in the Correct Text For Error Code field and the Text Returned From Driver are similar and have similar meaning.

	3.14 The Test Menu
		MENU COMMAND: NetBIOS Test
			TEST STEPS: Verify output to COM1. Get a hard copy of the file created NetBIOSW.log.
				RESULTS: Verify:
				1) Output to COM1.
				2) No hang or crash
				3) No failures and all tests logged as passed in NetBIOSW.log.
				4) If the network is not installed verify that functions to not crash but do not PASS or return anything other than 0, where appropriate.

		MENU COMMAND: Count File Handles
			TEST STEPS: 
			1. Before running any other applications, run NetTest and select Count File Handles from the Test menu.  Record the number of available file handles reported.
			2. Run Write.exe and open a file over the network.
			3. Make a change to the file and save it.  Close Write.exe.
			4. Again select Count File Handles from the Test menu.
				RESULTS:  Verify that all the file handles are freed; you do not receive a smaller number in step 4 then in step 1.
