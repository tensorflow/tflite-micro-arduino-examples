*** Settings ***
Suite Setup                     Setup
Suite Teardown                  Teardown
Test Setup                      Reset Emulation
Test Teardown                   Test Teardown
Resource                        ${RENODEKEYWORDS}

*** Variables ***
${UART}                         sysbus.uart0

*** Keywords ***
Create Machine
    Execute Command             mach create
    Execute Command             machine LoadPlatformDescription @platforms/boards/arduino_nano_33_ble.repl

    Execute Command             sysbus LoadELF @${CURDIR}/output/hello_world.ino.elf

*** Test Cases ***
Should Print Brightness Sequence
    Create Machine
    Create Terminal Tester      ${UART}

    Execute Command             machine EnableProfiler @${CURDIR}/hello_world.dump
    Start Emulation

    Wait For Line On Uart       127
    Wait For Line On Uart       260
    Wait For Line On Uart       205
    Wait For Line On Uart       195
    Wait For Line On Uart       10
    Wait For Line On Uart       1
    Wait For Line On Uart       100
