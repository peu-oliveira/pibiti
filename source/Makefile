################################################################################
# Makefile include to help find GL Libraries
#include ./findgllib.mk
#include ./make.inc

INCLUDES 		+= -Iexternal,./pch
ALL_CCFLAGS 	+= -w 
ALL_LDFLAGS 	+= -maxrregcount=0 -cudart static
GENCODE_FLAGS 	+= -w -gencode=arch=compute_30,code=\"sm_30,compute_30\" -x cu 
LIBRARIES 		= -lGLEW -lglut -lGLU -lGL 
NVCC 			= /usr/local/cuda/bin/nvcc -ccbin g++

APP_SOURCES = App/*.cpp
CUDA_SOURCES = CUDA/radixsort.cu CUDA/System.cu
GRAPHICS_SOURCES = Graphics/*.cpp
PCH_SOURCES = pch/*.cpp
SPH_SOURCES = SPH/*.cpp
EXTERNAL_SOURCES = external/*.cpp

################################################################################

# Target rules
all: SPH

SPH: $(APP_SOURCES) $(CUDA_SOURCES) $(GRAPHICS_SOURCES) $(PCH_SOURCES) $(SPH_SOURCES) $(EXTERNAL_SOURCES)
	$(NVCC) $(INCLUDES) $(ALL_LDFLAGS) $(GENCODE_FLAGS) -o bin/$@ $+ $(LIBRARIES)
	
clean:
	rm -f bin/SPH *~ *.*~ 

