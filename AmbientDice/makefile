target = dice.exe
flags = -std=c++17 -g -O3
CC = g++

src = $(wildcard ./*.cpp ../Common/Soft/Buffer/*.cpp ../Common/Soft/Display/*.cpp ../Common/Soft/Math/*.cpp ../Common/Soft/Sampler/*.cpp ../Common/Ext/stb_image/*.cpp)

obj = $(patsubst %.cpp, %.o, $(src))

inc = $(patsubst %.cpp, %.h, $(src))

all : $(target)

$(target) : $(obj)
	$(CC) $(obj) -o $(target) $(flags)

%.o : %.cpp
	$(CC) -c $< -o $@ $(flags)

%.cpp : %.h

clean :
	rm ./*o ../Common/Soft/Buffer/*.o ../Common/Soft/Display/*.o ../Common/Soft/Math/*.o ../Common/Soft/Sampler/*.o