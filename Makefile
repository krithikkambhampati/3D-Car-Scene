CXX      = g++
CXXFLAGS = -std=c++17 -O2 -Wall \
           -Iglad/include \
           -Isrc

LDFLAGS  = -lglfw -lGL -ldl

SRC = main.cpp \
      glad/src/glad.c \
      src/shader.cpp \
      src/texture.cpp \
      src/mesh.cpp \
      src/collision.cpp \
      src/car.cpp \
      src/track.cpp \
      src/building.cpp \
      src/fan.cpp \
      src/light_source.cpp \
      src/wall.cpp \
      src/camera_system.cpp \
      src/input_handler.cpp \
      src/world.cpp

OUT = app3d

all: $(OUT)

$(OUT): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) $(LDFLAGS) -o $(OUT)

run: $(OUT)
	./$(OUT)

clean:
	rm -f $(OUT)

.PHONY: all run clean
