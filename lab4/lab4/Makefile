q2:
	mainframer.sh 'cd one-level/os && make clean'
	mainframer.sh 'cd one-level/os && make default'
	mainframer.sh 'cd one-level/apps/example && make clean'
	mainframer.sh 'cd one-level/apps/example && make'
	mainframer.sh 'cd one-level/apps/example && make run'

two-level-test:
	mainframer.sh 'cd two-level/os && make clean'
	mainframer.sh 'cd two-level/os && make default'
	mainframer.sh 'cd two-level/apps/example && make clean'
	mainframer.sh 'cd two-level/apps/example && make'
	mainframer.sh 'cd two-level/apps/example && make run'

q5:
	mainframer.sh 'cd fork/os && make clean'
	mainframer.sh 'cd fork/os && make default'
	mainframer.sh 'cd fork/apps/example && make clean'
	mainframer.sh 'cd fork/apps/example && make'
	mainframer.sh 'cd fork/apps/example && make run'

clean:
	mainframer.sh 'cd two-level/os && make clean'
	mainframer.sh 'cd two-level/apps/example && make clean'
	mainframer.sh 'cd one-level/os && make clean'
	mainframer.sh 'cd one-level/apps/example && make clean'
	mainframer.sh 'cd fork/os && make clean'
	mainframer.sh 'cd fork/apps/example && make clean'
