LIB=libh5check.a
LIB_SRC=h5checker.c h5error.c h5checker_api.c
LIB_OBJ=h5checker.o h5error.o h5checker_api.o

$(LIB): $(LIB_OBJ)
	ar cru $@ $(LIB_OBJ)

h5check: h5checker_main.o $(LIB)
	$(CC) $< $(LIB) -o $@

h5checker.o: h5checker.c h5_check.h

h5error.o: h5error.c h5_error.h

clean:
	rm -f $(LIB_OBJ) $(LIB) *.o h5check 

#cc -c h5checker.c h5error.c h5checker_api.c
#ar cru libcheck.a h5checker.o h5error.o h5checker_api.o
#cc h5checker_main.c -o h5check libcheck.a

