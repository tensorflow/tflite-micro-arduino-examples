*** Settings ***
Suite Setup                         Setup
Suite Teardown                      Teardown
Test Setup                          Reset Emulation
Test Teardown                       Test Teardown
Resource                            ${RENODEKEYWORDS}

*** Keywords ***
Should Run Detection
    Wait For Line On Uart           Attempting to start Arducam
    Wait For Line On Uart           Starting capture
    Wait For Line On Uart           Image captured
    Wait For Line On Uart           Reading \\d+ bytes from Arducam                   treatAsRegex=true
    Wait For Line On Uart           Finished reading
    Wait For Line On Uart           Decoding JPEG and converting to greyscale
    Wait For Line On Uart           Image decoded and processed
    ${l}=  Wait For Line On Uart    Person score: (-?\\d+) No person score: (-?\\d+)      treatAsRegex=true
    ${s}=  Evaluate                 int(${l.groups[0]}) - int(${l.groups[1]})

    [return]                        ${s}

Run Test
    [Arguments]                     ${image}

    Execute Command                 Clear
    Execute Command                 mach create
    Execute Command                 machine LoadPlatformDescription @platforms/boards/arduino_nano_33_ble.repl
    Execute Command                 sysbus LoadELF @${CURDIR}/output/person_detection.ino.elf

    Create Terminal Tester          sysbus.uart0
    Execute Command                 sysbus.spi2.camera ImageSource @${image}
    Execute Command                 machine EnableProfiler "${CURDIR}/person_detection.dump"
    Start Emulation

Detect Template
    [Arguments]                     ${image}

    Run Test                        ${image}
    ${r}=  Should Run Detection
    Should Be True                  ${r} > 30

No Detect Template
    [Arguments]                     ${image}

    Run Test                        ${image}
    ${r}=  Should Run Detection
    Should Be True                  ${r} < -30

*** Test Cases ***
Should Detect Person
    [Template]                      Detect Template

    ${CURDIR}/test_files/person_image_0.jpg
    ${CURDIR}/test_files/person_image_1.jpg

Should Not Detect Person
    [Template]                      No Detect Template

    ${CURDIR}/test_files/no_person_image_0.jpg
    ${CURDIR}/test_files/no_person_image_1.jpg
