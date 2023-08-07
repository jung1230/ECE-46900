default:
	cd injection_H2O; make
	cd injection_NO; make
	cd reaction1; make
	cd reaction2; make
	cd reaction3; make
	cd makeprocs; make

clean:
	cd injection_H2O; make clean
	cd injection_NO; make clean
	cd reaction1; make clean
	cd reaction2; make clean
	cd reaction3; make clean
	cd makeprocs; make clean

run:
	cd ../../bin; dlxsim -x os.dlx.obj -a -u makeprocs.dlx.obj 3 0;
