CXX      = cl
CXXFLAGS = /std:c++20 /W4 /EHsc /nologo /O2 /MT
LIBS     = psapi.lib user32.lib advapi32.lib

TARGET   = tm.exe
OBJS     = main.obj

all: $(TARGET)

$(TARGET): $(OBJS)
    $(CXX) $(CXXFLAGS) /Fe$@ $** /link $(LIBS)

.cpp.obj:
    $(CXX) $(CXXFLAGS) /c $<

clean:
    @if exist *.obj del /q *.obj
    @if exist $(TARGET) del /q $(TARGET)
    @if exist *.pdb del /q *.pdb

.PHONY: all clean
