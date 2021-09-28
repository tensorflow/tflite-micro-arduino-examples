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

    Execute Command             sysbus LoadELF @${CURDIR}/output/magic_wand.ino.elf

*** Test Cases ***
Should Detect RING Motion
    Create Machine
    Create Terminal Tester      ${UART}

    Execute Command             sysbus.twi0.lsm9ds1_imu FeedAccelerationSample @${CURDIR}/test_files/circle_rotated.data

    Execute Command             machine EnableProfiler "${CURDIR}/magic_wand_ring.dump"
    Start Emulation
    Wait For Line On Uart       Magic starts
    Wait For Line On Uart       RING:

Should Detect SLOPE Motion
    Create Machine
    Create Terminal Tester      ${UART}

    Execute Command             sysbus.twi0.lsm9ds1_imu FeedAccelerationSample @${CURDIR}/test_files/angle_rotated.data

    Execute Command             machine EnableProfiler "${CURDIR}/magic_wand_slope.dump"
    Start Emulation
    Wait For Line On Uart       Magic starts
    Wait For Line On Uart       SLOPE:
