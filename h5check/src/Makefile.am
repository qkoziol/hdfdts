LIB=libh5check.a
LIB_SRC=h5checker.c h5checker_api.c h5error.c
LIB_OBJ=h5checker.o h5checker_api.o h5error.o

$(LIB): $(LIB_OBJ)
	ar cru $@ $(LIB_OBJ)

h5checker.o: h5checker.c h5_check.h

h5checker_api.o: h5checker_api.c h5_check.h

h5error.o: h5error.c h5_error.h

clean:
	rm -f $(LIB_OBJ) $(LIB) *.o h5check 
