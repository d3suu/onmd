all:
	make onmd-build
	make onmd-test

onmd-build: main_onmd-build.c
	gcc main_onmd-build.c -lssl -lcrypto -o onmd-build

onmd-test: main_onmd-test.c
	gcc main_onmd-test.c -lssl -lcrypto -o onmd-test
