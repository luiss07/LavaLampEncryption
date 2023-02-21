# DriverLib Instruction

In order to use the DriverLib, you need to:

1. Extract ```simplelink_msp432p4_sdk_3_40_01_02.zip``` file. 
2. Open CSS and left click on ```Project Folder``` to select ```Properties```
3. Select CSS Build
4. Click ```ARM Compiler``` and then ```Include Options```
5. Add ```simplelink_msp432p4_sdk_3_40_01_02/source``` directory to ```Add dir to #include search path``` window  
6. Click ```ARM Linker``` and ```File Search Path``` 
7. Add ```simplelink_msp432p4_sdk_3_40_01_02/source/ti/devices/msp432p4xx/driverlib/ccs/msp432p4xx_driverlib.lib``` to ```Include library file...``` window
8. Add ```simplelink_msp432p4_sdk_3_40_01_02/source/ti/grlib/lib/ccs/m4f/grlib.a``` to ```Include library file...``` window