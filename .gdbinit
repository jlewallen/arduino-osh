source ~/tools/bin/micro-trace-buffer.py
target extended-remote :2331
load
b HardFault_Handler
b UsageFault_Handler
b BusFault_Handler
b MemManage_Handler
b NMI_Handler
# b PendSV_Handler
b hard_fault_handler
b os_error
monitor reset
continue
