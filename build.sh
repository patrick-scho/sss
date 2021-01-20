# g++ -c ~/cpp/libs/imgui/*.cpp ~/cpp/libs/imgui-sfml/*.cpp -I ~/cpp/libs/imgui -I ~/cpp/libs/imgui-sfml/
g++ src/main.cpp ./*.o -o main -I ~/cpp/libs/imgui -I ~/cpp/libs/imgui-sfml -lsfml-system -lsfml-window -lsfml-graphics -lGLEW -lGL -lassimp
