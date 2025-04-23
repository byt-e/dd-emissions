#!/usr/bin/awk -f
NR > 1 {
    if (prev != "") {
        delta = $5 - prev
        printf "%s USD = %.2f\n", $1, delta
    }
    prev = $5
}
