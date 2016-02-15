
# V8_TARGET = native
#V8_TARGET = x64.release
V8_TARGET = x64.debug
# DEBUG = -g

ifdef LINUX
CPP=g++
V8_DIR = /home/xaxxon/v8/

V8_LIB_DIR2 = ${V8_DIR}/out/${V8_TARGET}/obj.target/tools/gyp/
V8_LIB_DIR3 = ${V8_DIR}/out/${V8_TARGET}/obj.target/third_party/icu/
V8_LIB_DIR_FLAGS = -L${V8_LIB_DIR2} -L${V8_LIB_DIR3}

LINUX_LIBS = -lpthread -licuuc -licudata -ldl

else

V8_DIR = /Users/xaxxon/v8

CPP=clang++
V8_LIB_DIR = ${V8_DIR}/out/${V8_TARGET}/
V8_LIB_DIR_FLAGS = -L${V8_LIB_DIR}

endif


V8_INCLUDE_DIR = ${V8_DIR}

# Whether you want to use snapshot files, but easier not to use them.  I see a .05s decrease in startup speed by not using them
ifdef USE_SNAPSHOTS
DEFINES = -DUSE_SNAPSHOTS -DV8TOOLKIT_JAVASCRIPT_DEBUG 
V8_LIBS = -lv8_base -lv8_libbase -licudata -licuuc -licui18n -lv8_base -lv8_libplatform -lv8_external_snapshot
else
#DEFINES = -DV8TOOLKIT_JAVASCRIPT_DEBUG 
V8_LIBS = -lv8_base -lv8_libbase -lv8_base -lv8_libplatform -lv8_nosnapshot -licudata -licuuc -licui18n 
endif

CPPFLAGS = -I${V8_INCLUDE_DIR} ${DEBUG} -std=c++14 -I/usr/local/include ${DEFINES} -Wall -Werror

# LIBS = -L/usr/local/lib -L${V8_LIB_DIR}  libv8toolkit.a ${V8_LIBS} -lboost_system -lboost_filesystem
LIBS = -L/usr/local/lib ${V8_LIB_DIR_FLAGS}  libv8toolkit.a ${V8_LIBS} ${LINUX_LIBS}




all: warning thread_sample javascript sample toolbox_sample exception_sample

SRCS=v8toolkit.cpp javascript.cpp

OBJS=$(SRCS:.cpp=.o)


warning:
	$(info )
	$(info )
	$(info THIS MAKEFILE IS FOR MY DEVELOPMENT WORK ONLY; IT IS NOT A GENERAL PURPOSE MAKEFILE)
	$(info You can look at this for an example of what to do, but you'll have to customize it)
	$(info )
	$(info )


thread_sample: lib
	${CPP} -I./ ${CPPFLAGS}  samples/thread_sample.cpp -o samples/thread_sample  ${LIBS}

javascript: lib
	${CPP} -I./ ${CPPFLAGS}  samples/javascript_sample.cpp -o samples/javascript_sample ${LIBS}

sample: lib
	${CPP} -I./ ${CPPFLAGS}  samples/sample.cpp -o samples/sample ${LIBS}

toolbox_sample: lib
	${CPP} -I./ ${CPPFLAGS}  samples/toolbox_sample.cpp -o samples/toolbox_sample ${LIBS}

exception_sample: lib
	${CPP} -I./ ${CPPFLAGS}  samples/exception_sample.cpp -o samples/exception_sample ${LIBS}

bidirectional_sample: lib
	${CPP} -I./ ${CPPFLAGS}  samples/bidirectional_sample.cpp -o samples/bidirectional_sample ${LIBS}


lib: ${OBJS}
	ar cr libv8toolkit.a v8toolkit.o javascript.o

clean:
	rm -f *.o *.a samples/*sample

run:
	(cd samples && ./thread_sample && ./javascript_sample && ./sample && ./toolbox_sample)

clean_docs:
	rm -rf doc	

docs: clean_docs
	mkdir -p docs/html
	doxygen doxygen.cfg


lint:
	cpplint.py --linelength=200 --filter=-whitespace/end_of_line *.cpp *.hpp *.h
