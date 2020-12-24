ALL: SIMULATOR 

SIMULATOR: simulator.c
		mpicc simulator.c base.c node.c utils.c -lpthread -lm -o simulator_out
run:
	mpirun -np 5 -oversubscribe simulator_out -n 2 -m 2 -i 20 -v 5

clean:
	/bin/rm -f simulator_out *.o 
	/bin/rm -f *.txt