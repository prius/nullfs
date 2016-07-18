LDFLAGS=-lfuse -lstdc++

nul1fs:
	cc $(LDFLAGS) -o nul1fs nul1fs.c

clean:
	rm -f nul1fs
