fdisk:
	mainframer.sh 'cd flat/os && make clean'
	mainframer.sh 'cd flat/os && make default'
	mainframer.sh 'cd flat/apps/fdisk && make clean'
	mainframer.sh 'cd flat/apps/fdisk && make'
	mainframer.sh 'cd flat/apps/fdisk && make run'

test:
	mainframer.sh 'cd flat/os && make clean'
	mainframer.sh 'cd flat/os && make default'
	mainframer.sh 'cd flat/apps/ostests && make clean'
	mainframer.sh 'cd flat/apps/ostests && make'
	mainframer.sh 'cd flat/apps/ostests && make run'

clean:
	mainframer.sh 'cd flat/os && make clean'
	mainframer.sh 'cd flat/apps/ostests && make clean'
	mainframer.sh 'cd flat/apps/fdisk && make clean'