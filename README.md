# mynewt-calliope
Experimental apache mynewt bsp, drivers and apps for calliope mini: button, oled ssd1306, si1145, led-matrix 


## usage
### Make the repo available locally in your project.yml
```

project.repositories:
    - apache-mynewt-core
    - mynewt-de-schilken-calliope

repository.mynewt-de-schilken-calliope:
    type: github
    vers: 0-latest
    user: schilken
    repo: mynewt-calliope
```

NOTE:
Before the first start of a mynewt programm on a fresh calliope, I had to do an full erase of the calliope.
I used "pyocd-flashtool -d debug -t nrf51 -ce"

Maybe a "telnet localhost 4444" to connect to a running openocd 
and enter "nrf51 mass_erase" could also work.

If you are not (yet:-) used to the mynewt workflow:
See the excellent documentation at http://mynewt.apache.org/latest/os/introduction/
to install the newt tool, the cross toolchain for ARM 
and load a bootloader before running one of the apps.




### There are two apps
apps/mydrivertest which uses the mynewt shell to issue commands to the drivers
apps/bleadc which uses adc to read an analog value and publish it as bluetooth characteristic

### There are also two targets
target/mydrivertest_calliope
target/bleadc_calliope

```
newt target create mydrivertest_calliope
newt target set mydrivertest_repo \
	app=@mynewt-de-schilken-calliope/apps/mydrivertest \
	bsp=@mynewt-de-schilken-calliope/hw/bsp/calliope_mini \
	build_profile=debug
newt target show mydrivertest_calliope
```

### build and deploy it to a connected calliope 
newt run mydrivertest_calliope 0.7.7


### Connect a terminal to the usb-uart ( I use CoolTerm on m mac :-)

Try out the commands:
* enter ? to see the commands
* write string to calliope 5x5 LED matrix
* write character to calliope 5x5 LED matrix
* react on button press with writing to LEDs
* scan i2c-bus
* set gpio digital values
* write text to a connected oled ( ssd1306 )
* read uv index of connected SI1145 ( I use the one of adafruit )


```
33659: > ?

Commands:
33842:     stat    config       log      imgr      echo         ? 
33843:   prompt     ticks     tasks  mempools      date      gpio 
33845:      i2c      oled        uv    matrix      rgb

33846: > gpio
usage: gpio <pin> <onOff>, <pin>: P0..P20 oder p0..p30, onOff: 0 oder 1

35569: > i2c
i2c probe|scan

36258: > oled
oled clr|p <string>

Before using oled you need to enter the command: oled init

36737: > uv
uv check|r

37232: > matrix

matrix c <char> | p <string>

1777: > rgb

usage: rgb r|g|b

you can also set the color as six hexdigits like: rgb 004466
```


