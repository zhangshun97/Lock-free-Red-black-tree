# here is our final project pf course 15618@CMU.
# we are going to implement a lock-free version of red-black tree

To run tests for lock-free (both insert and remove):

    1. go to the root folder of this repository
    2. run `python3 src/gen_data.py`, this will generate a `data.txt` containing 100,000 numbers
    3. if there's no dir called `build`, then `mkdir build`
    4. run `make`
    5. run `./test_parallel`, it will automatically run tests on both insert and remove functions
       in the case of {1,2,4,8,16} threads and sleeps {0, 0.000001, 0.00001, 0.0001, 0.001} seconds
       between every two operations to provide different contention scenario.
