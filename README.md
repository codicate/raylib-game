# raylib-game

Game made with C++ and raylib

## Getting Started

### Installing Dependencies

[Follow this guide](https://github.com/CapsCollective/raylib-cpp-starter/blob/main/docs/InstallingDependencies.md#installing-dependencies)

Note: on Windows, it's recommended to create a copy of mingw32-make.exe and rename it make.exe. If you don't rename it, you can use `mingw32-make` in place of `make`.  However, in IDE such as CLion, it will insist that `make` is missing.

### Cloning Repo

Running the command below will clone the repo and install [raylib-cpp](https://github.com/RobLoach/raylib-cpp) & [raylib](https://github.com/raysan5/raylib)

```bash
git clone https://github.com/codicate/raylib-game --recursive --shallow-submodules
cd raylib-game
```

Running the command below if you want to clone the repo without installing submodules

```bash
git clone https://github.com/codicate/raylib-game
cd raylib-game
```
```bash
# If you need submodules later, you can run the command below, or install raylib-cpp and raylib manually
make submodules
```

### Compiling Code

[How Makefile work](https://github.com/CapsCollective/raylib-cpp-starter/blob/main/docs/MakefileExplanation.md#how-the-makefile-works)

Run this command once, which will pulls in all raylib and raylib-cpp dependencies, and then formats the project file structure

```bash
make setup
```

Everytime you make a change to the code, the command below will compile and execute the game

```bash
make
```

To open the project folder in CLion:

1. Click `File`, `Open`
2. Go to the directory of the clone
3. Click `Makefile`, `Open as Project`
4. Running `all` configuration (Shift + F10) may not work. You have to set 'Executable' to (...)/bin/app.exe when it prompts you 
5. Go to setting, `Build, Execution, Deployment`, `Makefile`, `Build options`, and put a `.` there. Default is `--jobs=3` and it doesn't compile properly for some reason
6. Or you can just use the commands above inside the intergrated terminal

