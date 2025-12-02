CXX=cl
CXXFLAGS=/std:c++20 /W4 /EHsc /nologo
LIBS=psapi.lib

OBJS=main.obj
TARGET=tm.exe

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) /Fe$@ $(OBJS) /link $(LIBS)

main.obj: main.cpp
	$(CXX) $(CXXFLAGS) /c main.cpp

clean:
	del /q $(OBJS) $(TARGET) 2>nul
