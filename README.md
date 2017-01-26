# MultiplicationProject
## Downloading this repository

**TODO**
## Testing the code
### Prerequisites

	* `python v2.7` ( for automatic creation of data and testing )
	* `clang` ( to compile the C files within this repository )
	* A Unix command line ( Certain POSIX functions are required for the timer. )
	* [GMP](https://gmplib.org)
	* [FLINT](http://flintlib.org)

### Compiling the Programs

	1. Open up your Unix command line.  On Windows, you need to
		install a third party command-line like [Cygwin](http://cygwin.com)
	2. Get the absolute paths of the GMP and FLINT libraries and the
		directories of their headers.  The particulars may be
		different for your particular command-line. Remember that
		Google Search is your friend.
	3. To compile a multiplication algorithm, type
		`clang -I<GMP header directory> -l<FLINT header directory>
		-L<Directory containing GMP Library> -L<Directory containing
		FLINT Library> -Os <multiplication algorithm>.c -o
		<multiplication algorithm>`
	4. To compile the timer, type `clang -O3 ctime.c -o ctime`.
	
### Creating data to test

	Create a folder called `data` in the repository.

	In your Unix command line type `python create_data.py`.  This will
    take some time to do.  Once done, you should see files with labels
    corresponding to powers of two from 64 to 4096 and extension
    `.bz2`.

### Testing the algorithms

	Inside the `data` folder create one sub-folder for each
    multiplication algorithm you want to test.

	In your Unix command line type `python test_programs.py
    <multiplication algorithms>`.  This will take a while, like the
    last command.  When done, the directories you created in the
    `data` folder will contain .CSV files with the same labels as the files
    you created in the main `data` directory.
