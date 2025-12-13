G28                ; home all axes
; G1 X117 Y117 F3000 ; move to probe location (adjust as needed)
M190 S60           ; (optional) set bed temp if you want thermal conditions
; M109 S200          ; (optional) wait for nozzle temp
M48 P50 V4         ; probe repeatability test: 50 probes, verbose level 4