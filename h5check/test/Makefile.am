H5CC=h5cc

testgen: testgen.c
	$(H5CC) $< -o $@
	./testgen

check: testh5check.sh
	sh $<

testh5check.sh: testgen

clean:
	rm -f *.o testgen *.h5 *.data logfile.txt h5check.*.out h5check.*.err

