CC=h5cc

testgen: testgen.c
	$(CC) $< -o $@

check: testh5check.sh
	sh $<

testh5check.sh: testgen

clean:
	rm -f *.o testgen *.h5 *.data logfile.txt
