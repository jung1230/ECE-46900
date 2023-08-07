default:
	cd injection; make
	cd reaction; make
	cd makeprocs; make

clean:
	cd injection; make clean
	cd reaction; make clean
	cd makeprocs; make clean

run:
	cd ../../bin; dlxsim -x os.dlx.obj -a -u makeprocs.dlx.obj 3 4;
