# SharifAIChallange 2022

```console
$ chmod +x script.sh        # once
$ ./script.sh cmakebuild    # once
$ ./script.sh build         # everytime
$ ./script.sh run main main map0    # main = last compiled agent, otherwise pick from release/
$ ./script.sh publish v0-rc1        # save last compiled agent as release/v0-rc1
$ ./script.sh buildrun ...          # ./script.sh build && ./script.sh run ...
```