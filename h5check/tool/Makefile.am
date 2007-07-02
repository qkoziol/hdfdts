# This is our main target, the h5ls tool
bin_PROGRAMS=h5check

LIB_SRC=../src/h5checker.c ../src/h5checker_api.c ../src/h5error.c
h5check_SOURCES=$(LIB_SRC) h5checker_main.c

clean:
	rm -f $(LIB_OBJ) $(LIB) *.o h5check
