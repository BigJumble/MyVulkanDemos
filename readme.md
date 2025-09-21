on linux get gcc-14 and g++-14, idk what on windows and mac
on vscode
get CMake Tools, clangd
setup cmaketools to use gcc-14, ctrl + shift + p, then cmake: select a kit
f7 to build
f5 to run

setup f5 - create launch.json via prompt in Run and Debug tab and edit "program" line to - "program": "${workspaceFolder}/build/MyEngine",