# GDB configuration file
# Usage:
# - Launch JLinkGDBServer first:
#   
#     JLinkGDBServer -device YOUR_DEVICE -if (swd|jtag)
#
# - Execute GDB with:
#
#     arm-none-eabi-gdb PATH/TO/ELF_FILE

# Load program into target
target remote localhost:2331
monitor reset
load
disconnect

# Restart target and break at main()
target remote localhost:2331
monitor reset
break main
continue

# Display Test User Interface
tui enable
