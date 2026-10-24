// File : C-9.6.cpp

/*
    C-9.6 The hash table dictionary implementation requires that we find a prime
    number between a number M and a number 2M. Implement a function for
    finding such a prime by using the sieve algorithm. In this algorithm, we
    allocate a 2M cell Boolean array A, such that cell i is associated with the
    integer i. We then initialize the array cells to all be “true” and we “mark
    off” all the cells that are multiples of 2, 3, 5, 7, and so on. This process
    can stop after it reaches a number larger than \sqrt{2M}.
    (Hint: Consider a bootstrapping method for computing the primes up to\sqrt{2M}.)
*/
