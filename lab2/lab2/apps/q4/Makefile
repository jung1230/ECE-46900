default:
	cd spawn_me_prod; make
	cd spawn_me_cons; make
	cd makeprocs; make

clean:
	cd spawn_me_prod; make clean
	cd spawn_me_cons; make clean
	cd makeprocs; make clean

run:
	cd ../../bin; dlxsim -x os.dlx.obj -a -u makeprocs.dlx.obj 3;
