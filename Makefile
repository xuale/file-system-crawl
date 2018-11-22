# NAME: Richard Yang, Alexander Xu
# EMAIL: ryang72@ucla.edu, xualexander1@gmail.com
# ID: 704936219, 504966392

default: build

build:
	gcc -Wall -Wextra -g lab3a.c -o lab3a
dist:
	tar -cvzf lab3a-704936219.tar.gz lab3a.c ext2_fs.h Makefile README
clean:
	rm -f lab3a lab3a-704936219.tar.gz
