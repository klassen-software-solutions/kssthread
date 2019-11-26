PREFIX := kss
PROJECT_NAME := KSSThread
PROJECT_TITLE := C++ Threading Utilities
LIBS := -lkssutil -lksscontract
TESTLIBS := -lksstest

OS := $(shell uname -s)
ifeq ($(OS),Linux)
    LIBS := $(LIBS) -luuid
endif

include BuildSystem/common.mk
