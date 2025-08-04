UPFRONT
====================
This repository is a fork of [INIshell](https://inishell.slf.ch/), a general GUI for numerical modeling softwares.
UPFRONT is aimed to be the GUI for the [UPTIGHT](https://github.com/tiberlab/uptight) and [TiberCAD](https://github.com/tiberlab/tibercad) codes.

Installation
====================
Download this repository and unzip it, then go to folder `upfront/build/` and run the binary `inishell` to launch the application.
If you don't know what to do next, press F1 to read the help. A test example is prepared in `upfront/test/` for you to try.

In case the application does not launch, you can try to compile it from source.

Compilation from source on GNU/Linux (Debian-based distros):
--------------------

### From the command line

```bash
sudo apt install build-essential
sudo apt install make
sudo apt install qttools5-dev-tools
sudo apt install libqt5xmlpatterns5-dev
```

```bash
cd inishell-ng/
qmake
make
```

Compilation from source on Windows (using Qt Creator):
--------------------

* Install Qt Creator
* Import inishell.pro
* Select your compiler kit
* Build the project
* Copy needed dlls from Qt installation directory alongside exe

### Static linking

* Requires a static version of Qt

Features
====================
* Intuitive GUI for filling the configuration files for UPTIGHT
* Run UPTIGHT directly within the GUI
* Visualize the results using gnuplot directly by pressing a button