#!/bin/sh

# g++ -c C:/prg/cpp/libs/imgui/*.cpp C:/prg/cpp/libs/imgui-sfml/*.cpp -I C:/prg/cpp/libs/imgui -I C:/prg/cpp/libs/imgui-sfml -I C:/prg/cpp/libs/SFML-2.5.1/include
g++ src/main.cpp ./bin/*.o -o main -I C:/prg/cpp/libs/SFML-2.5.1/include -I C:/prg/cpp/libs/assimp-5.0.1/include -I C:/prg/cpp/libs/assimp-5.0.1/build/include -I C:/prg/cpp/libs/glew-2.1.0/include -I C:/prg/cpp/libs/glm -I C:/prg/cpp/libs/imgui -I C:/prg/cpp/libs/imgui-sfml -L "C:/prg/cpp/libs/SFML-2.5.1/build/lib/Release" -L "C:/prg/cpp/libs/assimp-5.0.1/build/code/Release" -L "C:/prg/cpp/libs/glew-2.1.0/lib/Release/x64" -lsfml-system -lsfml-window -lsfml-graphics -lGLEW -lGL -lassimp
