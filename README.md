# Final project pf course 15618@CMU.
We have implemented a lock-free version of red-black tree acoording to the following paper
> Kim J H, Cameron H, Graham P. Lock-free red-black trees using cas

## Summary
We found that there are many flaws within this paper and the pseudocodes are inconsistent. We believe 
that's why there are no experiments in the paper (i.e., the authors didn't manage to implement it) and
why it is hard to implement, especially the MoveUpRule.

However, we managed to implement this algorithm with both insert and remove (linear speedup with multi-threads)
in branch `spacingAndMoveUp`. What's more, we found that the two rules proposed by the paper is unnecessary
that we introduced a much simpler marker mechanism in our report, which will avoid the `double marker problem`
and thus do not need the two rules. The implementation of the simplified version is in branch master.

Also, feel free to read our final report [here](https://github.com/zhangshun97/Lock-free-Red-black-tree/blob/master/final_report.pdf) for implementation details.

## Run test demo
To run tests for lock-free (both insert and remove):

    1. go to the root folder of this repository
    2. run `python3 src/gen_data.py`, this will generate a `data.txt` containing 100,000 numbers
    3. if there's no dir called `build`, then `mkdir build`
    4. run `make`
    5. run `./test_parallel`, it will automatically run tests on both insert and remove functions
       in the case of {1,2,4,8,16} threads and sleeps {0, 0.000001, 0.00001, 0.0001, 0.001} seconds
       between every two operations to provide different contention scenario.

Sample stdout:
```
bash-4.2$ ./test_parallel 
total_size: 100000
time taken by insert with 1 threads and sleep 0 seconds: 5.456105sec
time taken by remove with 1 threads and sleep 0 seconds: 5.492099sec
time taken by insert with 2 threads and sleep 0 seconds: 2.761535sec
time taken by remove with 2 threads and sleep 0 seconds: 2.758071sec
time taken by insert with 4 threads and sleep 0 seconds: 1.382796sec
time taken by remove with 4 threads and sleep 0 seconds: 1.389684sec
time taken by insert with 8 threads and sleep 0 seconds: 0.686128sec
time taken by remove with 8 threads and sleep 0 seconds: 0.696240sec
time taken by insert with 16 threads and sleep 0 seconds: 0.358965sec
time taken by remove with 16 threads and sleep 0 seconds: 0.372839sec

time taken by insert with 1 threads and sleep 1e-06 seconds: 5.519208sec
time taken by remove with 1 threads and sleep 1e-06 seconds: 5.554028sec
time taken by insert with 2 threads and sleep 1e-06 seconds: 2.812645sec
time taken by remove with 2 threads and sleep 1e-06 seconds: 2.819834sec
time taken by insert with 4 threads and sleep 1e-06 seconds: 1.414126sec
time taken by remove with 4 threads and sleep 1e-06 seconds: 1.417157sec
time taken by insert with 8 threads and sleep 1e-06 seconds: 0.711227sec
time taken by remove with 8 threads and sleep 1e-06 seconds: 0.708651sec
time taken by insert with 16 threads and sleep 1e-06 seconds: 0.372079sec
time taken by remove with 16 threads and sleep 1e-06 seconds: 0.388966sec

time taken by insert with 1 threads and sleep 1e-05 seconds: 6.415008sec
time taken by remove with 1 threads and sleep 1e-05 seconds: 6.459631sec
time taken by insert with 2 threads and sleep 1e-05 seconds: 3.269116sec
time taken by remove with 2 threads and sleep 1e-05 seconds: 3.256373sec
time taken by insert with 4 threads and sleep 1e-05 seconds: 1.633940sec
time taken by remove with 4 threads and sleep 1e-05 seconds: 1.639143sec
time taken by insert with 8 threads and sleep 1e-05 seconds: 0.810158sec
time taken by remove with 8 threads and sleep 1e-05 seconds: 0.818031sec
time taken by insert with 16 threads and sleep 1e-05 seconds: 0.477665sec
time taken by remove with 16 threads and sleep 1e-05 seconds: 0.435705sec

time taken by insert with 1 threads and sleep 0.0001 seconds: 15.502029sec
time taken by remove with 1 threads and sleep 0.0001 seconds: 15.520141sec
time taken by insert with 2 threads and sleep 0.0001 seconds: 7.765034sec
time taken by remove with 2 threads and sleep 0.0001 seconds: 7.767849sec
time taken by insert with 4 threads and sleep 0.0001 seconds: 3.891609sec
time taken by remove with 4 threads and sleep 0.0001 seconds: 3.895309sec
time taken by insert with 8 threads and sleep 0.0001 seconds: 1.945151sec
time taken by remove with 8 threads and sleep 0.0001 seconds: 1.948949sec
time taken by insert with 16 threads and sleep 0.0001 seconds: 0.998574sec
time taken by remove with 16 threads and sleep 0.0001 seconds: 0.979559sec

time taken by insert with 1 threads and sleep 0.001 seconds: 111.163769sec
time taken by remove with 1 threads and sleep 0.001 seconds: 111.636538sec
time taken by insert with 2 threads and sleep 0.001 seconds: 56.064795sec
time taken by remove with 2 threads and sleep 0.001 seconds: 56.531595sec
time taken by insert with 4 threads and sleep 0.001 seconds: 28.136038sec
time taken by remove with 4 threads and sleep 0.001 seconds: 28.367063sec
time taken by insert with 8 threads and sleep 0.001 seconds: 14.123047sec
time taken by remove with 8 threads and sleep 0.001 seconds: 14.066496sec
time taken by insert with 16 threads and sleep 0.001 seconds: 7.071987sec
time taken by remove with 16 threads and sleep 0.001 seconds: 7.050332sec
```
