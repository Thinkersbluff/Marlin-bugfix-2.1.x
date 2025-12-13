// Print this gcode file to validate the M48 W1/H functionality
// and to load 8 histories into EEPROM, prior to validating M1128

M420 S0  // Disable Leveling

// Capture a full history (8 records) of M48 output into EEPROM
// Show what M48 history is currently stored in EEPROM, at the end of each run.
M48 P10 V2 E1 L0 W1 // 10 samples, moderate verbosity, engage probe each time, no legs
M48 H // oOnfirm the entry count increments and the latest entry appears.
G4 p1000 // wait 1 second between runs
M48 P10 V2 E1 L0 W1
M48 H
G4 p1000 // wait 1 second between runs
M48 P10 V2 E1 L0 W1
M48 H
G4 p1000 // wait 1 second between runs
M48 P10 V2 E1 L0 W1
M48 H
G4 p1000 // wait 1 second between runs
M48 P10 V2 E1 L0 W1
M48 H
G4 p1000 // wait 1 second between runs
M48 P10 V2 E1 L0 W1
M48 H
G4 p1000 // wait 1 second between runs
M48 P10 V2 E1 L0 W1
M48 H
G4 p1000 // wait 1 second between runs
M48 P10 V2 E1 L0 W1
M48 H
G4 p1000 // wait 1 second between runs

// Perform a 9th cycle, to confirm that the first run is popped off and the new data becomes #8
M48 P10 V2 E1 L0 W1
M48 H

M300 // Beep to indicate end of test
M420 S1 // Enable Leveling