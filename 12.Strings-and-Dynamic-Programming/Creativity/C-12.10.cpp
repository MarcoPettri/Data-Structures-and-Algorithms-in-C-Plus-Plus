// File : C-12.10.cpp

/*
    C-12.10 Say that a pattern P of length m is a circular substring of a text T of
    length n if there is an index 0 \leq i < m, such that P = T [n - m + i..n - 1] +
    T [0..i - 1], that is, if P is a (normal) substring of T or P is equal to the
    concatenation of a suffix of T and a prefix of T . Give an O(n + m)-time
    algorithm for determining whether P is a circular substring of T.
*/
