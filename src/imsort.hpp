//
//  imsort.hpp
//  lsm-tree
//
//  Created by Aron Szanto on 3/20/17.
//  Copyright © 2017 Aron Szanto. All rights reserved.
//

#ifndef imsort_h
#define imsort_h
#include "run.hpp"

template <typename K, typename V>
void swap(KVPair<K, V>* xs, int i, int j) {
    KVPair<K, V> tmp = xs[i]; xs[i] = xs[j]; xs[j] = tmp;
}

/*
 * merge two sorted subs xs[i, m) and xs[j...n) to working area xs[w...]
 */
template <typename K, typename V>

void wmerge(KVPair<K,V>* xs, int i, int m, int j, int n, int w) {
    while (i < m && j < n)
        swap(xs, w++, xs[i] < xs[j] ? i++ : j++);
    while (i < m)
        swap(xs, w++, i++);
    while (j < n)
        swap(xs, w++, j++);
}
template <typename K, typename V>
void imsort(KVPair<K,V>* xs, int l, int u);

/*
 * sort xs[l, u), and put result to working area w.
 * constraint, len(w) == u - l
 */
template <typename K, typename V>
void wsort(KVPair<K,V>* xs, int l, int u, int w) {
    int m;
    if (u - l > 1) {
        m = l + (u - l) / 2;
        imsort(xs, l, m);
        imsort(xs, m, u);
        wmerge(xs, l, m, m, u, w);
    }
    else
        while (l < u)
            swap(xs, l++, w++);
}
template <typename K, typename V>
void imsort(KVPair<K,V>* xs, int l, int u) {
    int m, n, w;
    if (u - l > 1) {
        m = l + (u - l) / 2;
        w = l + u - m;
        wsort(xs, l, m, w); /* the last half contains sorted elements */
        while (w - l > 2) {
            n = w;
            w = l + (n - l + 1) / 2;
            wsort(xs, w, n, l);  /* the first half of the previous working area contains sorted elements */
            wmerge(xs, l, l + n - w, n, u, w);
        }
        for (n = w; n > l; --n) /*switch to insertion sort*/
            for (m = n; m < u && xs[m] < xs[m-1]; ++m)
                swap(xs, m, m - 1);
    }
}
#endif /* imsort_h */
