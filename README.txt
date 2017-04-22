Notes on my submission:

1. On line 37 of a2_lib.h please specify:
    1a. The number of pods
    1b. The number of kv_pairs per pod
    1c. The max size of a key
    1d. The max size of a value
   That is required to run your tests

2. Still having issues with the FIFO/Read Order/Read All test, I didn't understand the error messages
   nor could I decipher what the code was actually doing.

3. Since key cycling was a requested feature, the performance of multiple readers is reduced
   because each reader needs mutually exclusive RW access to the read pointer and the previously
   read key

