# Filesystem

## Introduction

Normally when you flash an ESP8266 you only flash the firmware, like for any other microcontroller. But the ESP8266 has plenty of room and normally it is split into different partitions. One such partition is used to store web files like a normal webserver.

Problem is that, even thou the ESP8266 is a very capable microcontroller it has its limitations and does not handle very well concurrent connections. This is specially true for the *default* webserver that comes with the Arduino Core for ESP8266.

On the other side, to provide a good user experience and work with a comfortable development environment you end up having different files: scripts, stylesheet files, images,... The browser will load the index.html file and quickly request for all those other files and the ESP8266 will easily struggle trying to serve them all.

So the trick here is to squeeze them all into one single compressed file just before uploading it. Luckily, we can do that automatically.

## Web interface build process

The build process reads the HTML files, looks for the stylesheet and script files linked there, grabs them, minifies them and includes them inline, in the same order they are loaded. The resulting single HTML file is then cleaned, further minified and compressed resulting in a single index.html.gz file. This way the ESP8266 webserver can serve it really fast.

To build this file we are using **[Gulp][1]**, a build system built in [node.js][2]. So you will need node (and [npm][3], its package manager) first. [Read the documentation][4] on how to install them.

Once you have node and npm installed, go to the 'code' folder and install all the dependencies with:

```
npm install
```

It will take a minute or two. Then you are ready to build the webserver files with:

```
gulp
```

It will create a populate a 'data' folder with all the required files.

## Images

Along with the HTML, CSS and JS files compressed into the index.html.gz file, the ESPurna firmware uses some images in its web interface. These are the favicon.ico file and a **sprite** for the iPhone style buttons in the interface. Again the idea is to use the minimum possible number of files.

## Flashing it

### Using PlatformIO

[PlatformIO][5] allows the developer to define hooks to be executed before or after certain actions. This is really cool since we can plug one such hook to the *uploadfs* target to automatically build the web interface before uploading it to the board.

This is done by specifying the script with the hook definitions in the *extra_script* option in the platformio.ini file. The script, written in python, binds the target file (spiffs.bin) to a method that executes the gulp command we have seen before.

```
#!/bin/python

from SCons.Script import DefaultEnvironment
env = DefaultEnvironment()

def before_build_spiffs(source, target, env):
    env.Execute("gulp buildfs")

env.AddPreAction(".pioenvs/%s/spiffs.bin" % env['PIOENV'], before_build_spiffs)
```

The included platformio.ini file has all this already configured, so you don't have to worry about it. Just type:

```
pio run -t uploadfs -e sonoff
```

(or whatever other enviroment) and you are good to go.

### Using Arduino IDE

First you will have to manually build the data folder contents using gulp (see [instructions above](#web-interface-build-process)).

Then you will have to have the "[ESP8266 Sketch Data Upload][6]" utility installed. Check the instructions in the previous link. The data folder should be a subfolder of the code folder for the tool to find it. Then just execute it and it will upload the data folder contents to your board.


[1]: http://gulpjs.com/
[2]: https://nodejs.org/en/
[3]: https://www.npmjs.com/
[4]: https://docs.npmjs.com/getting-started/installing-node
[5]: http://www.platformio.org
[6]: https://github.com/esp8266/Arduino/blob/master/doc/filesystem.md#uploading-files-to-file-system
