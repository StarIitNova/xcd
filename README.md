# xcd

The cd command we all needed

A drop-in replacement for the cd command allowing you to not only change directories, but also add aliases to your directories with ease.

## Setup

1. Clone this repository
2. Run make (alternatively compile main.cpp any way you want, just make sure the output is named "xcd-a")
3. Add to your environment variables/path

NOTE: The shell command to use this on linux is not setup and xcd is also not tested on any linux distro, use at your own risk (and sanity)

### xcd setup

xcd has setup too! you need a config file to use xcd. To do this, create a directory `xcd` in the same folder as the executable. Inside this folder, create a file `xcd_conf.cf`. That's it for setup!

### configuring xcd

You can add aliases to xcd using the `xcd_conf.cf` file using this simple format: `alias=location`

Some extra styles can be used in the config file, but spaces around equals signs is not supported. You can however add comments (using `#`) and use semicolons (`;`) to put multiple aliases on the same line. Do note a newline also acts as a delimeter for the aliases, so semicolons are not required.

## License

xcd is licensed under the MIT license. See LICENSE.txt for more details
